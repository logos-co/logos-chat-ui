#include "mainwindow.h"

#include <QApplication>
#include <QDir>
#include <QDebug>
#include <iostream>
#include <cstdlib>
#include <csignal>

#include <QSocketNotifier>

#include <unistd.h>
#include <vector>

#ifdef __APPLE__
#include <libproc.h>
#elif defined(__linux__)
#include <dirent.h>
#endif

// Replace CoreManager with direct C API functions
extern "C" {
    void logos_core_set_plugins_dir(const char* plugins_dir);
    void logos_core_start();
    void logos_core_cleanup();
    char** logos_core_get_loaded_plugins();
    int logos_core_load_plugin(const char* plugin_name);
}

// Helper function to convert C-style array to QStringList
QStringList convertPluginsToStringList(char** plugins) {
    QStringList result;
    if (plugins) {
        for (int i = 0; plugins[i] != nullptr; i++) {
            result.append(plugins[i]);
        }
    }
    return result;
}

static QString g_instanceTmpDir;
static std::vector<pid_t> g_childPids; // child PIDs recorded at startup

// --- Async-signal-safe signal handling via self-pipe trick ---
static int s_signalFd[2]; // pipe for self-pipe trick

static void setupInstanceTempDir()
{
    const char* inheritedTmp = std::getenv("TMPDIR");
    const QString baseTmp = (inheritedTmp && *inheritedTmp)
        ? QString::fromLocal8Bit(inheritedTmp)
        : QStringLiteral("/tmp");
    const QString instanceTmpDir =
        QDir(baseTmp).filePath(QStringLiteral("logos-instance-%1").arg(qint64(getpid())));

    if (QDir().mkpath(instanceTmpDir)) {
        g_instanceTmpDir = instanceTmpDir;
        qputenv("TMPDIR", g_instanceTmpDir.toUtf8());
        std::cout << "Instance temp dir: " << g_instanceTmpDir.toStdString() << std::endl;
        return;
    }

    std::cerr << "Failed to create instance temp dir: "
              << instanceTmpDir.toStdString() << std::endl;
}

static void signalHandler(int)
{
    // Only async-signal-safe calls here: write a single byte to the pipe.
    char c = 1;
    (void)::write(s_signalFd[1], &c, sizeof(c));
}

// Snapshot direct child PIDs of this process using platform-native APIs.
// Called once after plugin loading while the app is fully functional.
static void recordChildPids()
{
#ifdef __APPLE__
    pid_t pids[128];
    int count = proc_listchildpids(getpid(), pids, sizeof(pids));
    for (int i = 0; i < count; ++i) {
        if (pids[i] > 0)
            g_childPids.push_back(pids[i]);
    }
#elif defined(__linux__)
    pid_t self = getpid();
    DIR* procDir = opendir("/proc");
    if (!procDir) return;

    struct dirent* entry;
    while ((entry = readdir(procDir))) {
        // Skip non-numeric entries
        char* end;
        long pid = strtol(entry->d_name, &end, 10);
        if (*end != '\0' || pid <= 0) continue;

        char statPath[64];
        snprintf(statPath, sizeof(statPath), "/proc/%ld/stat", pid);
        FILE* fp = fopen(statPath, "r");
        if (!fp) continue;

        // /proc/<pid>/stat format: pid (comm) state ppid ...
        // Skip past the comm field (may contain spaces/parens)
        pid_t filePid;
        char comm[256];
        char state;
        pid_t ppid;
        if (fscanf(fp, "%d %255s %c %d", &filePid, comm, &state, &ppid) == 4) {
            if (ppid == self)
                g_childPids.push_back(static_cast<pid_t>(pid));
        }
        fclose(fp);
    }
    closedir(procDir);
#endif
}

static void cleanup()
{
    static bool cleaned = false;
    if (cleaned) return;
    cleaned = true;

    // Kill child processes (e.g. logos_host) BEFORE logos_core_cleanup(),
    // because the core's own termination logic can crash children and
    // abort cleanup midway, leaving stragglers.
    for (pid_t pid : g_childPids) {
        if (kill(pid, 0) == 0) {
            kill(pid, SIGKILL);
        }
    }

    logos_core_cleanup();

    if (!g_instanceTmpDir.isEmpty()) {
        QDir(g_instanceTmpDir).removeRecursively();
    }
}

int main(int argc, char *argv[])
{
    // Set up per-instance temp directory before QApplication.
    setupInstanceTempDir();

    // Create QApplication after the temp dir setup.
    QApplication app(argc, argv);

    // --- Async-signal-safe signal handling via self-pipe trick ---
    // Create self-pipe
    if (::pipe(s_signalFd) == 0) {
        // Notifier fires on the main event loop when the signal handler writes
        auto* notifier = new QSocketNotifier(s_signalFd[0], QSocketNotifier::Read, &app);
        QObject::connect(notifier, &QSocketNotifier::activated, [notifier]() {
            notifier->setEnabled(false);
            char c;
            (void)::read(s_signalFd[0], &c, sizeof(c));
            // We're on the main thread now – safe to call Qt/cleanup APIs
            cleanup();
            QCoreApplication::quit();
        });

        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
    }

    // Ensure cleanup runs on normal exit (e.g. window close)
    QObject::connect(&app, &QCoreApplication::aboutToQuit, []() {
        cleanup();
    });

    // Set the plugins directory
    QString pluginsDir = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../modules");
    std::cout << "Setting plugins directory to: " << pluginsDir.toStdString() << std::endl;
    logos_core_set_plugins_dir(pluginsDir.toUtf8().constData());

    // Start the core
    logos_core_start();
    std::cout << "Logos Core started successfully!" << std::endl;

    // Load plugins in required order
    std::cout << "Loading plugins in specified order..." << std::endl;
    
    // Load capability_module first (handles auth tokens)
    if (logos_core_load_plugin("capability_module")) {
        std::cout << "Successfully loaded capability_module plugin" << std::endl;
    } else {
        std::cerr << "Failed to load capability_module plugin" << std::endl;
    }
    
    // Then load chatsdk_module
    if (logos_core_load_plugin("chatsdk_module")) {
        std::cout << "Successfully loaded chatsdk_module plugin" << std::endl;
    } else {
        std::cerr << "Failed to load chatsdk_module plugin" << std::endl;
    }

    // Record child PIDs spawned by the core (e.g. logos_host) so we can
    // reliably terminate them during cleanup without depending on pgrep
    // or Qt at shutdown time.
    recordChildPids();

    // Print all loaded plugins
    char** loadedPlugins = logos_core_get_loaded_plugins();
    QStringList plugins = convertPluginsToStringList(loadedPlugins);
    
    if (plugins.isEmpty()) {
        qInfo() << "No plugins loaded.";
    } else {
        qInfo() << "Currently loaded plugins:";
        for (const QString &plugin : plugins) {
            qInfo() << "  -" << plugin;
        }
        qInfo() << "Total plugins:" << plugins.size();
    }
    
    // Create and show the main window
    MainWindow window;
    window.show();
    
    // Run the application
    return app.exec();
}

#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QDebug>
#include <QIcon>
#include <QRegularExpression>
#include <iostream>
#include <memory>

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

int main(int argc, char *argv[])
{
    // Create QApplication first
    QApplication app(argc, argv);
    QApplication::setWindowIcon(QIcon(":/icons/app_icon.png"));

    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption showSettingsOption("show-settings", "Show the settings tab in the UI");
    parser.addOption(showSettingsOption);
    parser.process(app);

    // Store as application property so plugins can access it
    app.setProperty("showSettings", parser.isSet(showSettingsOption));

    // Set the plugins directory
    QString pluginsDir = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../modules");
    std::cout << "Setting plugins directory to: " << pluginsDir.toStdString() << std::endl;
    logos_core_set_plugins_dir(pluginsDir.toUtf8().constData());

    // Start the core
    logos_core_start();
    std::cout << "Logos Core started successfully!" << std::endl;

    // Explicitly load plugins in specified order
    std::cout << "Loading plugins in specified order..." << std::endl;
    
    // Load capability_module plugin first
    if (logos_core_load_plugin("capability_module")) {
        std::cout << "Successfully loaded capability_module plugin" << std::endl;
    } else {
        std::cerr << "Failed to load capability_module plugin" << std::endl;
    }
    
    // Load waku plugin second
    if (logos_core_load_plugin("waku_module")) {
        std::cout << "Successfully loaded waku_module plugin" << std::endl;
    } else {
        std::cerr << "Failed to load waku_module plugin" << std::endl;
    }
    
    // Then load chat plugin
    if (logos_core_load_plugin("chat")) {
        std::cout << "Successfully loaded chat plugin" << std::endl;
    } else {
        std::cerr << "Failed to load chat plugin" << std::endl;
    }

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
    window.raise();
    window.activateWindow();
    
    // Run the application
    int result = app.exec();
    
    // Cleanup core before exit
    logos_core_cleanup();
    
    return result;
}

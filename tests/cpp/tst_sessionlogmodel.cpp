#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QTextStream>

#include "SessionLogModel.h"

// The model holds a bounded window of a file that one module can write far
// faster than the rest. What that window is made of decides whether the console
// can still answer for the quiet ones.
class TestSessionLogModel : public QObject
{
    Q_OBJECT

private slots:
    void keepsAQuietDomainWhenALoudOneFloods();
    void keepsTheNewestOfTheDomainItTrims();
    void keepsTheSevereLinesOfALoudDomain();
    void keepsWhatItReadWhenTheFileRotates();
};

namespace {

// `quiet` chat_ui lines, then `loud` delivery ones, which is the order a real
// session writes them in: the app announces itself once and the node then talks
// for as long as it runs. Every `errorEvery`-th loud line states an error, the
// way the delivery node reports one.
QString writeLog(const QTemporaryDir& dir, int quiet, int loud, int errorEvery = 0)
{
    const QString path = dir.filePath(QStringLiteral("session.log"));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return {};
    QTextStream out(&file);
    for (int i = 0; i < quiet; ++i)
        out << "[2026-07-28 01:24:57.164] [info] [chat_ui] started " << i << "\n";
    for (int i = 0; i < loud; ++i) {
        const bool severe = errorEvery > 0 && i % errorEvery == 0;
        out << "[2026-07-28 01:25:01.856] [out] [delivery_module] " << (severe ? "ERR" : "DBG")
            << " 2026-07-28 01:25:01.854+02:00 relay " << i << "\n";
    }
    out.flush();
    return path;
}

int rowsOfLevel(const SessionLogModel& model, LogLine::Level level)
{
    int found = 0;
    for (int row = 0; row < model.rowCount(); ++row)
        if (model.lineAt(row).level == level)
            ++found;
    return found;
}

int rowsOfDomain(const SessionLogModel& model, const QString& domain)
{
    int found = 0;
    for (int row = 0; row < model.rowCount(); ++row)
        if (model.lineAt(row).domain == domain)
            ++found;
    return found;
}

} // namespace

void TestSessionLogModel::keepsAQuietDomainWhenALoudOneFloods()
{
    QTemporaryDir dir;
    SessionLogModel model;
    model.follow(writeLog(dir, 20, 6000));

    // Every one of the twenty survives: dropping the oldest lines outright
    // would have taken all of them, and the console would then offer a chat-ui
    // chip counting twenty lines it could not show.
    QCOMPARE(rowsOfDomain(model, QStringLiteral("chat-ui")), 20);
    QCOMPARE(model.lineCount(QStringLiteral("chat-ui")), 20);
    QVERIFY(model.rowCount() < 6020);
}

void TestSessionLogModel::keepsTheNewestOfTheDomainItTrims()
{
    QTemporaryDir dir;
    SessionLogModel model;
    model.follow(writeLog(dir, 20, 6000));

    // What a flooding domain loses is its oldest, so the tail a reader is
    // watching is the part that stays.
    const LogLine newest = model.lineAt(model.rowCount() - 1);
    QCOMPARE(newest.domain, QStringLiteral("delivery"));
    QCOMPARE(newest.message, QStringLiteral("relay 5999"));
}

void TestSessionLogModel::keepsTheSevereLinesOfALoudDomain()
{
    QTemporaryDir dir;
    SessionLogModel model;
    model.follow(writeLog(dir, 20, 6000, 100));

    // The chatter around them goes first: an error is the one line a reader
    // opened the console for, and the count in the header has to stay
    // answerable.
    QCOMPARE(model.errorCount(), 60);
    QCOMPARE(rowsOfLevel(model, LogLine::Error), 60);
}

void TestSessionLogModel::keepsWhatItReadWhenTheFileRotates()
{
    QTemporaryDir dir;
    SessionLogModel model;
    const QString path = writeLog(dir, 5, 5);
    model.follow(path);
    QCOMPARE(model.rowCount(), 10);

    // What the host does when a file fills: the full one moves aside and a
    // fresh one opens under the name the run announced, which the reader sees
    // only as the file shrinking.
    QVERIFY(QFile::rename(path, dir.filePath(QStringLiteral("rotated.001.log"))));
    QFile fresh(path);
    QVERIFY(fresh.open(QIODevice::WriteOnly | QIODevice::Text));
    fresh.write("[2026-07-28 01:26:00.000] [info] [chat_ui] after the rotation\n");
    fresh.close();

    // The ten already read still describe this session, so they stay.
    QTRY_COMPARE(model.rowCount(), 11);
    QCOMPARE(model.lineAt(10).message, QStringLiteral("after the rotation"));
    QCOMPARE(model.lineCount(), 11);
}

QTEST_GUILESS_MAIN(TestSessionLogModel)
#include "tst_sessionlogmodel.moc"

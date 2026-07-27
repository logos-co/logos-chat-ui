#include <QTest>

#include "LogLine.h"

// The session log is a merge of several writers in several processes, and the
// console's whole account of a line comes from parsing it. These are the shapes
// the file actually carries.
class TestLogLine : public QObject
{
    Q_OBJECT

private slots:
    void readsAModulesOwnDiagnostic();
    void readsAModulesStdoutAsUnlevelled();
    void attributesALibchatEventToLibchat();
    void toleratesAnEmptyField();
    void leavesAnUnattributedLineWithTheHost();
    void keepsAnUnrecognisedLineWhole();
    void readsThisModulesOwnLines();
    void takesTheSeverityAModuleStatedItself();
    void keepsTheModuleWhenAnInnerScopeIsNamedToo();
};

void TestLogLine::readsAModulesOwnDiagnostic()
{
    const LogLine line = LogLine::parse(
        QStringLiteral("[2026-07-27 14:32:07.114] [error] [chat_module] "
                       "chat_module: delivery_module.send failed: node not started"));

    QCOMPARE(line.time, QStringLiteral("14:32:07.114"));
    QCOMPARE(line.level, LogLine::Error);
    QCOMPARE(line.domain, QStringLiteral("chat-module"));
    QCOMPARE(line.message,
             QStringLiteral("chat_module: delivery_module.send failed: node not started"));
}

void TestLogLine::readsAModulesStdoutAsUnlevelled()
{
    const LogLine line = LogLine::parse(QStringLiteral(
        "[2026-07-27 14:31:52.006] [out] [delivery_module] node: listening on 127.0.0.1:60000"));

    QCOMPARE(line.level, LogLine::Info);
    QCOMPARE(line.domain, QStringLiteral("delivery"));
    QCOMPARE(line.message, QStringLiteral("node: listening on 127.0.0.1:60000"));
}

void TestLogLine::attributesALibchatEventToLibchat()
{
    // A module that names its own severity and target overrides both the tag it
    // arrived under and the level the host ranked it at.
    const LogLine line = LogLine::parse(
        QStringLiteral("[2026-07-27 14:32:01.104] [info] [chat_module] "
                       "WARNING: libchat::inbox_v2: welcome rejected"));

    QCOMPARE(line.level, LogLine::Warning);
    QCOMPARE(line.domain, QStringLiteral("libchat"));
    QCOMPARE(line.message, QStringLiteral("libchat::inbox_v2: welcome rejected"));
}

void TestLogLine::toleratesAnEmptyField()
{
    // A logger with no name renders as an empty field, which names nobody.
    const LogLine line = LogLine::parse(
        QStringLiteral("[2026-07-27 14:32:00.913] [info] [] [chat_module] inbox drained"));

    QCOMPARE(line.domain, QStringLiteral("chat-module"));
    QCOMPARE(line.message, QStringLiteral("inbox drained"));
}

void TestLogLine::leavesAnUnattributedLineWithTheHost()
{
    const LogLine line = LogLine::parse(
        QStringLiteral("[2026-07-27 14:31:44.201] [info] [core] Logos Core started"));

    QCOMPARE(line.domain, QStringLiteral("host"));
    QCOMPARE(line.message, QStringLiteral("Logos Core started"));
}

void TestLogLine::keepsAnUnrecognisedLineWhole()
{
    // Nothing is ever dropped: a shape the parser does not know still reads.
    const QString raw = QStringLiteral("Segmentation fault (core dumped)");
    const LogLine line = LogLine::parse(raw);

    QVERIFY(line.time.isEmpty());
    QCOMPARE(line.level, LogLine::Info);
    QCOMPARE(line.domain, QStringLiteral("host"));
    QCOMPARE(line.message, raw);
}

void TestLogLine::readsThisModulesOwnLines()
{
    const LogLine line = LogLine::parse(
        QStringLiteral("[2026-07-27 14:32:07.902] [warning] [chat_ui] sendMessage refused"));

    QCOMPARE(line.level, LogLine::Warning);
    QCOMPARE(line.domain, QStringLiteral("chat-ui"));
    QCOMPARE(line.message, QStringLiteral("sendMessage refused"));
}

void TestLogLine::takesTheSeverityAModuleStatedItself()
{
    // The delivery node states a severity and a time of its own, and the host
    // relays the whole line as stdout. Ranking it by the host's `[out]` would
    // report an error as ordinary traffic, which is what the reader is hunting.
    const LogLine line = LogLine::parse(
        QStringLiteral("[2026-07-28 01:25:01.856] [out] [delivery_module] "
                       "ERR 2026-07-28 01:25:01.854+02:00 no subscribed peers found"));

    QCOMPARE(line.level, LogLine::Error);
    QCOMPARE(line.domain, QStringLiteral("delivery"));
    // The row already carries the time in its own column.
    QCOMPARE(line.time, QStringLiteral("01:25:01.856"));
    QCOMPARE(line.message, QStringLiteral("no subscribed peers found"));
}

void TestLogLine::keepsTheModuleWhenAnInnerScopeIsNamedToo()
{
    const LogLine line = LogLine::parse(
        QStringLiteral("[2026-07-28 01:24:57.164] [info] [logos] [chat_module] [LogosObject] "
                       "ModuleProxy: callRemoteMethod \"init\""));

    QCOMPARE(line.domain, QStringLiteral("chat-module"));
    QCOMPARE(line.message, QStringLiteral("ModuleProxy: callRemoteMethod \"init\""));
}

QTEST_APPLESS_MAIN(TestLogLine)
#include "tst_logline.moc"

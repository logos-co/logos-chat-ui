#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include "ProcessLog.h"

// Qt's message handler is process-wide, so the log is installed once, over a
// handler standing in for whatever the process had before it.
class TestProcessLog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void keepsDebugLinesOutOfItsFile();

private:
    QTemporaryDir m_dir;
};

namespace {

QStringList passedOn;

void previousHandler(QtMsgType, const QMessageLogContext&, const QString& message)
{
    passedOn.append(message);
}

QString contentsOf(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
}

} // namespace

void TestProcessLog::initTestCase()
{
    qInstallMessageHandler(previousHandler);
    ProcessLog::install();
}

// The SDK logs at debug level, the capability tokens it is handed included.
void TestProcessLog::keepsDebugLinesOutOfItsFile()
{
    qDebug("requestModule result for \"chat_module\" : \"a-capability-token\"");
    qWarning("held until the file opens");
    QVERIFY(ProcessLog::openIn(m_dir.path()));
    qDebug("a debug line");
    qInfo("an info line");

    const QString written = contentsOf(ProcessLog::path());
    QVERIFY2(!written.contains(QStringLiteral("DEBUG")), qPrintable(written));
    QVERIFY2(!written.contains(QStringLiteral("a-capability-token")), qPrintable(written));
    QVERIFY2(written.contains(QStringLiteral("WARNING: default: held until the file opens")), qPrintable(written));
    QVERIFY2(written.contains(QStringLiteral("INFO: default: an info line")), qPrintable(written));
    QCOMPARE(passedOn, QStringList({
        QStringLiteral("requestModule result for \"chat_module\" : \"a-capability-token\""),
        QStringLiteral("held until the file opens"),
        QStringLiteral("a debug line"),
        QStringLiteral("an info line"),
    }));
}

QTEST_MAIN(TestProcessLog)
#include "tst_processlog.moc"

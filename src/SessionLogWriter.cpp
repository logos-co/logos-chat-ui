#include "SessionLogWriter.h"

#include <QDateTime>
#include <QDebug>
#include <QFile>

namespace {

QString levelWord(LogLine::Level level)
{
    switch (level) {
    case LogLine::Debug:
        return QStringLiteral("debug");
    case LogLine::Warning:
        return QStringLiteral("warning");
    case LogLine::Error:
        return QStringLiteral("error");
    default:
        return QStringLiteral("info");
    }
}

} // namespace

void SessionLogWriter::open(const QString& path)
{
    m_path = path;
}

void SessionLogWriter::write(LogLine::Level level, const QString& message)
{
    if (m_path.isEmpty())
        return;

    QFile file(m_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "chat_ui: cannot append to the session log" << m_path << file.errorString();
        return;
    }

    const QString line = QStringLiteral("[%1] [%2] [chat_ui] %3\n")
                             .arg(QDateTime::currentDateTime().toString(
                                      QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")),
                                  levelWord(level), message);
    file.write(line.toUtf8());
}

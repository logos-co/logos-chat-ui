#ifndef SESSION_LOG_WRITER_H
#define SESSION_LOG_WRITER_H

#include <QString>

#include "LogLine.h"

// Appends this module's own lines to the session log, in the shape the host
// writes so one reader parses the whole file.
//
// The file has other writers, in other processes. Every line is one appending
// write, which the kernel keeps whole; nothing here assumes it is alone. Nor is
// the file held open between lines: the host rotates a full one aside and opens
// a fresh file under the same name, and a held handle would go on filling the
// one nobody is reading. There are a handful of lines in a session, so opening
// per line costs nothing worth saving.
class SessionLogWriter
{
public:
    // An empty path leaves the writer inert, which is what a host that captures
    // nothing gets.
    void open(const QString& path);
    void write(LogLine::Level level, const QString& message);

private:
    QString m_path;
};

#endif

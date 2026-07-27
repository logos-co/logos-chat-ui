#ifndef LOG_LINE_H
#define LOG_LINE_H

#include <QString>
#include <QStringList>

// One line of the session log, as the console reads it.
//
// The file is a merge of several writers with several shapes, so a line is
// parsed on a best-effort basis: whatever cannot be recognised keeps its whole
// text and lands in the catch-all domain rather than being dropped.
struct LogLine {
    // Coarse severity. The file carries more level words than this; each maps
    // onto the four the console filters by, and a line with no level reads as
    // Info. The values are a bitmask, so a filter is one integer.
    enum Level {
        Debug = 1 << 0,
        Info = 1 << 1,
        Warning = 1 << 2,
        Error = 1 << 3,
        AllLevels = Debug | Info | Warning | Error
    };

    // Clock time as the file spells it, `HH:mm:ss.zzz`; empty when the line
    // carries none.
    QString time;
    Level level = Info;
    // Which part of the stack spoke. One of the names in `logDomains()`.
    QString domain;
    // The line with the fields the other members carry taken off the front, or
    // the whole line when nothing was recognised.
    QString message;
    // The line exactly as the file holds it, which is what an export writes.
    QString raw;

    // Reads one line of the session log. Never fails.
    static LogLine parse(const QString& line);
};

// The domains a line can be attributed to, in the order the console lists them.
const QStringList& logDomainNames();

#endif

#include "SessionLogModel.h"

#include <QFile>
#include <QFileInfo>

#include <algorithm>
#include <utility>

namespace {

// How many lines stay addressable. A debug-level run writes faster than anyone
// reads, and every row costs a fetch over the replica; the file keeps the rest.
constexpr int kRetainedLines = 5000;

// How often the file is re-read. Every line the poll finds is inserted in one
// batch, which is also what keeps a flood from becoming one packet per line.
constexpr int kPollIntervalMs = 250;

QString levelName(LogLine::Level level)
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

SessionLogModel::SessionLogModel(QObject* parent)
    : QAbstractListModel(parent)
{
    m_poll.setInterval(kPollIntervalMs);
    connect(&m_poll, &QTimer::timeout, this, &SessionLogModel::poll);
}

int SessionLogModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_lines.size();
}

QVariant SessionLogModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= m_lines.size())
        return {};

    const LogLine& line = m_lines.at(index.row());
    switch (role) {
    case TimeRole:
        return line.time;
    case LevelRole:
        return static_cast<int>(line.level);
    case LevelNameRole:
        return levelName(line.level);
    case DomainRole:
        return line.domain;
    case MessageRole:
        return line.message;
    default:
        return {};
    }
}

QHash<int, QByteArray> SessionLogModel::roleNames() const
{
    return {
        {TimeRole, "time"},
        {LevelRole, "level"},
        {LevelNameRole, "levelName"},
        {DomainRole, "domain"},
        {MessageRole, "message"},
    };
}

void SessionLogModel::follow(const QString& path)
{
    if (path == m_path)
        return;

    m_path = path;
    restart();
    if (m_path.isEmpty()) {
        m_poll.stop();
        return;
    }
    poll();
    m_poll.start();
}

QString SessionLogModel::path() const
{
    return m_path;
}

qint64 SessionLogModel::size() const
{
    return m_size;
}

const LogLine& SessionLogModel::lineAt(int row) const
{
    static const LogLine none;
    return row >= 0 && row < m_lines.size() ? m_lines.at(row) : none;
}

int SessionLogModel::lineCount() const
{
    return m_lineCount;
}

int SessionLogModel::lineCount(const QString& domain) const
{
    return m_countByDomain.value(domain);
}

int SessionLogModel::errorCount() const
{
    return m_errorCount;
}

void SessionLogModel::restart()
{
    beginResetModel();
    m_lines.clear();
    m_countByDomain.clear();
    m_lineCount = 0;
    m_errorCount = 0;
    m_offset = 0;
    m_partial.clear();
    m_size = 0;
    endResetModel();
}

void SessionLogModel::poll()
{
    const QFileInfo info(m_path);
    if (!info.exists())
        return;

    const qint64 size = info.size();
    // A file that shrank was rotated: the host moves the full one aside and
    // opens a fresh one under the name it announced. What was read of the old
    // file is still a true account of the session, so the window keeps it and
    // the new file is read from its start.
    if (size < m_offset) {
        m_offset = 0;
        m_partial.clear();
    }
    if (size == m_offset)
        return;

    QFile file(m_path);
    if (!file.open(QIODevice::ReadOnly))
        return;
    if (!file.seek(m_offset))
        return;

    m_partial += file.read(size - m_offset);
    m_offset = size;
    m_size = size;

    const int lastBreak = m_partial.lastIndexOf('\n');
    if (lastBreak < 0) {
        emit grew();
        return;
    }

    const QString complete = QString::fromUtf8(m_partial.left(lastBreak));
    m_partial = m_partial.mid(lastBreak + 1);
    appendLines(complete.split(QLatin1Char('\n')));
    emit grew();
}

void SessionLogModel::appendLines(const QStringList& lines)
{
    QVector<LogLine> parsed;
    parsed.reserve(lines.size());
    for (const QString& line : lines) {
        if (line.isEmpty())
            continue;
        parsed.append(LogLine::parse(line));
    }
    if (parsed.isEmpty())
        return;

    for (const LogLine& line : parsed) {
        ++m_lineCount;
        ++m_countByDomain[line.domain];
        if (line.level == LogLine::Error)
            ++m_errorCount;
    }

    beginInsertRows(QModelIndex(), m_lines.size(), m_lines.size() + parsed.size() - 1);
    m_lines += parsed;
    endInsertRows();

    const int excess = m_lines.size() - kRetainedLines;
    if (excess > 0)
        evict(excess);
}

void SessionLogModel::evict(int excess)
{
    QHash<QString, int> held;
    for (const LogLine& line : std::as_const(m_lines))
        ++held[line.domain];
    const int share = kRetainedLines / logDomainNames().size();

    // Oldest first, but only from a domain over its share: the shares sum to
    // the whole window, so a domain writing thousands of lines a minute always
    // has enough of them to give and one writing a handful is never touched.
    //
    // Ordinary lines go before severe ones, because a window that dropped the
    // errors to keep the chatter around them would leave the console counting
    // errors it could no longer show. The two passes partition the window, so
    // together they can always raise the whole excess.
    QVector<int> doomed;
    doomed.reserve(excess);
    for (const bool severe : {false, true}) {
        for (int row = 0; row < m_lines.size() && doomed.size() < excess; ++row) {
            const LogLine& line = m_lines.at(row);
            if (severe != (line.level == LogLine::Warning || line.level == LogLine::Error))
                continue;
            if (held.value(line.domain) <= share)
                continue;
            --held[line.domain];
            doomed.append(row);
        }
    }
    std::sort(doomed.begin(), doomed.end());

    // Newest run first, so the rows still to be dropped keep the indices this
    // walk gave them.
    for (int last = doomed.size() - 1; last >= 0;) {
        int first = last;
        while (first > 0 && doomed.at(first - 1) == doomed.at(first) - 1)
            --first;
        beginRemoveRows(QModelIndex(), doomed.at(first), doomed.at(last));
        m_lines.remove(doomed.at(first), last - first + 1);
        endRemoveRows();
        last = first - 1;
    }
}

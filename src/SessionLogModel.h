#ifndef SESSION_LOG_MODEL_H
#define SESSION_LOG_MODEL_H

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QString>
#include <QTimer>
#include <QVector>

#include "LogLine.h"

// The session log's lines, read from the file the host assigns and kept as the
// console's source model.
//
// Only a bounded window is held; the file stays the whole truth, which is what
// an export reads. The window is shared out between domains rather than kept
// strictly newest-first, so the one module that writes thousands of lines a
// minute cannot push every quieter one out of reach. Counts are of everything
// the session wrote, so a domain whose lines have aged out is still not silent.
class SessionLogModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        TimeRole = Qt::UserRole + 1,
        LevelRole,
        // The severity as a word, which is both what a row shows and what the
        // view keys its colour off.
        LevelNameRole,
        DomainRole,
        MessageRole
    };

    explicit SessionLogModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Reads `path` from the beginning and keeps reading as it grows, dropping
    // anything read from a file followed earlier. An empty path stops.
    void follow(const QString& path);
    QString path() const;
    qint64 size() const;

    // Row `row` of the model, oldest first. Out of range gives an empty line.
    const LogLine& lineAt(int row) const;

    int lineCount() const;
    int lineCount(const QString& domain) const;
    int errorCount() const;

signals:
    // The file grew: rows, counts and size may all have moved.
    void grew();

private:
    void poll();
    void appendLines(const QStringList& lines);
    // Drops `excess` of the held lines, taking only from domains holding more
    // than an equal share of the window, and their ordinary lines before their
    // severe ones.
    void evict(int excess);
    void restart();

    QVector<LogLine> m_lines;
    QHash<QString, int> m_countByDomain;
    int m_lineCount = 0;
    int m_errorCount = 0;

    QString m_path;
    // Where the next read starts, and the trailing bytes of a line the writer
    // has not finished yet.
    qint64 m_offset = 0;
    QByteArray m_partial;
    qint64 m_size = 0;
    QTimer m_poll;
};

#endif

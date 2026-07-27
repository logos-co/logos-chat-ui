#ifndef LOG_FILTER_MODEL_H
#define LOG_FILTER_MODEL_H

#include <QSet>
#include <QSortFilterProxyModel>
#include <QString>

struct LogLine;

// The console's view of the session log: the lines whose domain, level and text
// the reader has asked for. The predicate lives here rather than in QML because
// a model reaches the view as a replica, which proxies no methods.
class LogFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit LogFilterModel(QObject* parent = nullptr);

    // A bitmask of LogLine::Level.
    int levels() const;
    void setLevels(int levels);

    bool isDomainEnabled(const QString& domain) const;
    void setDomainEnabled(const QString& domain, bool enabled);

    QString text() const;
    void setText(const QString& text);

    // Whether the current filters let a line through. Public because an export
    // of "what is on screen" has to apply the same predicate to the file, which
    // holds more lines than the model retains.
    bool accepts(const LogLine& line) const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    int m_levels;
    QSet<QString> m_hiddenDomains;
    QString m_text;
};

#endif

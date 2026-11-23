#ifndef NAME_BOOK_H
#define NAME_BOOK_H

#include <QHash>
#include <QObject>
#include <QString>

// The display name each account set, by address, as chat_module last reported
// it. Every model that shows a person reads its names from one book.
class NameBook : public QObject
{
    Q_OBJECT

public:
    explicit NameBook(QObject* parent = nullptr);

    // The name `address` set; empty when none is known.
    QString name(const QString& address) const;
    // Records the name chat_module reported for `address`, on one line: an
    // account picks its own, line breaks included.
    void record(const QString& address, const QString& name);

signals:
    void changed(const QString& address);

private:
    QHash<QString, QString> m_names;
};

#endif

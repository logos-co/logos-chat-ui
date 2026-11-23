#include "NameBook.h"

NameBook::NameBook(QObject* parent)
    : QObject(parent)
{
}

QString NameBook::name(const QString& address) const
{
    return m_names.value(address);
}

void NameBook::record(const QString& address, const QString& name)
{
    const QString line = name.simplified();
    if (address.isEmpty() || m_names.value(address) == line)
        return;
    m_names.insert(address, line);
    emit changed(address);
}

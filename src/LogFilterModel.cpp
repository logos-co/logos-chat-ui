#include "LogFilterModel.h"

#include "LogLine.h"
#include "SessionLogModel.h"

LogFilterModel::LogFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent)
    , m_levels(LogLine::AllLevels)
{
}

int LogFilterModel::levels() const
{
    return m_levels;
}

void LogFilterModel::setLevels(int levels)
{
    if (levels == m_levels)
        return;
    m_levels = levels;
    invalidateFilter();
}

bool LogFilterModel::isDomainEnabled(const QString& domain) const
{
    return !m_hiddenDomains.contains(domain);
}

void LogFilterModel::setDomainEnabled(const QString& domain, bool enabled)
{
    if (isDomainEnabled(domain) == enabled)
        return;
    if (enabled)
        m_hiddenDomains.remove(domain);
    else
        m_hiddenDomains.insert(domain);
    invalidateFilter();
}

QString LogFilterModel::text() const
{
    return m_text;
}

void LogFilterModel::setText(const QString& text)
{
    if (text == m_text)
        return;
    m_text = text;
    invalidateFilter();
}

bool LogFilterModel::accepts(const LogLine& line) const
{
    if ((line.level & m_levels) == 0)
        return false;
    if (m_hiddenDomains.contains(line.domain))
        return false;
    return m_text.isEmpty() || line.message.contains(m_text, Qt::CaseInsensitive);
}

bool LogFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    const auto* source = qobject_cast<SessionLogModel*>(sourceModel());
    return source && !sourceParent.isValid() && accepts(source->lineAt(sourceRow));
}

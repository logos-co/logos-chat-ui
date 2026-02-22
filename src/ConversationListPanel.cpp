#include "ConversationListPanel.h"
#include <QFont>

ConversationListPanel::ConversationListPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void ConversationListPanel::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // Set terminal background for the entire panel
    setStyleSheet("background-color: #0A0A0A;");

    // Header with title and new conversation button
    QWidget* headerWidget = new QWidget(this);
    headerWidget->setFixedHeight(50);
    headerWidget->setStyleSheet("background-color: #0A0A0A; border-bottom: 1px solid #2a2a2a;");
    m_headerLayout = new QHBoxLayout(headerWidget);
    m_headerLayout->setContentsMargins(15, 0, 15, 0);

    m_titleLabel = new QLabel("> \xce\xbb chat", headerWidget);
    m_titleLabel->setStyleSheet("color: #FAFAFA; background: transparent;");
    QFont titleFont("JetBrains Mono", 14);
    titleFont.setStyleHint(QFont::Monospace);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);

    m_newConversationButton = new QPushButton(headerWidget);
    m_newConversationButton->setText("+ new");
    m_newConversationButton->setFixedHeight(32);
    m_newConversationButton->setToolTip("New Conversation");
    m_newConversationButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #10B981;"
        "  border: none;"
        "  border-radius: 4px;"
        "  font-size: 12px;"
        "  font-weight: bold;"
        "  color: #0A0A0A;"
        "  padding: 6px 12px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #34D399;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #059669;"
        "}"
    );

    m_headerLayout->addWidget(m_titleLabel);
    m_headerLayout->addStretch();
    m_headerLayout->addWidget(m_newConversationButton);

    // Conversation list
    m_conversationList = new QListWidget(this);
    m_conversationList->setStyleSheet(
        "QListWidget {"
        "  border: none;"
        "  background-color: #0A0A0A;"
        "}"
        "QListWidget::item {"
        "  padding: 12px 15px;"
        "  border-bottom: 1px solid #2a2a2a;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #1F1F1F;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: #1F1F1F;"
        "}"
    );

    // My Bundle button at bottom
    m_myBundleButton = new QPushButton("$ share_handle", this);
    m_myBundleButton->setFixedHeight(68);
    m_myBundleButton->setStyleSheet(
        "QPushButton {"
        "  background-color: transparent;"
        "  border: none;"
        "  border-top: 1px solid #2a2a2a;"
        "  border-radius: 0px;"
        "  padding: 15px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  color: #FAFAFA;"
        "}"
        "QPushButton:hover {"
        "  background-color: #1F1F1F;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #1F1F1F;"
        "}"
    );

    m_mainLayout->addWidget(headerWidget);
    m_mainLayout->addWidget(m_conversationList, 1);
    m_mainLayout->addWidget(m_myBundleButton);

    // Connect signals
    connect(m_newConversationButton, &QPushButton::clicked, 
            this, &ConversationListPanel::onNewConversationClicked);
    connect(m_myBundleButton, &QPushButton::clicked, 
            this, &ConversationListPanel::onMyBundleClicked);
    connect(m_conversationList, &QListWidget::itemClicked, 
            this, &ConversationListPanel::onItemClicked);
}

void ConversationListPanel::addConversation(const QString& id, const QString& name, 
                                             const QDateTime& lastActivity)
{
    if (m_conversationItems.contains(id)) {
        updateConversation(id, lastActivity);
        return;
    }

    QListWidgetItem* item = new QListWidgetItem(m_conversationList);
    item->setData(Qt::UserRole, id);
    updateConversationDisplay(item, name, lastActivity);

    m_conversationItems[id] = item;
    m_conversationData[id] = {name, lastActivity};
}

void ConversationListPanel::updateConversation(const QString& id, const QDateTime& lastActivity)
{
    if (!m_conversationItems.contains(id)) return;

    QListWidgetItem* item = m_conversationItems[id];
    m_conversationData[id].lastActivity = lastActivity;
    updateConversationDisplay(item, m_conversationData[id].name, lastActivity);
}

void ConversationListPanel::removeConversation(const QString& id)
{
    if (!m_conversationItems.contains(id)) return;

    QListWidgetItem* item = m_conversationItems[id];
    delete item;
    m_conversationItems.remove(id);
    m_conversationData.remove(id);
}

void ConversationListPanel::clearConversations()
{
    m_conversationList->clear();
    m_conversationItems.clear();
    m_conversationData.clear();
}

void ConversationListPanel::selectConversation(const QString& id)
{
    if (!m_conversationItems.contains(id)) return;
    m_conversationList->setCurrentItem(m_conversationItems[id]);
}

void ConversationListPanel::onItemClicked(QListWidgetItem* item)
{
    QString id = item->data(Qt::UserRole).toString();
    emit conversationSelected(id);
}

void ConversationListPanel::onNewConversationClicked()
{
    emit newConversationRequested();
}

void ConversationListPanel::onMyBundleClicked()
{
    emit myBundleRequested();
}

QString ConversationListPanel::formatRelativeTime(const QDateTime& dateTime)
{
    QDateTime now = QDateTime::currentDateTime();
    qint64 secs = dateTime.secsTo(now);

    if (secs < 60) {
        return "Just now";
    } else if (secs < 3600) {
        int mins = secs / 60;
        return QString("%1 min ago").arg(mins);
    } else if (secs < 86400) {
        int hours = secs / 3600;
        return QString("%1 hour%2 ago").arg(hours).arg(hours > 1 ? "s" : "");
    } else if (secs < 172800) {
        return "Yesterday";
    } else {
        return dateTime.toString("MMM d");
    }
}

void ConversationListPanel::updateConversationDisplay(QListWidgetItem* item, 
                                                       const QString& name, 
                                                       const QDateTime& lastActivity)
{
    QString displayText = QString("<b style='color: #FAFAFA; font-family: JetBrains Mono, monospace;'>%1</b><br><span style='color: #4B5563; font-size: 10pt; font-family: IBM Plex Mono, monospace;'>%2</span>")
                          .arg(name)
                          .arg(formatRelativeTime(lastActivity));
    
    QLabel* label = new QLabel(displayText);
    label->setTextFormat(Qt::RichText);
    label->setContentsMargins(0, 0, 0, 0);
    label->setStyleSheet("background: transparent;");
    
    item->setSizeHint(QSize(0, 50));
    m_conversationList->setItemWidget(item, label);
}

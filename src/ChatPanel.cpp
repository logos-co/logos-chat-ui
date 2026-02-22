#include "ChatPanel.h"
#include "MessageBubble.h"
#include <QStackedWidget>
#include <QScrollBar>
#include <QTimer>

ChatPanel::ChatPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void ChatPanel::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    m_stackedWidget = new QStackedWidget(this);

    setupEmptyState();
    setupChatState();

    m_stackedWidget->addWidget(m_emptyStateWidget);
    m_stackedWidget->addWidget(m_chatStateWidget);
    m_stackedWidget->setCurrentWidget(m_emptyStateWidget);

    m_mainLayout->addWidget(m_stackedWidget);
}

void ChatPanel::setupEmptyState()
{
    m_emptyStateWidget = new QWidget(this);
    m_emptyStateWidget->setStyleSheet("background-color: #0A0A0A;");
    QVBoxLayout* emptyLayout = new QVBoxLayout(m_emptyStateWidget);

    m_emptyStateLabel = new QLabel("Select a conversation or start a new one", m_emptyStateWidget);
    m_emptyStateLabel->setAlignment(Qt::AlignCenter);
    QFont emptyFont("JetBrains Mono", 14);
    emptyFont.setStyleHint(QFont::Monospace);
    emptyFont.setItalic(true);
    m_emptyStateLabel->setFont(emptyFont);
    m_emptyStateLabel->setStyleSheet("color: #6B7280; background: transparent;");

    emptyLayout->addStretch();
    emptyLayout->addWidget(m_emptyStateLabel);
    emptyLayout->addStretch();
}

void ChatPanel::setupChatState()
{
    m_chatStateWidget = new QWidget(this);
    m_chatLayout = new QVBoxLayout(m_chatStateWidget);
    m_chatLayout->setContentsMargins(0, 0, 0, 0);
    m_chatLayout->setSpacing(0);

    // Title header - terminal theme
    QWidget* headerWidget = new QWidget(m_chatStateWidget);
    headerWidget->setFixedHeight(50);
    headerWidget->setStyleSheet("background-color: #0A0A0A; border-bottom: 1px solid #2a2a2a;");
    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(15, 0, 15, 0);

    m_titleLabel = new QLabel("", headerWidget);
    m_titleLabel->setStyleSheet("color: #FAFAFA; background: transparent;");
    QFont titleFont("JetBrains Mono", 14);
    titleFont.setStyleHint(QFont::Monospace);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();

    // Messages scroll area - terminal theme
    m_scrollArea = new QScrollArea(m_chatStateWidget);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setStyleSheet(
        "QScrollArea {"
        "  border: none;"
        "  background-color: #0A0A0A;"
        "}"
        "QScrollBar:vertical {"
        "  background-color: #0A0A0A;"
        "  width: 8px;"
        "  margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background-color: #2a2a2a;"
        "  border-radius: 4px;"
        "  min-height: 20px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}"
    );

    m_messagesContainer = new QWidget(m_scrollArea);
    m_messagesContainer->setStyleSheet("background-color: #0A0A0A;");
    m_messagesLayout = new QVBoxLayout(m_messagesContainer);
    m_messagesLayout->setContentsMargins(16, 16, 16, 16);
    m_messagesLayout->setSpacing(5);
    m_messagesLayout->addStretch();

    m_scrollArea->setWidget(m_messagesContainer);

    // Input area - terminal theme
    m_inputWidget = new QWidget(m_chatStateWidget);
    m_inputWidget->setFixedHeight(68);
    m_inputWidget->setStyleSheet("background-color: #0A0A0A; border-top: 1px solid #2a2a2a;");
    m_inputLayout = new QHBoxLayout(m_inputWidget);
    m_inputLayout->setContentsMargins(15, 10, 15, 10);
    m_inputLayout->setSpacing(10);

    m_messageInput = new QLineEdit(m_inputWidget);
    m_messageInput->setPlaceholderText("Type a message...");
    m_messageInput->setStyleSheet(
        "QLineEdit {"
        "  border: 1px solid #2a2a2a;"
        "  border-radius: 4px;"
        "  padding: 10px 15px;"
        "  background-color: #0F0F0F;"
        "  color: #FAFAFA;"
        "  font-size: 13px;"
        "}"
        "QLineEdit:focus {"
        "  border-color: #10B981;"
        "}"
        "QLineEdit::placeholder {"
        "  color: #4B5563;"
        "}"
    );

    m_sendButton = new QPushButton(">>", m_inputWidget);
    m_sendButton->setFixedWidth(48);
    m_sendButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #10B981;"
        "  color: #0A0A0A;"
        "  border: none;"
        "  border-radius: 4px;"
        "  font-size: 16px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #34D399;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #059669;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #1F1F1F;"
        "  color: #4B5563;"
        "}"
    );

    m_inputLayout->addWidget(m_messageInput, 1);
    m_inputLayout->addWidget(m_sendButton);

    // Add widgets to chat layout
    m_chatLayout->addWidget(headerWidget);
    m_chatLayout->addWidget(m_scrollArea, 1);
    m_chatLayout->addWidget(m_inputWidget);

    // Connect signals
    connect(m_sendButton, &QPushButton::clicked, this, &ChatPanel::onSendClicked);
    connect(m_messageInput, &QLineEdit::returnPressed, this, &ChatPanel::onReturnPressed);

    // Initially disable input
    m_messageInput->setEnabled(false);
    m_sendButton->setEnabled(false);
}

void ChatPanel::setConversation(const QString& id, const QString& name)
{
    m_currentConversationId = id;
    m_currentConversationName = name;
    m_titleLabel->setText(name);

    // Enable input
    m_messageInput->setEnabled(true);
    m_sendButton->setEnabled(true);
    m_messageInput->setFocus();

    // Show chat state
    m_stackedWidget->setCurrentWidget(m_chatStateWidget);
}

void ChatPanel::clearConversation()
{
    m_currentConversationId.clear();
    m_currentConversationName.clear();
    m_titleLabel->setText("");

    // Disable input
    m_messageInput->setEnabled(false);
    m_sendButton->setEnabled(false);
    m_messageInput->clear();

    // Clear messages
    clearMessages();

    // Show empty state
    m_stackedWidget->setCurrentWidget(m_emptyStateWidget);
}

void ChatPanel::addMessage(const QString& sender, const QString& content, 
                           const QDateTime& timestamp, bool isMe)
{
    Q_UNUSED(sender);
    
    // Insert message before the stretch
    int insertIndex = m_messagesLayout->count() - 1;
    if (insertIndex < 0) insertIndex = 0;

    MessageBubble* bubble = new MessageBubble(content, timestamp, isMe, m_messagesContainer);
    m_messagesLayout->insertWidget(insertIndex, bubble);

    // Scroll to bottom after a short delay to ensure layout is updated
    QTimer::singleShot(10, this, &ChatPanel::scrollToBottom);
}

void ChatPanel::clearMessages()
{
    // Remove all widgets except the stretch
    while (m_messagesLayout->count() > 1) {
        QLayoutItem* item = m_messagesLayout->takeAt(0);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
}

void ChatPanel::onSendClicked()
{
    QString content = m_messageInput->text().trimmed();
    if (content.isEmpty() || m_currentConversationId.isEmpty()) {
        return;
    }

    emit messageSent(m_currentConversationId, content);

    // Add the message to the UI immediately (optimistic update)
    addMessage("Me", content, QDateTime::currentDateTime(), true);

    m_messageInput->clear();
    m_messageInput->setFocus();
}

void ChatPanel::onReturnPressed()
{
    onSendClicked();
}

void ChatPanel::scrollToBottom()
{
    QScrollBar* scrollBar = m_scrollArea->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QStatusBar>
#include <QMenuBar>
#include <QMap>
#include <QDateTime>
#include <QAction>
#include <QLabel>
#include <QMutex>
#include "logos_api.h"
#include "logos_sdk.h"

class ConversationListPanel;
class ChatPanel;

class ChatWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit ChatWindow(LogosAPI* logosAPI = nullptr, QWidget* parent = nullptr);
    ~ChatWindow();

private slots:
    // Menu actions
    void onConversationSelected(const QString& conversationId);
    void onNewConversationRequested();
    void onMyBundleRequested();
    void onMessageSent(const QString& conversationId, const QString& content);
    void onAboutAction();
    
    // Chat lifecycle menu actions
    void onInitChat();
    void onStartChat();
    void onStopChat();
    
    // Event handlers for chat module responses
    void onChatInitResult(const QVariantList& data);
    void onChatStartResult(const QVariantList& data);
    void onChatStopResult(const QVariantList& data);
    void onChatCreateIntroBundleResult(const QVariantList& data);
    void onChatNewMessage(const QVariantList& data);
    void onChatNewConversation(const QVariantList& data);
    void onChatNewPrivateConversationResult(const QVariantList& data);
    void onChatSendMessageResult(const QVariantList& data);
    void onChatGetIdResult(const QVariantList& data);

private:
    void setupUI();
    void setupMenu();
    void setupEventHandlers();
    void updateChatMenuState();
    void showConversationMessages(const QString& conversationId);

    // LogosAPI integration
    LogosAPI* m_logosAPI;
    bool m_ownsLogosAPI;  // Track if we created the LogosAPI ourselves
    LogosModules* m_logos;
    bool m_chatInitialized;
    bool m_chatRunning;
    bool m_pendingBundleRequest;
    bool m_autoStartOnLaunch;
  QString m_pendingInitialMessage;  // Workaround for issue #86
  QString m_myIdentity;
  QMap<QString, QString> m_peerIdentities;  // conversationId -> peerId
    QSplitter* m_splitter;
    ConversationListPanel* m_conversationList;
    ChatPanel* m_chatPanel;
    QStatusBar* m_statusBar;
    
    // Chat menu actions (for enabling/disabling)
    QAction* m_initChatAction;
    QAction* m_startChatAction;
    QAction* m_stopChatAction;
    QLabel* m_identityLabel;

    // Store conversation messages
    struct ConversationInfo {
        QString name;
        QString peerId;
        QDateTime lastActivity;
    };
    struct MessageInfo {
        QString sender;
        QString content;
        QDateTime timestamp;
        bool isMe;
    };
    QMap<QString, ConversationInfo> m_conversations;
    QMap<QString, QList<MessageInfo>> m_messages;
    QString m_currentConversationId;  // Currently selected conversation
    QMutex m_conversationsMutex;  // Protect conversation and message maps from concurrent access
};

#include "ChatBackend.h"
#include "ChatConfig.h"
#include "ConversationListModel.h"
#include "MessageListModel.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTimer>

namespace {

/** If @p content looks like hex (optional 0x), decode to UTF-8; otherwise return as-is. */
QString decodeMessageContent(const QString& content)
{
    const QString trimmed = content.trimmed();
    QString hex = trimmed;
    if (hex.startsWith(QLatin1String("0x"), Qt::CaseInsensitive))
        hex = hex.mid(2).trimmed();
    if (hex.length() < 2 || (hex.length() % 2) != 0)
        return content;

    static const QRegularExpression kHexOnly(QStringLiteral("^[0-9a-fA-F]+$"));
    if (!kHexOnly.match(hex).hasMatch())
        return content;

    const QByteArray bytes = QByteArray::fromHex(hex.toLatin1());
    if (bytes.isEmpty())
        return content;
    return QString::fromUtf8(bytes);
}

} // namespace

ChatBackend::ChatBackend(LogosAPI* logosAPI, QObject* parent)
    : ChatBackendSimpleSource(parent)
    , m_logosAPI(logosAPI ? logosAPI : new LogosAPI("chat_ui", this))
    , m_logos(new LogosModules(m_logosAPI))
    , m_conversationModel(new ConversationListModel(this))
    , m_messageModel(new MessageListModel(this))
{
    setChatStatus(ChatBackendSimpleSource::Disconnected);
    setMyIdentity(QString());
    setStatusMessage(QStringLiteral("Ready"));
    setCurrentConversationId(QString());

    // Defer to the next event-loop iteration so the ui-host can finish exposing
    // this object to Qt Remote Objects before we subscribe to chat_module events
    // (avoids a fixed wall-clock delay).
    QTimer::singleShot(0, this, [this]() {
        setupEventHandlers();
        initChat();
    });
}

ChatBackend::~ChatBackend()
{
    if (chatStatus() == ChatBackendSimpleSource::Running && m_logos) {
        m_logos->chat_module.stopChat();
    }
    delete m_logos;
}

ConversationListModel* ChatBackend::conversationModel() const
{
    return m_conversationModel;
}

MessageListModel* ChatBackend::messageModel() const
{
    return m_messageModel;
}

// ── .rep slot implementations ────────────────────────────────────────────────

void ChatBackend::initChat()
{
    if (!m_logos) {
        emit error(QStringLiteral("LogosAPI not available"));
        return;
    }

    if (chatStatus() != ChatBackendSimpleSource::Disconnected) {
        setStatusMessage(QStringLiteral("Chat already initialized"));
        return;
    }

    QString configJson = ChatConfig::buildConfigJson();
    qDebug() << "ChatBackend: Initializing chat with config:" << configJson;

    setChatStatus(ChatBackendSimpleSource::Initializing);
    setStatusMessage(QStringLiteral("Initializing chat..."));

    bool success = m_logos->chat_module.initChat(configJson);
    if (!success) {
        setChatStatus(ChatBackendSimpleSource::Disconnected);
        setStatusMessage(QStringLiteral("Chat initialization failed"));
        emit error(QStringLiteral("Failed to initialize chat"));
    }
}

void ChatBackend::startChat()
{
    if (!m_logos) {
        emit error(QStringLiteral("LogosAPI not available"));
        return;
    }

    if (chatStatus() != ChatBackendSimpleSource::Initialized) {
        setStatusMessage(QStringLiteral("Chat not initialized"));
        return;
    }

    qDebug() << "ChatBackend: Starting chat...";
    setChatStatus(ChatBackendSimpleSource::Starting);
    setStatusMessage(QStringLiteral("Starting chat..."));

    m_logos->chat_module.setEventCallback();

    bool success = m_logos->chat_module.startChat();
    if (!success) {
        setChatStatus(ChatBackendSimpleSource::Initialized);
        setStatusMessage(QStringLiteral("Chat start failed"));
        emit error(QStringLiteral("Failed to start chat"));
    }
}

void ChatBackend::stopChat()
{
    if (!m_logos) return;

    if (chatStatus() != ChatBackendSimpleSource::Running) {
        setStatusMessage(QStringLiteral("Chat is not running"));
        return;
    }

    qDebug() << "ChatBackend: Stopping chat...";
    setChatStatus(ChatBackendSimpleSource::Stopping);
    setStatusMessage(QStringLiteral("Stopping chat..."));

    bool success = m_logos->chat_module.stopChat();
    if (!success) {
        setChatStatus(ChatBackendSimpleSource::Running);
        setStatusMessage(QStringLiteral("Chat stop failed"));
        emit error(QStringLiteral("Failed to stop chat"));
    }
}

void ChatBackend::createConversation(QString introBundle, QString initialMessage)
{
    if (chatStatus() != ChatBackendSimpleSource::Running || !m_logos) {
        emit error(QStringLiteral("Chat not running"));
        return;
    }

    if (introBundle.isEmpty() || initialMessage.isEmpty()) {
        emit error(QStringLiteral("Bundle and message cannot be empty"));
        return;
    }

    m_pendingInitialMessage = initialMessage;
    setStatusMessage(QStringLiteral("Creating new conversation..."));

    // Content must be hex-encoded for the libchat API
    QString initialMessageHex = QString::fromLatin1(initialMessage.toUtf8().toHex());

    bool success = m_logos->chat_module.newPrivateConversation(introBundle, initialMessageHex);
    if (!success) {
        m_pendingInitialMessage.clear();
        setStatusMessage(QStringLiteral("Failed to create conversation"));
        emit error(QStringLiteral("Failed to create conversation"));
    }
}

void ChatBackend::requestMyBundle()
{
    if (chatStatus() != ChatBackendSimpleSource::Running || !m_logos) {
        emit error(QStringLiteral("Chat not running"));
        return;
    }

    m_pendingBundleRequest = true;
    setStatusMessage(QStringLiteral("Requesting intro bundle..."));

    bool success = m_logos->chat_module.createIntroBundle();
    if (!success) {
        m_pendingBundleRequest = false;
        setStatusMessage(QStringLiteral("Failed to request bundle"));
        emit error(QStringLiteral("Failed to request intro bundle"));
    }
}

void ChatBackend::sendMessage(QString conversationId, QString content)
{
    if (chatStatus() != ChatBackendSimpleSource::Running || !m_logos) {
        emit error(QStringLiteral("Chat not running"));
        return;
    }

    if (conversationId.isEmpty() || content.isEmpty()) return;

    qDebug() << "ChatBackend: Sending message to:" << conversationId;

    QDateTime sentAt = QDateTime::currentDateTime();
    m_messages[conversationId].append({ QStringLiteral("Me"), content, sentAt, true });

    // Update message model if this is the current conversation
    if (conversationId == currentConversationId()) {
        m_messageModel->addMessage(QStringLiteral("Me"), content, sentAt, true);
    }

    emit messageReceived(conversationId, QStringLiteral("Me"), content, true);

    // Content must be hex-encoded for the libchat API
    QString contentHex = QString::fromLatin1(content.toUtf8().toHex());

    bool success = m_logos->chat_module.sendMessage(conversationId, contentHex);
    if (!success) {
        setStatusMessage(QStringLiteral("Failed to send message"));
        emit error(QStringLiteral("Failed to send message"));
    }
}

void ChatBackend::selectConversation(QString conversationId)
{
    if (conversationId == currentConversationId()) return;

    setCurrentConversationId(conversationId);
    m_conversationModel->clearUnread(conversationId);
    showConversationMessages(conversationId);
}

// ── Event handlers ───────────────────────────────────────────────────────────

void ChatBackend::setupEventHandlers()
{
    if (!m_logos) return;

    auto safeInvoke = [this](auto handler) {
        return [this, handler](const QVariantList& data) {
            QMetaObject::invokeMethod(this, [this, handler, data]() {
                (this->*handler)(data);
            }, Qt::QueuedConnection);
        };
    };

    m_logos->chat_module.on("chatInitResult",                    safeInvoke(&ChatBackend::onChatInitResult));
    m_logos->chat_module.on("chatStartResult",                   safeInvoke(&ChatBackend::onChatStartResult));
    m_logos->chat_module.on("chatStopResult",                    safeInvoke(&ChatBackend::onChatStopResult));
    m_logos->chat_module.on("chatCreateIntroBundleResult",       safeInvoke(&ChatBackend::onChatCreateIntroBundleResult));
    m_logos->chat_module.on("chatNewMessage",                    safeInvoke(&ChatBackend::onChatNewMessage));
    m_logos->chat_module.on("chatNewConversation",               safeInvoke(&ChatBackend::onChatNewConversation));
    m_logos->chat_module.on("chatNewPrivateConversationResult",  safeInvoke(&ChatBackend::onChatNewPrivateConversationResult));
    m_logos->chat_module.on("chatSendMessageResult",             safeInvoke(&ChatBackend::onChatSendMessageResult));
    m_logos->chat_module.on("chatGetIdResult",                   safeInvoke(&ChatBackend::onChatGetIdResult));

    qDebug() << "ChatBackend: Event handlers set up";
}

void ChatBackend::showConversationMessages(const QString& conversationId)
{
    m_messageModel->clear();
    if (!m_messages.contains(conversationId))
        return;

    const QList<MessageInfo>& messages = m_messages[conversationId];
    QVector<MessageItem> rows;
    rows.reserve(messages.size());
    for (const MessageInfo& msg : messages) {
        rows.append({ msg.sender, msg.content, msg.timestamp, msg.isMe });
    }
    m_messageModel->addMessages(std::move(rows));
}

void ChatBackend::onChatInitResult(const QVariantList& data)
{
    qDebug() << "ChatBackend: Init result:" << data;

    bool success = data.size() > 0 ? data[0].toBool() : false;
    int returnCode = data.size() > 1 ? data[1].toInt() : -1;
    QString message = data.size() > 2 ? data[2].toString() : QString();

    if (success) {
        setChatStatus(ChatBackendSimpleSource::Initialized);
        setStatusMessage(QStringLiteral("Chat initialized"));
        // Auto-start after init
        startChat();
    } else {
        setChatStatus(ChatBackendSimpleSource::Disconnected);
        setStatusMessage(QString("Init failed (code: %1)").arg(returnCode));
        emit error(QString("Failed to initialize chat: %1").arg(message));
    }
}

void ChatBackend::onChatStartResult(const QVariantList& data)
{
    qDebug() << "ChatBackend: Start result:" << data;

    bool success = data.size() > 0 ? data[0].toBool() : false;
    int returnCode = data.size() > 1 ? data[1].toInt() : -1;
    QString message = data.size() > 2 ? data[2].toString() : QString();

    if (success) {
        setChatStatus(ChatBackendSimpleSource::Running);
        setStatusMessage(QStringLiteral("Connected to network"));
        m_logos->chat_module.getId();
    } else {
        setChatStatus(ChatBackendSimpleSource::Initialized);
        setStatusMessage(QString("Start failed (code: %1)").arg(returnCode));
        emit error(QString("Failed to start chat: %1").arg(message));
    }
}

void ChatBackend::onChatStopResult(const QVariantList& data)
{
    qDebug() << "ChatBackend: Stop result:" << data;

    bool success = data.size() > 0 ? data[0].toBool() : false;
    int returnCode = data.size() > 1 ? data[1].toInt() : -1;

    if (success) {
        setChatStatus(ChatBackendSimpleSource::Disconnected);
        setStatusMessage(QStringLiteral("Chat stopped"));
    } else {
        setChatStatus(ChatBackendSimpleSource::Running);
        setStatusMessage(QString("Stop failed (code: %1)").arg(returnCode));
    }
}

void ChatBackend::onChatCreateIntroBundleResult(const QVariantList& data)
{
    qDebug() << "ChatBackend: Bundle result:" << data;

    if (!m_pendingBundleRequest) return;
    m_pendingBundleRequest = false;

    bool success = data.size() > 0 ? data[0].toBool() : false;
    QString bundleStr = data.size() > 2 ? data[2].toString() : QString();

    if (success && !bundleStr.isEmpty()) {
        setStatusMessage(QStringLiteral("Bundle ready"));
        emit bundleReady(bundleStr);
    } else {
        setStatusMessage(QStringLiteral("Failed to get bundle"));
        emit error(QStringLiteral("Failed to create intro bundle"));
    }
}

void ChatBackend::onChatNewMessage(const QVariantList& data)
{
    qDebug() << "ChatBackend: New message:" << data;
    if (data.isEmpty()) return;

    QString jsonStr = data[0].toString();
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();
    QString conversationId = obj["conversationId"].toString();
    if (conversationId.isEmpty())
        conversationId = obj["conversation_id"].toString();

    QString content = obj["content"].toString();
    content = decodeMessageContent(content);

    QString sender = obj["sender"].toString();
    if (sender.isEmpty()) sender = obj["from"].toString();
    if (sender.isEmpty()) sender = QStringLiteral("Peer");

    QDateTime receivedAt = QDateTime::currentDateTime();

    m_conversationModel->updateLastActivity(conversationId, receivedAt);
    m_messages[conversationId].append({ sender, content, receivedAt, false });

    if (conversationId == currentConversationId()) {
        m_messageModel->addMessage(sender, content, receivedAt, false);
    } else {
        m_conversationModel->incrementUnread(conversationId);
    }

    emit messageReceived(conversationId, sender, content, false);
    setStatusMessage(QString("New message from %1").arg(sender));
}

void ChatBackend::onChatNewConversation(const QVariantList& data)
{
    qDebug() << "ChatBackend: New conversation:" << data;
    if (data.isEmpty()) return;

    QString jsonStr = data[0].toString();
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();
    QString conversationId = obj["conversationId"].toString();
    if (conversationId.isEmpty()) return;

    if (m_conversationModel->contains(conversationId)) {
        m_conversationModel->updateLastActivity(conversationId, QDateTime::currentDateTime());
        return;
    }

    // Extract peer identity
    QString peerId;
    if (obj.contains("peerId"))
        peerId = obj["peerId"].toString();
    else if (obj.contains("peerIdentity"))
        peerId = obj["peerIdentity"].toString();

    QString displayName;
    if (!peerId.isEmpty()) {
        m_peerIdentities[conversationId] = peerId;
        displayName = QString("Chat %1").arg(peerId.left(6));
    } else {
        displayName = QString("Chat %1").arg(conversationId.left(8));
    }

    m_conversationModel->addConversation(conversationId, displayName, peerId,
                                         QDateTime::currentDateTime());

    // If there's a pending initial message, attach it to this conversation
    if (!m_pendingInitialMessage.isEmpty()) {
        QDateTime createdAt = QDateTime::currentDateTime();
        m_messages[conversationId].append({
            QStringLiteral("Me"), m_pendingInitialMessage, createdAt, true
        });
        m_pendingInitialMessage.clear();

        // Auto-select the conversation we just initiated
        selectConversation(conversationId);
    }

    emit conversationCreated(conversationId, displayName);
    setStatusMessage(QString("New conversation created"));
}

void ChatBackend::onChatNewPrivateConversationResult(const QVariantList& data)
{
    qDebug() << "ChatBackend: Private conversation result:" << data;

    bool success = data.size() > 0 ? data[0].toBool() : false;
    int returnCode = data.size() > 1 ? data[1].toInt() : -1;
    bool effectiveSuccess = success || returnCode == 0;

    if (!effectiveSuccess) {
        m_pendingInitialMessage.clear();
        setStatusMessage(QStringLiteral("Failed to create conversation"));
        emit error(QString("Failed to create conversation (code: %1)").arg(returnCode));
        return;
    }

    setStatusMessage(QStringLiteral("Conversation created successfully"));
}

void ChatBackend::onChatSendMessageResult(const QVariantList& data)
{
    qDebug() << "ChatBackend: Send result:" << data;

    bool success = data.size() > 0 ? data[0].toBool() : false;
    int returnCode = data.size() > 1 ? data[1].toInt() : -1;

    if (success) {
        setStatusMessage(QStringLiteral("Message sent"));
    } else {
        setStatusMessage(QString("Send failed (code: %1)").arg(returnCode));
    }
}

void ChatBackend::onChatGetIdResult(const QVariantList& data)
{
    qDebug() << "ChatBackend: ID result:" << data;

    if (data.size() > 0) {
        QString identity = data[0].toString();
        if (!identity.isEmpty()) {
            setMyIdentity(identity);
            qDebug() << "ChatBackend: Identity:" << identity;
        }
    }
}

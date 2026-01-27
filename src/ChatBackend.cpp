#include "ChatBackend.h"
#include <QDebug>
#include <QDateTime>

ChatBackend::ChatBackend(LogosAPI* logosAPI, QObject* parent)
    : QObject(parent),
      m_status(NotInitialized),
      m_logosAPI(nullptr),
      m_logos(nullptr),
      m_currentChannel("")
{
    qDebug() << "Initializing ChatBackend...";
    
    if (logosAPI) {
        m_logosAPI = logosAPI;
    } else {
        m_logosAPI = new LogosAPI("core", this);
    }
    
    m_logos = new LogosModules(m_logosAPI);
    
    // Generate random username with 2 digits that will persist during this class lifetime
    int randomNum = rand() % 100;
    m_username = QString("LogosUser_%1").arg(randomNum, 2, 10, QChar('0'));
    qDebug() << "Generated username for this session:" << m_username;
    
    initializeChat();
}

ChatBackend::~ChatBackend()
{
    qDebug() << "Destroying ChatBackend...";
}

void ChatBackend::setStatus(ChatStatus newStatus)
{
    if (m_status != newStatus) {
        m_status = newStatus;
        emit statusChanged();
        qDebug() << "ChatBackend: Status changed to" << m_status;
    }
}

void ChatBackend::setCurrentChannel(const QString& channel)
{
    if (m_currentChannel != channel) {
        m_currentChannel = channel;
        emit currentChannelChanged();
        qDebug() << "ChatBackend: Current channel changed to" << m_currentChannel;
    }
}

void ChatBackend::initializeChat()
{
    setStatus(Initializing);

    if (!m_logos->chat.on("chatMessage", [this](const QVariantList& data) {
            if (data.size() < 3) {
                qWarning() << "ChatBackend: chatMessage payload missing fields";
                return;
            }
            onChatMessage(data);
        })) {
        qWarning() << "ChatBackend: failed to subscribe to chatMessage events";
    }

    if (!m_logos->chat.on("historyMessage", [this](const QVariantList& data) {
            if (data.size() < 3) {
                qWarning() << "ChatBackend: historyMessage payload missing fields";
                return;
            }
            onHistoryMessage(data);
        })) {
        qWarning() << "ChatBackend: failed to subscribe to historyMessage events";
    }

    bool success = m_logos->chat.initialize();

    if (!success) {
        setStatus(Error);
        return;
    }

    setStatus(Ready);

    // Set default channel name and join it
    QString defaultChannel = "baixa-chiado";
    joinChannel(defaultChannel);
}

void ChatBackend::joinChannel(const QString& channelName)
{
    if (channelName.trimmed().isEmpty()) {
        qWarning() << "ChatBackend: Cannot join channel with empty name";
        return;
    }

    if (m_status != Ready) {
        qWarning() << "ChatBackend: Chat not ready, cannot join channel";
        return;
    }

    clearMessages();

    bool success = m_logos->chat.joinChannel(channelName);
    if (success) {
        setCurrentChannel(channelName);
        
        QVariantMap systemMessage;
        systemMessage["sender"] = "System";
        systemMessage["message"] = "You have joined channel: " + channelName;
        systemMessage["timestamp"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        systemMessage["isSystem"] = true;
        systemMessage["isHistory"] = false;
        m_messages.append(systemMessage);
        emit messagesChanged();

        qDebug() << "ChatBackend: Joined channel:" << channelName;

        addMessage("System", "--- Message History ---", false);
        bool histSuccess = m_logos->chat.retrieveHistory(channelName);
        qDebug() << "ChatBackend: retrieveHistory result:" << histSuccess;
    } else {
        qWarning() << "ChatBackend: Failed to join channel:" << channelName;
    }
}

void ChatBackend::sendMessage(const QString& message)
{
    if (message.trimmed().isEmpty()) {
        return;
    }

    if (m_status != Ready) {
        qWarning() << "ChatBackend: Chat not ready, cannot send message";
        return;
    }

    if (m_currentChannel.isEmpty()) {
        qWarning() << "ChatBackend: No channel selected, cannot send message";
        return;
    }

    if (m_logosAPI && m_logosAPI->getClient("chat")->isConnected()) {
        m_logos->chat.sendMessage(m_currentChannel, m_username, message);
        qDebug() << "ChatBackend: Sent message to channel" << m_currentChannel;
    } else {
        qWarning() << "ChatBackend: LogosAPI not connected";
    }
}

void ChatBackend::clearMessages()
{
    m_messages.clear();
    emit messagesChanged();
}

void ChatBackend::addMessage(const QString& sender, const QString& message, bool isHistory)
{
    QVariantMap messageData;
    messageData["sender"] = sender;
    messageData["message"] = message;
    messageData["timestamp"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    messageData["isSystem"] = false;
    messageData["isHistory"] = isHistory;
    
    m_messages.append(messageData);
    emit messagesChanged();
}

void ChatBackend::onChatMessage(const QVariantList& data)
{
    if (data.size() < 3) {
        qWarning() << "ChatBackend::onChatMessage: Invalid data";
        return;
    }

    QString timestamp = data[0].toString();
    QString sender = data[1].toString();
    QString message = data[2].toString();

    qDebug() << "ChatBackend::onChatMessage: [" << timestamp << "]" << sender << ":" << message;

    addMessage(sender, message, false);
}

void ChatBackend::onHistoryMessage(const QVariantList& data)
{
    if (data.size() < 3) {
        qWarning() << "ChatBackend::onHistoryMessage: Invalid data";
        return;
    }

    QString timestamp = data[0].toString();
    QString sender = data[1].toString();
    QString message = data[2].toString();

    qDebug() << "ChatBackend::onHistoryMessage: [" << timestamp << "]" << sender << ":" << message;

    addMessage("[HISTORY] " + sender, message, true);
}

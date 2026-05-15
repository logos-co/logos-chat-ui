#include "ChatBackend.h"
#include "ConversationListModel.h"
#include "MessageListModel.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTimer>
#include <cstdlib>

namespace {

constexpr int kEventPollIntervalMs = 200;
constexpr int kDefaultDeliveryPort = 60000;
constexpr const char* kDefaultDeliveryPreset = "logos.dev";
constexpr const char* kInstancePathEnvVar = "CHAT_MODULE_INSTANCE_PATH";
constexpr const char* kDeliveryPortEnvVar = "CHAT_MODULE_DELIVERY_PORT";

QDateTime msToDateTime(qint64 ms)
{
    return ms > 0 ? QDateTime::fromMSecsSinceEpoch(ms) : QDateTime::currentDateTime();
}

} // namespace

ChatBackend::ChatBackend(LogosAPI* logosAPI, QObject* parent)
    : ChatBackendSimpleSource(parent)
    , m_logosAPI(logosAPI ? logosAPI : new LogosAPI("chat_ui", this))
    , m_logos(new LogosModules(m_logosAPI))
    , m_conversationModel(new ConversationListModel(this))
    , m_messageModel(new MessageListModel(this))
    , m_eventPollTimer(new QTimer(this))
    , m_instancePath(resolveInstancePath())
{
    setChatStatus(ChatBackendSimpleSource::Stopped);
    setMyIdentity(QString());
    setStatusMessage(QStringLiteral("Ready"));
    setCurrentConversationId(QString());

    m_eventPollTimer->setInterval(kEventPollIntervalMs);
    m_eventPollTimer->setTimerType(Qt::CoarseTimer);
    connect(m_eventPollTimer, &QTimer::timeout, this, &ChatBackend::pollEvents);

    // Defer to the next event-loop iteration so the ui-host can finish exposing
    // this object to Qt Remote Objects before the first IPC roundtrip.
    QTimer::singleShot(0, this, [this]() { initialiseModule(); });
}

ChatBackend::~ChatBackend()
{
    if (m_eventPollTimer)
        m_eventPollTimer->stop();
    if (m_logos)
        m_logos->chat_module.module_shutdown();
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

// ── instance path ───────────────────────────────────────────────────────────

QString ChatBackend::resolveInstancePath()
{
    if (const char* env = std::getenv(kInstancePathEnvVar); env && *env) {
        QString path = QString::fromUtf8(env);
        QDir().mkpath(path);
        return path;
    }

    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty())
        base = QDir::homePath() + QStringLiteral("/.local/share");
    const QString path = base + QStringLiteral("/chat_module");
    QDir().mkpath(path);
    return path;
}

int ChatBackend::resolveDeliveryPort()
{
    const char* env = std::getenv(kDeliveryPortEnvVar);
    if (!env || !*env) return kDefaultDeliveryPort;

    bool ok = false;
    const int parsed = QString::fromUtf8(env).toInt(&ok);
    if (!ok) {
        qWarning() << "ChatBackend: ignoring non-integer" << kDeliveryPortEnvVar << "=" << env;
        return kDefaultDeliveryPort;
    }
    return parsed;
}

// ── lifecycle ───────────────────────────────────────────────────────────────

void ChatBackend::initialiseModule()
{
    if (!m_logos) {
        setChatStatus(ChatBackendSimpleSource::Error);
        setStatusMessage(QStringLiteral("LogosAPI not available"));
        emit error(QStringLiteral("LogosAPI not available"));
        return;
    }

    setChatStatus(ChatBackendSimpleSource::Initialising);
    setStatusMessage(QStringLiteral("Initialising chat..."));
    const int port = resolveDeliveryPort();
    qDebug() << "ChatBackend: module_init at" << m_instancePath << "port" << port;

    int rc = m_logos->chat_module.module_init(m_instancePath,
                                              QString::fromLatin1(kDefaultDeliveryPreset),
                                              port);
    if (rc != 0) {
        setChatStatus(ChatBackendSimpleSource::Error);
        setStatusMessage(QString::asprintf("init failed (rc=%d)", rc));
        emit error(QString::asprintf("Failed to initialise chat (rc=%d)", rc));
        return;
    }

    m_moduleInitialised = true;

    const QString identity = m_logos->chat_module.module_get_installation_name();
    if (!identity.isEmpty())
        setMyIdentity(identity);

    // Apply current delivery state from the snapshot in case we missed the
    // initial deliveryStateChanged event before subscribing.
    const QString statusJson = m_logos->chat_module.module_status_json();
    QJsonDocument statusDoc = QJsonDocument::fromJson(statusJson.toUtf8());
    if (statusDoc.isObject()) {
        const QJsonObject obj = statusDoc.object();
        applyDeliveryState(obj.value(QStringLiteral("delivery_state")).toString(),
                           obj.value(QStringLiteral("detail")).toString());
    }

    rehydrateConversations();
    m_eventPollTimer->start();
}

void ChatBackend::rehydrateConversations()
{
    if (!m_moduleInitialised) return;

    const QString raw = m_logos->chat_module.module_list_conversations_json();
    QJsonDocument doc = QJsonDocument::fromJson(raw.toUtf8());
    if (!doc.isArray()) return;

    m_conversationModel->clear();
    const QJsonArray arr = doc.array();
    for (const QJsonValue& v : arr) {
        if (!v.isObject()) continue;
        const QJsonObject obj = v.toObject();
        const QString convoId = obj.value(QStringLiteral("convo_id")).toString();
        if (convoId.isEmpty()) continue;
        const QString nickname = obj.value(QStringLiteral("nickname")).toString();
        const qint64 lastTs = static_cast<qint64>(obj.value(QStringLiteral("last_ts")).toDouble());
        m_conversationModel->addConversation(convoId,
                                             nickname.isEmpty() ? fallbackDisplayName(convoId)
                                                                : nickname,
                                             msToDateTime(lastTs));
    }
}

void ChatBackend::showConversationMessages(const QString& convoId)
{
    m_messageModel->clear();
    if (!m_moduleInitialised || convoId.isEmpty()) return;

    const QString raw = m_logos->chat_module.module_get_messages_json(convoId);
    QJsonDocument doc = QJsonDocument::fromJson(raw.toUtf8());
    if (!doc.isArray()) return;

    QVector<MessageItem> rows;
    const QJsonArray arr = doc.array();
    rows.reserve(arr.size());
    for (const QJsonValue& v : arr) {
        if (!v.isObject()) continue;
        const QJsonObject obj = v.toObject();
        const bool fromSelf = obj.value(QStringLiteral("from_self")).toBool();
        const QString content = obj.value(QStringLiteral("content")).toString();
        const qint64 ts = static_cast<qint64>(obj.value(QStringLiteral("timestamp_ms")).toDouble());
        rows.append({ fromSelf ? QStringLiteral("Me") : QStringLiteral("Peer"),
                      content, msToDateTime(ts), fromSelf });
    }
    m_messageModel->addMessages(std::move(rows));
}

// ── .rep slot implementations ───────────────────────────────────────────────

void ChatBackend::createConversation(QString introBundle, QString initialMessage)
{
    if (chatStatus() != ChatBackendSimpleSource::Online || !m_logos) {
        emit error(QStringLiteral("Chat not online"));
        return;
    }
    if (introBundle.isEmpty() || initialMessage.isEmpty()) {
        emit error(QStringLiteral("Bundle and message cannot be empty"));
        return;
    }

    setStatusMessage(QStringLiteral("Creating new conversation..."));
    const QString convoId = m_logos->chat_module.module_create_conversation(introBundle, initialMessage);
    if (convoId.isEmpty()) {
        setStatusMessage(QStringLiteral("Failed to create conversation"));
        emit error(QStringLiteral("Failed to create conversation"));
    }
    // The conversationCreated event (and the initial message recorded in
    // module history) surface via the next drain — the appliers handle the
    // UI side from there.
}

void ChatBackend::requestMyBundle()
{
    if (chatStatus() != ChatBackendSimpleSource::Online || !m_logos) {
        emit error(QStringLiteral("Chat not online"));
        return;
    }

    setStatusMessage(QStringLiteral("Requesting intro bundle..."));
    const QString bundle = m_logos->chat_module.module_create_intro_bundle();
    if (bundle.isEmpty()) {
        setStatusMessage(QStringLiteral("Failed to get bundle"));
        emit error(QStringLiteral("Failed to create intro bundle"));
        return;
    }
    setStatusMessage(QStringLiteral("Bundle ready"));
    emit bundleReady(bundle);
}

void ChatBackend::sendMessage(QString conversationId, QString content)
{
    if (chatStatus() != ChatBackendSimpleSource::Online || !m_logos) {
        emit error(QStringLiteral("Chat not online"));
        return;
    }
    if (conversationId.isEmpty() || content.isEmpty()) return;

    const int rc = m_logos->chat_module.module_send_message(conversationId, content);
    if (rc != 0) {
        setStatusMessage(QString::asprintf("Send failed (rc=%d)", rc));
        emit error(QString::asprintf("Failed to send message (rc=%d)", rc));
    }
    // On success the module emits a messageSent event, which applyMessageSent
    // appends to the model on the next poll.
}

void ChatBackend::selectConversation(QString conversationId)
{
    if (conversationId == currentConversationId()) return;

    setCurrentConversationId(conversationId);
    m_conversationModel->clearUnread(conversationId);
    showConversationMessages(conversationId);
}

// ── event polling ───────────────────────────────────────────────────────────

void ChatBackend::pollEvents()
{
    if (!m_moduleInitialised || !m_logos) return;

    const QString raw = m_logos->chat_module.module_drain_events_json();
    if (raw.isEmpty() || raw == QStringLiteral("[]")) return;

    QJsonDocument doc = QJsonDocument::fromJson(raw.toUtf8());
    if (!doc.isArray()) return;

    const QJsonArray arr = doc.array();
    for (const QJsonValue& v : arr) {
        if (!v.isObject()) continue;
        const QJsonObject obj = v.toObject();
        const QString type = obj.value(QStringLiteral("type")).toString();

        if (type == QStringLiteral("messageReceived")) {
            applyMessageReceived(obj);
        } else if (type == QStringLiteral("messageSent")) {
            applyMessageSent(obj);
        } else if (type == QStringLiteral("conversationCreated")) {
            applyConversationCreated(obj);
        } else if (type == QStringLiteral("conversationUpdated")) {
            applyConversationUpdated(obj);
        } else if (type == QStringLiteral("conversationDeleted")) {
            applyConversationDeleted(obj);
        } else if (type == QStringLiteral("deliveryStateChanged")) {
            applyDeliveryState(obj.value(QStringLiteral("state")).toString(),
                               obj.value(QStringLiteral("detail")).toString());
        } else if (type == QStringLiteral("eventsDropped")) {
            qWarning() << "ChatBackend: events dropped, rehydrating"
                       << obj.value(QStringLiteral("count")).toInt();
            rehydrateConversations();
            if (!currentConversationId().isEmpty())
                showConversationMessages(currentConversationId());
        } else {
            qDebug() << "ChatBackend: unknown event type" << type;
        }
    }
}

void ChatBackend::applyDeliveryState(const QString& state, const QString& detail)
{
    ChatBackendSimpleSource::ChatStatus next = ChatBackendSimpleSource::Stopped;
    QString msg;
    if (state == QStringLiteral("online")) {
        next = ChatBackendSimpleSource::Online;
        msg = QStringLiteral("Connected to network");
    } else if (state == QStringLiteral("initialising")) {
        next = ChatBackendSimpleSource::Initialising;
        msg = QStringLiteral("Initialising chat...");
    } else if (state == QStringLiteral("error")) {
        next = ChatBackendSimpleSource::Error;
        msg = detail.isEmpty() ? QStringLiteral("Delivery error") : detail;
    } else if (state == QStringLiteral("stopped")) {
        next = ChatBackendSimpleSource::Stopped;
        msg = QStringLiteral("Chat stopped");
    } else {
        return;
    }

    setChatStatus(next);
    if (!detail.isEmpty() && state == QStringLiteral("error"))
        msg = detail;
    setStatusMessage(msg);
}

void ChatBackend::applyMessageReceived(const QJsonObject& evt)
{
    const QString convoId = evt.value(QStringLiteral("convo_id")).toString();
    if (convoId.isEmpty()) return;
    const QString content = evt.value(QStringLiteral("content")).toString();
    const qint64 ts = static_cast<qint64>(evt.value(QStringLiteral("timestamp_ms")).toDouble());
    const QDateTime when = msToDateTime(ts);

    if (!m_conversationModel->contains(convoId)) {
        m_conversationModel->addConversation(convoId, fallbackDisplayName(convoId), when);
    } else {
        m_conversationModel->updateLastActivity(convoId, when);
    }

    if (convoId == currentConversationId()) {
        m_messageModel->addMessage(QStringLiteral("Peer"), content, when, false);
    } else {
        m_conversationModel->incrementUnread(convoId);
    }

    emit messageReceived(convoId, QStringLiteral("Peer"), content, false);
    setStatusMessage(QStringLiteral("New message"));
}

void ChatBackend::applyMessageSent(const QJsonObject& evt)
{
    const QString convoId = evt.value(QStringLiteral("convo_id")).toString();
    if (convoId.isEmpty()) return;
    const QString content = evt.value(QStringLiteral("content")).toString();
    const qint64 ts = static_cast<qint64>(evt.value(QStringLiteral("timestamp_ms")).toDouble());
    const QDateTime when = msToDateTime(ts);

    if (!m_conversationModel->contains(convoId)) {
        m_conversationModel->addConversation(convoId, fallbackDisplayName(convoId), when);
    } else {
        m_conversationModel->updateLastActivity(convoId, when);
    }

    if (convoId == currentConversationId())
        m_messageModel->addMessage(QStringLiteral("Me"), content, when, true);

    emit messageReceived(convoId, QStringLiteral("Me"), content, true);
    setStatusMessage(QStringLiteral("Message sent"));
}

void ChatBackend::applyConversationCreated(const QJsonObject& evt)
{
    const QString convoId = evt.value(QStringLiteral("convo_id")).toString();
    if (convoId.isEmpty()) return;
    const bool isOutgoing = evt.value(QStringLiteral("is_outgoing")).toBool();
    const QString peerLabel = evt.value(QStringLiteral("peer_label")).toString();
    const QString displayName = fallbackDisplayName(convoId, peerLabel);
    const QDateTime now = QDateTime::currentDateTime();

    if (!m_conversationModel->contains(convoId)) {
        m_conversationModel->addConversation(convoId, displayName, now);
    } else {
        m_conversationModel->updateDisplayName(convoId, displayName);
        m_conversationModel->updateLastActivity(convoId, now);
    }

    emit conversationCreated(convoId, displayName);

    if (isOutgoing && currentConversationId().isEmpty())
        selectConversation(convoId);
}

void ChatBackend::applyConversationUpdated(const QJsonObject& evt)
{
    Q_UNUSED(evt);
    // Cheapest correct option: refresh the whole list. The IPC call is local
    // and conversation counts are small.
    rehydrateConversations();
}

void ChatBackend::applyConversationDeleted(const QJsonObject& evt)
{
    const QString convoId = evt.value(QStringLiteral("convo_id")).toString();
    if (convoId.isEmpty()) return;

    m_conversationModel->removeConversation(convoId);
    if (convoId == currentConversationId()) {
        setCurrentConversationId(QString());
        m_messageModel->clear();
    }
}

QString ChatBackend::fallbackDisplayName(const QString& convoId, const QString& peerLabel)
{
    if (!peerLabel.isEmpty())
        return QStringLiteral("Chat ") + peerLabel;
    return QStringLiteral("Chat ") + convoId.left(8);
}

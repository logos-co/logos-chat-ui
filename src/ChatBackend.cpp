#include "ChatBackend.h"
#include "ConversationListModel.h"
#include "MessageListModel.h"

// Generated umbrella: LogosModules (behind modules()) from
// metadata.json#dependencies — the Qt-typed chat_module wrapper.
#include "logos_sdk.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QVariantMap>
#include <cstdlib>

namespace {

constexpr int kDefaultDeliveryPort = 60000;
constexpr const char* kDefaultDeliveryPreset = "logos.test";
constexpr const char* kInstancePathEnvVar = "CHAT_MODULE_INSTANCE_PATH";
constexpr const char* kDeliveryPortEnvVar = "CHAT_MODULE_DELIVERY_PORT";

QDateTime msToDateTime(qint64 ms)
{
    return ms > 0 ? QDateTime::fromMSecsSinceEpoch(ms) : QDateTime::currentDateTime();
}

} // namespace

ChatBackend::ChatBackend(QObject* parent)
    : ChatBackendSimpleSource(parent)
    , m_conversationModel(new ConversationListModel(this))
    , m_messageModel(new MessageListModel(this))
    , m_instancePath(resolveInstancePath())
{
    setChatStatus(ChatBackendSimpleSource::Stopped);
    setMyIdentity(QString());
    setStatusMessage(QStringLiteral("Ready"));
    setCurrentConversationId(QString());
}

void ChatBackend::onContextReady()
{
    // Fires after the framework has wired modules(), so the typed chat_module
    // surface is live and the QtRO source is registered — the right point to
    // initialise the module and arm event subscriptions.
    initialiseModule();
}

ChatBackend::~ChatBackend()
{
    if (isContextReady())
        modules().chat_module.shutdown();
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
    setChatStatus(ChatBackendSimpleSource::Initialising);
    setStatusMessage(QStringLiteral("Initialising chat..."));
    const int port = resolveDeliveryPort();
    qDebug() << "ChatBackend: init at" << m_instancePath << "port" << port;

    const LogosResult res = modules().chat_module.init(m_instancePath,
                                                      QString::fromLatin1(kDefaultDeliveryPreset),
                                                      port);
    if (!res.success) {
        const QString reason = res.getError<QString>();
        setChatStatus(ChatBackendSimpleSource::Error);
        setStatusMessage(QStringLiteral("init failed: ") + reason);
        emit error(QStringLiteral("Failed to initialise chat: ") + reason);
        return;
    }

    m_moduleInitialised = true;

    const QString identity = modules().chat_module.get_installation_name();
    if (!identity.isEmpty())
        setMyIdentity(identity);

    // Subscribe before the initial snapshot so no event fires in the gap
    // between snapshotting and registering the listeners.
    subscribeToEvents();

    // Take the initial snapshot and mark it done *before* seeding delivery
    // state. If status() already reports online (re-attaching to a still-running,
    // already-connected module), the seed's online transition then drives the
    // recovery refetch through applyDeliveryState instead of being dropped by
    // the m_initialSnapshotDone gate — which previously left history missing
    // until a reconnect that never came.
    rehydrateConversations();
    m_initialSnapshotDone = true;

    // Seed delivery state from the snapshot in case delivery_state_changed
    // fired during init(), before subscribeToEvents() registered the listener.
    const QVariantMap status = modules().chat_module.status().toMap();
    applyDeliveryState(status.value(QStringLiteral("delivery_state")).toString(),
                       status.value(QStringLiteral("detail")).toString());
}

void ChatBackend::subscribeToEvents()
{
    auto& chat = modules().chat_module;
    chat.on(QStringLiteral("message_received"),
            [this](const QVariantList& a) { applyMessageReceived(a); });
    chat.on(QStringLiteral("message_sent"),
            [this](const QVariantList& a) { applyMessageSent(a); });
    chat.on(QStringLiteral("conversation_created"),
            [this](const QVariantList& a) { applyConversationCreated(a); });
    chat.on(QStringLiteral("conversation_updated"),
            [this](const QVariantList& a) { applyConversationUpdated(a); });
    chat.on(QStringLiteral("conversation_deleted"),
            [this](const QVariantList& a) { applyConversationDeleted(a); });
    chat.on(QStringLiteral("delivery_state_changed"), [this](const QVariantList& a) {
        applyDeliveryState(a.value(0).toString(), a.value(1).toString());
    });
}

void ChatBackend::rehydrateConversations()
{
    if (!m_moduleInitialised) return;

    const QVariantList convos = modules().chat_module.list_conversations();
    m_conversationModel->clear();
    for (const QVariant& v : convos) {
        const QVariantMap obj = v.toMap();
        const QString convoId = obj.value(QStringLiteral("convo_id")).toString();
        if (convoId.isEmpty()) continue;
        const QString nickname = obj.value(QStringLiteral("nickname")).toString();
        const qint64 lastActivity = obj.value(QStringLiteral("last_activity_ms")).toLongLong();
        m_conversationModel->addConversation(convoId,
                                             nickname.isEmpty() ? fallbackDisplayName(convoId)
                                                                : nickname,
                                             msToDateTime(lastActivity));
    }
}

void ChatBackend::showConversationMessages(const QString& convoId)
{
    m_messageModel->clear();
    if (!m_moduleInitialised || convoId.isEmpty()) return;

    const QVariantList msgs = modules().chat_module.get_messages(convoId);
    QVector<MessageItem> rows;
    rows.reserve(msgs.size());
    for (const QVariant& v : msgs) {
        const QVariantMap obj = v.toMap();
        const bool fromSelf = obj.value(QStringLiteral("from_self")).toBool();
        const QString content = obj.value(QStringLiteral("content")).toString();
        const qint64 ts = obj.value(QStringLiteral("timestamp_ms")).toLongLong();
        rows.append({ fromSelf ? QStringLiteral("Me") : QStringLiteral("Peer"),
                      content, msToDateTime(ts), fromSelf });
    }
    m_messageModel->addMessages(std::move(rows));
}

void ChatBackend::deferToEventLoop(std::function<void()> work)
{
    QMetaObject::invokeMethod(this, std::move(work), Qt::QueuedConnection);
}

// ── .rep slot implementations ───────────────────────────────────────────────

void ChatBackend::createConversation(QString introBundle, QString initialMessage)
{
    if (chatStatus() != ChatBackendSimpleSource::Online || !isContextReady()) {
        emit error(QStringLiteral("Chat not online"));
        return;
    }
    if (introBundle.isEmpty() || initialMessage.isEmpty()) {
        emit error(QStringLiteral("Bundle and message cannot be empty"));
        return;
    }

    setStatusMessage(QStringLiteral("Creating new conversation..."));
    const LogosResult res = modules().chat_module.create_conversation(introBundle, initialMessage);
    if (!res.success) {
        const QString reason = res.getError<QString>();
        setStatusMessage(QStringLiteral("Failed to create conversation: ") + reason);
        emit error(QStringLiteral("Failed to create conversation: ") + reason);
    }
    // The conversation_created event (and the initial message recorded in
    // module history) surface via the push subscription — the appliers handle
    // the UI side from there.
}

void ChatBackend::requestMyBundle()
{
    if (chatStatus() != ChatBackendSimpleSource::Online || !isContextReady()) {
        emit error(QStringLiteral("Chat not online"));
        return;
    }

    setStatusMessage(QStringLiteral("Requesting intro bundle..."));
    const LogosResult res = modules().chat_module.create_intro_bundle();
    if (!res.success) {
        const QString reason = res.getError<QString>();
        setStatusMessage(QStringLiteral("Failed to get bundle: ") + reason);
        emit error(QStringLiteral("Failed to create intro bundle: ") + reason);
        return;
    }
    setStatusMessage(QStringLiteral("Bundle ready"));
    emit bundleReady(res.getValue<QString>());
}

void ChatBackend::sendMessage(QString conversationId, QString content)
{
    if (chatStatus() != ChatBackendSimpleSource::Online || !isContextReady()) {
        emit error(QStringLiteral("Chat not online"));
        return;
    }
    if (conversationId.isEmpty() || content.isEmpty()) return;

    const LogosResult res = modules().chat_module.send_message(conversationId, content);
    if (!res.success) {
        const QString reason = res.getError<QString>();
        setStatusMessage(QStringLiteral("Send failed: ") + reason);
        emit error(QStringLiteral("Failed to send message: ") + reason);
    }
    // On success the module emits a message_sent event, which applyMessageSent
    // appends to the model.
}

void ChatBackend::selectConversation(QString conversationId)
{
    if (conversationId == currentConversationId()) return;

    setCurrentConversationId(conversationId);
    m_conversationModel->clearUnread(conversationId);
    showConversationMessages(conversationId);
}

// ── event handlers ────────────────────────────────────────────────────────────

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

    const bool becameOnline =
        next == ChatBackendSimpleSource::Online && chatStatus() != ChatBackendSimpleSource::Online;

    setChatStatus(next);
    setStatusMessage(msg);

    // Events are push-only, so a fresh online transition (reconnect) is our
    // cue to refetch the lists and recover anything missed while offline.
    // Deferred: this runs inside a module event callback and the refetch makes
    // synchronous module reads (see deferToEventLoop).
    if (becameOnline && m_initialSnapshotDone) {
        deferToEventLoop([this] {
            rehydrateConversations();
            if (!currentConversationId().isEmpty())
                showConversationMessages(currentConversationId());
        });
    }
}

void ChatBackend::applyMessageReceived(const QVariantList& args)
{
    const QString convoId = args.value(0).toString();
    if (convoId.isEmpty()) return;
    const QString content = args.value(1).toString();
    const qint64 ts = args.value(2).toLongLong();
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

    setStatusMessage(QStringLiteral("New message"));
}

void ChatBackend::applyMessageSent(const QVariantList& args)
{
    const QString convoId = args.value(0).toString();
    if (convoId.isEmpty()) return;
    const QString content = args.value(1).toString();
    const qint64 ts = args.value(2).toLongLong();
    const QDateTime when = msToDateTime(ts);

    if (!m_conversationModel->contains(convoId)) {
        m_conversationModel->addConversation(convoId, fallbackDisplayName(convoId), when);
    } else {
        m_conversationModel->updateLastActivity(convoId, when);
    }

    if (convoId == currentConversationId())
        m_messageModel->addMessage(QStringLiteral("Me"), content, when, true);

    setStatusMessage(QStringLiteral("Message sent"));
}

void ChatBackend::applyConversationCreated(const QVariantList& args)
{
    const QString convoId = args.value(0).toString();
    if (convoId.isEmpty()) return;
    const bool isOutgoing = args.value(1).toBool();
    const QString peerLabel = args.value(2).toString();
    const QString displayName = fallbackDisplayName(convoId, peerLabel);
    const QDateTime now = QDateTime::currentDateTime();

    if (!m_conversationModel->contains(convoId)) {
        m_conversationModel->addConversation(convoId, displayName, now);
    } else {
        m_conversationModel->updateDisplayName(convoId, displayName);
        m_conversationModel->updateLastActivity(convoId, now);
    }

    if (isOutgoing && currentConversationId().isEmpty())
        // Deferred: selectConversation makes a synchronous module read and this
        // runs inside a module event callback (see deferToEventLoop).
        deferToEventLoop([this, convoId] { selectConversation(convoId); });
}

void ChatBackend::applyConversationUpdated(const QVariantList& args)
{
    Q_UNUSED(args);
    // Cheapest correct option: refresh the whole list (the read is cheap and
    // conversation counts are small). Deferred — this runs inside a module event
    // callback (see deferToEventLoop).
    deferToEventLoop([this] { rehydrateConversations(); });
}

void ChatBackend::applyConversationDeleted(const QVariantList& args)
{
    const QString convoId = args.value(0).toString();
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

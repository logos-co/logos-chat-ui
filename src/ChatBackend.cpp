#include "ChatBackend.h"
#include "ConversationListModel.h"
#include "MessageListModel.h"
#include "MemberListModel.h"

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
    , m_memberModel(new MemberListModel(this))
    , m_instancePath(resolveInstancePath())
{
    setChatStatus(ChatBackendSimpleSource::Stopped);
    setMyIdentity(QString());
    setStatusMessage(QStringLiteral("Ready"));
    setCurrentConversationId(QString());
    syncCurrentConversationMeta();
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

MemberListModel* ChatBackend::memberModel() const
{
    return m_memberModel;
}

// ── instance path ───────────────────────────────────────────────────────────

// The chat_module's own instance directory, passed to it via init(). The UI
// only chooses the location — the module owns the contents. Override with
// CHAT_MODULE_INSTANCE_PATH to run side-by-side instances; otherwise it
// defaults under the app's data location.
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
        const bool isGroup = obj.value(QStringLiteral("kind")).toString() == QStringLiteral("group");
        m_conversationModel->addConversation(convoId,
                                             nickname.isEmpty()
                                                 ? fallbackDisplayName(convoId, QString(), isGroup)
                                                 : nickname,
                                             msToDateTime(lastActivity), isGroup);
    }
    // The rebuilt list may now know the current conversation's kind/name.
    syncCurrentConversationMeta();
}

// Push the current conversation's derived view state (group flag, display name)
// as backend properties. The models reach QML as QtRO replicas that don't proxy
// ConversationListModel's isGroupFor/displayNameFor Q_INVOKABLE lookups, so the
// view binds these instead. Call whenever the selection or the list changes.
void ChatBackend::syncCurrentConversationMeta()
{
    const QString id = currentConversationId();
    setCurrentIsGroup(m_conversationModel->isGroupFor(id));
    setCurrentDisplayName(m_conversationModel->displayNameFor(id));
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
        const QString sender = obj.value(QStringLiteral("sender")).toString();
        rows.append({ fromSelf ? QStringLiteral("Me") : shortSenderLabel(sender),
                      content, msToDateTime(ts), fromSelf });
    }
    m_messageModel->addMessages(std::move(rows));
}

void ChatBackend::deferToEventLoop(std::function<void()> work)
{
    QMetaObject::invokeMethod(this, std::move(work), Qt::QueuedConnection);
}

// ── .rep slot implementations ───────────────────────────────────────────────

void ChatBackend::createConversation(QString peerAddress)
{
    if (chatStatus() != ChatBackendSimpleSource::Online || !isContextReady()) {
        emit error(QStringLiteral("Chat not online"));
        return;
    }
    if (peerAddress.isEmpty()) {
        emit error(QStringLiteral("Address cannot be empty"));
        return;
    }

    setStatusMessage(QStringLiteral("Creating new conversation..."));
    const LogosResult res = modules().chat_module.create_conversation(peerAddress);
    if (!res.success) {
        const QString reason = res.getError<QString>();
        setStatusMessage(QStringLiteral("Failed to create conversation: ") + reason);
        emit error(QStringLiteral("Failed to create conversation: ") + reason);
    }
    // The conversation_created event surfaces via the push subscription — the
    // appliers handle the UI side from there.
}

void ChatBackend::createGroupConversation()
{
    if (chatStatus() != ChatBackendSimpleSource::Online || !isContextReady()) {
        emit error(QStringLiteral("Chat not online"));
        return;
    }

    setStatusMessage(QStringLiteral("Creating group..."));
    const LogosResult res = modules().chat_module.create_group_conversation();
    if (!res.success) {
        const QString reason = res.getError<QString>();
        setStatusMessage(QStringLiteral("Failed to create group: ") + reason);
        emit error(QStringLiteral("Failed to create group: ") + reason);
    }
    // The group starts with only this member; conversation_created selects it.
    // Members are added afterwards from the members panel.
}

void ChatBackend::addGroupMember(QString conversationId, QString peerAddress)
{
    if (chatStatus() != ChatBackendSimpleSource::Online || !isContextReady()) {
        emit error(QStringLiteral("Chat not online"));
        return;
    }
    if (conversationId.isEmpty() || peerAddress.isEmpty()) {
        emit error(QStringLiteral("Address cannot be empty"));
        return;
    }

    const LogosResult res = modules().chat_module.add_group_member(conversationId, peerAddress);
    if (!res.success) {
        const QString reason = res.getError<QString>();
        setStatusMessage(QStringLiteral("Failed to add member: ") + reason);
        emit error(QStringLiteral("Failed to add member: ") + reason);
        return;
    }
    // The add is committed and the welcome delivered asynchronously, so be
    // honest that the peer is not in the group yet.
    setStatusMessage(QStringLiteral("Invite sent; peer joins when the group commits"));
}

void ChatBackend::requestMyAddress()
{
    if (chatStatus() != ChatBackendSimpleSource::Online || !isContextReady()) {
        emit error(QStringLiteral("Chat not online"));
        return;
    }

    setStatusMessage(QStringLiteral("Requesting address..."));
    const QString address = modules().chat_module.get_address();
    if (address.isEmpty()) {
        setStatusMessage(QStringLiteral("Failed to get address"));
        emit error(QStringLiteral("Failed to get address"));
        return;
    }
    setStatusMessage(QStringLiteral("Address ready"));
    emit addressReady(address);
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
    syncCurrentConversationMeta();
    m_conversationModel->clearUnread(conversationId);
    showConversationMessages(conversationId);
    // Reset the roster on every switch: a group whose roster we can't fetch
    // right now (offline) must show empty, not the previous conversation's
    // members. refreshMembers then loads the new group's roster when it can, and
    // keeps the last-known one across a transient offline of the same group.
    // User-driven, so its synchronous read is safe here (not in an event callback).
    m_memberModel->clear();
    setMemberCount(0);
    refreshMembers();
}

void ChatBackend::refreshMembers()
{
    const QString convoId = currentConversationId();
    if (convoId.isEmpty() || !m_conversationModel->isGroupFor(convoId)) {
        m_memberModel->clear();
        setMemberCount(0);
        return;
    }
    if (chatStatus() != ChatBackendSimpleSource::Online || !isContextReady())
        return; // a group, but we can't fetch now; keep the last-known roster

    if (m_myAddress.isEmpty())
        m_myAddress = modules().chat_module.get_address();

    // list_group_members returns [GroupMember], so the typed wrapper is a
    // QVariantList (each element a QVariantMap), like the other record lists.
    const QVariantList members = modules().chat_module.list_group_members(convoId);
    QVector<MemberItem> rows;
    rows.reserve(members.size());
    for (const QVariant& v : members) {
        const QString address = v.toMap().value(QStringLiteral("address")).toString();
        // An empty address is the roster's "no confirmed account" signal; keep
        // it — the model renders it as "unknown_account". Only a real account
        // address can be self.
        rows.append({ address, !address.isEmpty() && address == m_myAddress });
    }
    m_memberModel->setMembers(rows);
    setMemberCount(static_cast<int>(rows.size()));
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
    const QString sender = args.value(3).toString();
    const QDateTime when = msToDateTime(ts);

    if (!m_conversationModel->contains(convoId)) {
        // Defensive: ConversationStarted normally lands first with the kind.
        // Add it now and backfill the kind by re-reading the list (deferred:
        // this runs inside a module event callback, see deferToEventLoop).
        m_conversationModel->addConversation(convoId, fallbackDisplayName(convoId), when, false);
        deferToEventLoop([this] { rehydrateConversations(); });
    } else {
        m_conversationModel->updateLastActivity(convoId, when);
    }

    if (convoId == currentConversationId()) {
        m_messageModel->addMessage(shortSenderLabel(sender), content, when, false);
        // A message from someone not yet on the roster means the group grew;
        // refetch (deferred: sync module read from inside an event callback).
        if (!sender.isEmpty() && !m_memberModel->contains(sender))
            deferToEventLoop([this] { refreshMembers(); });
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
        m_conversationModel->addConversation(convoId, fallbackDisplayName(convoId), when, false);
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
    const bool isGroup = args.value(3).toString() == QStringLiteral("group");
    const QString displayName = fallbackDisplayName(convoId, peerLabel, isGroup);
    const QDateTime now = QDateTime::currentDateTime();

    if (!m_conversationModel->contains(convoId)) {
        m_conversationModel->addConversation(convoId, displayName, now, isGroup);
    } else {
        m_conversationModel->updateDisplayName(convoId, displayName);
        m_conversationModel->updateLastActivity(convoId, now);
    }

    // Open a conversation we just created, so creating a chat or group lands
    // the user in it. Deferred: selectConversation makes a synchronous module
    // read and this runs inside a module event callback (see deferToEventLoop).
    if (isOutgoing)
        deferToEventLoop([this, convoId] { selectConversation(convoId); });
}

void ChatBackend::applyConversationUpdated(const QVariantList& args)
{
    const QString convoId = args.value(0).toString();
    // Cheapest correct option: refresh the whole list (the read is cheap and
    // conversation counts are small). Deferred — this runs inside a module event
    // callback (see deferToEventLoop).
    deferToEventLoop([this, convoId] {
        rehydrateConversations();
        // A group update (e.g. a member added) may have grown the roster of the
        // conversation on screen; refetch it.
        if (convoId == currentConversationId() && m_conversationModel->isGroupFor(convoId))
            refreshMembers();
    });
}

void ChatBackend::applyConversationDeleted(const QVariantList& args)
{
    const QString convoId = args.value(0).toString();
    if (convoId.isEmpty()) return;

    m_conversationModel->removeConversation(convoId);
    if (convoId == currentConversationId()) {
        setCurrentConversationId(QString());
        syncCurrentConversationMeta();
        m_messageModel->clear();
    }
}

QString ChatBackend::fallbackDisplayName(const QString& convoId, const QString& peerLabel,
                                         bool isGroup)
{
    const QString label = peerLabel.isEmpty() ? convoId.left(8) : peerLabel;
    return (isGroup ? QStringLiteral("Group ") : QStringLiteral("Chat ")) + label;
}

QString ChatBackend::shortSenderLabel(const QString& sender)
{
    return sender.isEmpty() ? QStringLiteral("Peer") : sender.left(8);
}

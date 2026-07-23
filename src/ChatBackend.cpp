#include "ChatBackend.h"
#include "ConversationListModel.h"
#include "MessageListModel.h"
#include "MemberListModel.h"

// Generated umbrella: LogosModules (behind modules()) from
// metadata.json#dependencies — the Qt-typed chat_module wrapper.
#include "logos_sdk.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QVariantMap>
#include <utility>

namespace {

// Preview cap; mirrors chat_module's own 160-char truncation so a live preview
// matches the one a rehydrate reads back from the module.
constexpr int kPreviewMaxChars = 160;
constexpr const char* kDefaultDeliveryPreset = "logos.test";

QDateTime msToDateTime(qint64 ms)
{
    return ms > 0 ? QDateTime::fromMSecsSinceEpoch(ms) : QDateTime::currentDateTime();
}

} // namespace

// QtCore Settings (QSettings) persists only when the application has an
// identity, and the module host does not always set one. Provide a fallback so
// UI preferences survive restarts, leaving a host-set identity untouched.
static void ensureApplicationIdentity()
{
    if (QCoreApplication::organizationName().isEmpty())
        QCoreApplication::setOrganizationName(QStringLiteral("Logos"));
    if (QCoreApplication::applicationName().isEmpty())
        QCoreApplication::setApplicationName(QStringLiteral("logos-chat-ui"));
}
Q_COREAPP_STARTUP_FUNCTION(ensureApplicationIdentity)

ChatBackend::ChatBackend(QObject* parent)
    : ChatBackendSimpleSource(parent)
    , m_conversationModel(new ConversationListModel(this))
    , m_conversationProxy(new QSortFilterProxyModel(this))
    , m_messageModel(new MessageListModel(this))
    , m_memberModel(new MemberListModel(this))
{
    // Present conversations newest-first without disturbing the source's
    // insertion order; the proxy re-sorts live as last_activity changes.
    m_conversationProxy->setSourceModel(m_conversationModel);
    m_conversationProxy->setSortRole(ConversationListModel::LastActivityRole);
    m_conversationProxy->setDynamicSortFilter(true);
    m_conversationProxy->sort(0, Qt::DescendingOrder);

    setChatStatus(ChatBackendSimpleSource::Stopped);
    setMyIdentity(QString());
    setStatusMessage(QStringLiteral("Ready"));
    setCurrentConversationId(QString());
    setLoadedConversationId(QString());
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

QAbstractItemModel* ChatBackend::conversationModel() const
{
    return m_conversationProxy;
}

MessageListModel* ChatBackend::messageModel() const
{
    return m_messageModel;
}

MemberListModel* ChatBackend::memberModel() const
{
    return m_memberModel;
}

// ── lifecycle ───────────────────────────────────────────────────────────────

void ChatBackend::initialiseModule()
{
    setChatStatus(ChatBackendSimpleSource::Initialising);
    setStatusMessage(QStringLiteral("Initialising chat..."));

    const LogosResult res = modules().chat_module.init(QString::fromLatin1(kDefaultDeliveryPreset));
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
    chat.on(QStringLiteral("members_changed"),
            [this](const QVariantList& a) { applyMembersChanged(a); });
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
        const QString name = obj.value(QStringLiteral("name")).toString();
        const QString description = obj.value(QStringLiteral("description")).toString();
        const QString preview = obj.value(QStringLiteral("preview")).toString();
        const qint64 lastActivity = obj.value(QStringLiteral("last_activity_ms")).toLongLong();
        const bool isGroup = obj.value(QStringLiteral("kind")).toString() == QStringLiteral("group");
        // Local nickname wins, then the group's shared name, else a generated label.
        const QString displayName = !nickname.isEmpty() ? nickname
            : !name.isEmpty()                           ? name
                                                        : fallbackDisplayName(convoId, QString(), isGroup);
        m_conversationModel->addConversation(convoId, displayName, description,
                                             msToDateTime(lastActivity), isGroup, preview);
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
    setCurrentDescription(m_conversationModel->descriptionFor(id));
}

bool ChatBackend::showConversationMessages(const QString& convoId)
{
    if (convoId.isEmpty()) {
        m_messageModel->clear();
        return true;
    }
    if (!m_moduleInitialised) {
        m_messageModel->clear();
        return false;
    }

    // A failed read comes back as an empty list, so ask for the error too: an
    // empty thread and an unreachable module must not look alike.
    logos::CallError err;
    const QVariantList msgs = modules().chat_module.get_messages(convoId, &err);
    if (!err.ok()) {
        const QString reason = QString::fromStdString(err.message);
        setStatusMessage(QStringLiteral("Could not load messages: ") + reason);
        emit error(QStringLiteral("Could not load messages: ") + reason);
        return false;
    }

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
    m_messageModel->setMessages(std::move(rows));
    return true;
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

    setStatusMessage(QStringLiteral("Creating DM..."));
    const LogosResult res = modules().chat_module.create_conversation(peerAddress);
    if (!res.success) {
        const QString reason = res.getError<QString>();
        setStatusMessage(QStringLiteral("Failed to create DM: ") + reason);
        emit error(QStringLiteral("Failed to create DM: ") + reason);
    }
    // The conversation_created event surfaces via the push subscription — the
    // appliers handle the UI side from there.
}

void ChatBackend::createGroupConversation(QString name, QString description)
{
    if (chatStatus() != ChatBackendSimpleSource::Online || !isContextReady()) {
        emit error(QStringLiteral("Chat not online"));
        return;
    }

    setStatusMessage(QStringLiteral("Creating group..."));
    const LogosResult res = modules().chat_module.create_group_conversation(name, description);
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
    // The commit is async, so the peer isn't in the group yet. Show it as a
    // pending busy row; the members_changed event reconciles it once committed.
    if (conversationId == currentConversationId()) {
        m_pendingMembers.insert(peerAddress);
        refreshMembers();
    }
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
        emit sendFailed(conversationId, content);
    }
    // On success the module emits a message_sent event, which applyMessageSent
    // appends to the model.
}

void ChatBackend::selectConversation(QString conversationId)
{
    // Re-selecting the conversation on screen is a no-op only once its messages
    // are in; while they are not, it is the user's retry.
    if (conversationId == currentConversationId() && conversationId == loadedConversationId())
        return;

    setLoadedConversationId(QString());
    setCurrentConversationId(conversationId);
    syncCurrentConversationMeta();
    m_conversationModel->clearUnread(conversationId);
    const bool loaded = showConversationMessages(conversationId);
    // Reset the roster on every switch: a group whose roster we can't fetch
    // right now (offline) must show empty, not the previous conversation's
    // members. refreshMembers then loads the new group's roster when it can, and
    // keeps the last-known one across a transient offline of the same group.
    // User-driven, so its synchronous read is safe here (not in an event callback).
    m_pendingMembers.clear();
    m_memberModel->clear();
    setMemberCount(0);
    refreshMembers();
    if (loaded)
        setLoadedConversationId(conversationId);
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
    QSet<QString> committed;
    for (const QVariant& v : members) {
        const QString address = v.toMap().value(QStringLiteral("address")).toString();
        // An empty address is the roster's "no confirmed account" signal; keep
        // it — the model renders it as "unknown_account". Only a real account
        // address can be self.
        rows.append({ address, !address.isEmpty() && address == m_myAddress, false });
        if (!address.isEmpty())
            committed.insert(address);
    }
    // Drop invites that have since committed; show the rest as busy rows.
    m_pendingMembers.subtract(committed);
    for (const QString& address : std::as_const(m_pendingMembers))
        rows.append({ address, false, true });

    m_memberModel->setMembers(rows);
    // Committed roster size only; pending invites appear in the list but aren't counted.
    setMemberCount(static_cast<int>(members.size()));
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
            const QString convoId = currentConversationId();
            if (convoId.isEmpty())
                return;
            // The refetch replaces the thread, so the models stop holding it
            // until the reload lands.
            setLoadedConversationId(QString());
            const bool loaded = showConversationMessages(convoId);
            refreshMembers();
            if (loaded)
                setLoadedConversationId(convoId);
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
    const QString preview = content.left(kPreviewMaxChars);

    if (!m_conversationModel->contains(convoId)) {
        // Defensive: ConversationStarted normally lands first with the kind.
        // Add it now and backfill the kind by re-reading the list (deferred:
        // this runs inside a module event callback, see deferToEventLoop).
        m_conversationModel->addConversation(convoId, fallbackDisplayName(convoId), QString(), when, false, preview);
        deferToEventLoop([this] { rehydrateConversations(); });
    } else {
        m_conversationModel->updateLastActivity(convoId, when);
    }
    m_conversationModel->updatePreview(convoId, preview);

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
    const QString preview = content.left(kPreviewMaxChars);

    if (!m_conversationModel->contains(convoId)) {
        m_conversationModel->addConversation(convoId, fallbackDisplayName(convoId), QString(), when, false, preview);
    } else {
        m_conversationModel->updateLastActivity(convoId, when);
    }
    m_conversationModel->updatePreview(convoId, preview);

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
    const QString name = args.value(4).toString();
    const QString description = args.value(5).toString();
    const QString displayName =
        name.isEmpty() ? fallbackDisplayName(convoId, peerLabel, isGroup) : name;
    const QDateTime now = QDateTime::currentDateTime();

    if (!m_conversationModel->contains(convoId)) {
        m_conversationModel->addConversation(convoId, displayName, description, now, isGroup, QString());
    } else {
        m_conversationModel->updateDisplayName(convoId, displayName);
        m_conversationModel->updateDescription(convoId, description);
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

void ChatBackend::applyMembersChanged(const QVariantList& args)
{
    const QString convoId = args.value(0).toString();
    // A commit changed this group's roster; refetch it if it is on screen.
    deferToEventLoop([this, convoId] {
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
        setLoadedConversationId(QString());
        syncCurrentConversationMeta();
        m_messageModel->clear();
    }
}

QString ChatBackend::fallbackDisplayName(const QString& convoId, const QString& peerLabel,
                                         bool isGroup)
{
    const QString label = peerLabel.isEmpty() ? convoId.left(8) : peerLabel;
    return (isGroup ? QStringLiteral("Group ") : QStringLiteral("DM ")) + label;
}

QString ChatBackend::shortSenderLabel(const QString& sender)
{
    return sender.isEmpty() ? QStringLiteral("Peer") : sender.left(8);
}

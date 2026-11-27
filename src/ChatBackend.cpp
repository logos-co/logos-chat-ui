#include "ChatBackend.h"
#include "ConversationListModel.h"
#include "MessageListModel.h"
#include "MemberListModel.h"
#include "Identity.h"
#include "ProcessLog.h"

// Generated umbrella: LogosModules (behind modules()) from
// metadata.json#dependencies — the Qt-typed chat_module wrapper.
#include "logos_sdk.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QSet>
#include <QTime>
#include <QVariantMap>
#include <utility>

namespace {

// Preview cap; mirrors chat_module's own 160-char truncation so a live preview
// matches the one a rehydrate reads back from the module.
constexpr int kPreviewMaxChars = 160;
constexpr const char* kDefaultDeliveryPreset = "logos.test";
// How much of the chat core's account of a run to keep.
// TODO: expose to the client through the UI in the future.
constexpr const char* kChatLogLevel = "info";

// The network Chat asks for: LOGOS_CHAT_DELIVERY_PRESET when it is set, else
// the default.
QString requestedDeliveryPreset()
{
    const QString named = qEnvironmentVariable("LOGOS_CHAT_DELIVERY_PRESET");
    return named.isEmpty() ? QString::fromLatin1(kDefaultDeliveryPreset) : named;
}

// How often the module is asked whether it is still there, and how long that
// question is worth waiting for. Acquiring the object blocks the caller, so the
// timeout is what a dead module costs this thread: short enough not to be felt,
// long enough that a loaded box does not look like a crash.
constexpr int kHealthIntervalMs = 10000;
constexpr int kHealthTimeoutMs = 2000;
// Probes that must go unanswered before the module is called gone. One is a
// hiccup; the announcement is not worth being wrong about.
constexpr int kHealthMissesBeforeGone = 2;

QDateTime msToDateTime(qint64 ms)
{
    return ms > 0 ? QDateTime::fromMSecsSinceEpoch(ms) : QDateTime::currentDateTime();
}

QString humanSize(qint64 bytes)
{
    static const char* const units[] = {"bytes", "KB", "MB", "GB"};
    double size = static_cast<double>(bytes);
    int unit = 0;
    while (size >= 1024.0 && unit < 3) {
        size /= 1024.0;
        ++unit;
    }
    return unit == 0 ? QStringLiteral("%1 bytes").arg(bytes)
                     : QStringLiteral("%1 %2").arg(size, 0, 'f', 1).arg(QLatin1String(units[unit]));
}

// The run's start time as a reader states it, or the stamp itself when the file
// name does not spell one.
QString runLabel(const QString& stamp)
{
    const QDateTime started = QDateTime::fromString(stamp, QStringLiteral("yyyyMMdd_HHmmss"));
    return started.isValid() ? started.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")) : stamp;
}

// One writer's runs, newest first, appended to `published` and tagged with the
// writer so the view can offer each on its own tab. The announced path fixes both
// the directory to read and the naming to group by, so the two writers sharing a
// directory never pick up each other's files.
void appendRuns(QVariantList& published, const QString& writer, const QString& announcedPath)
{
    const QList<SessionLogRun> runs = listSessionLogRuns(announcedPath);
    const QString currentStamp = runs.isEmpty() ? QString() : runs.first().stamp;
    for (const SessionLogRun& run : runs) {
        published.append(QVariantMap{
            {QStringLiteral("writer"), writer},
            {QStringLiteral("label"), runLabel(run.stamp)},
            {QStringLiteral("sizeLabel"), humanSize(run.bytes)},
            {QStringLiteral("fileCount"), run.paths.size()},
            {QStringLiteral("path"), run.paths.isEmpty() ? QString() : run.paths.last()},
            {QStringLiteral("paths"), run.paths.join(QLatin1Char('\n'))},
            {QStringLiteral("stamp"), run.stamp},
            {QStringLiteral("current"), run.stamp == currentStamp},
        });
    }
}

// A conversation's debug data as the developer tools show it, from the map
// get_debug_data returned: see the debugData property for its keys.
QVariantMap debugDataView(const QVariantMap& data, const QString& myAddress)
{
    QVariantList members;
    QMap<QString, int> counts;
    for (const QVariant& entry : data.value(QStringLiteral("members")).toList()) {
        const QVariantMap member = entry.toMap();
        const QString account = member.value(QStringLiteral("account")).toString();
        const QString sync = member.value(QStringLiteral("sync")).toString();
        const bool isSelf = !account.isEmpty() && account == myAddress;
        ++counts[sync];
        const QVariantMap row{
            {QStringLiteral("label"),
             account.isEmpty() ? QStringLiteral("unknown_account") : Identity::shortLabel(account)},
            {QStringLiteral("signer"), member.value(QStringLiteral("signer")).toString()},
            {QStringLiteral("sync"), sync},
            {QStringLiteral("epoch"), member.value(QStringLiteral("epoch"), -1).toInt()},
            {QStringLiteral("epochFingerprint"), member.value(QStringLiteral("epoch_fingerprint")).toString()},
            {QStringLiteral("isSelf"), isSelf},
        };
        if (isSelf)
            members.prepend(row);
        else
            members.append(row);
    }

    // The states libchat names, in the order a reader scans for trouble; a
    // state it adds later is counted after them.
    static const QStringList order = {QStringLiteral("synced"), QStringLiteral("behind"),
                                      QStringLiteral("ahead"), QStringLiteral("forked"),
                                      QStringLiteral("unconfirmed")};
    QVariantList countList;
    const auto count = [&countList](const QString& sync, int n) {
        countList.append(QVariantMap{{QStringLiteral("sync"), sync}, {QStringLiteral("count"), n}});
    };
    for (const QString& sync : order) {
        if (counts.contains(sync))
            count(sync, counts.take(sync));
    }
    for (auto it = counts.cbegin(); it != counts.cend(); ++it)
        count(it.key(), it.value());

    return {
        {QStringLiteral("epoch"), data.value(QStringLiteral("epoch"), -1).toInt()},
        {QStringLiteral("epochFingerprint"), data.value(QStringLiteral("epoch_fingerprint")).toString()},
        {QStringLiteral("members"), members},
        {QStringLiteral("counts"), countList},
        {QStringLiteral("json"),
         QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(data)).toJson(QJsonDocument::Indented))},
        {QStringLiteral("readAt"), QTime::currentTime().toString(QStringLiteral("HH:mm:ss"))},
    };
}

// Sorted descending, it lists this session's conversations before those kept
// from a previous one, whatever their activity: a ListView heads a section only
// over rows grouped together, and a kept conversation with no messages counts
// as active now.
class ConversationOrderProxy : public QSortFilterProxyModel
{
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

protected:
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override
    {
        const bool leftKept = left.data(ConversationListModel::HistoryOnlyRole).toBool();
        const bool rightKept = right.data(ConversationListModel::HistoryOnlyRole).toBool();
        if (leftKept != rightKept)
            return leftKept;
        return QSortFilterProxyModel::lessThan(left, right);
    }
};

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
    , m_names(new NameBook(this))
    , m_conversationModel(new ConversationListModel(*m_names, this))
    , m_conversationProxy(new ConversationOrderProxy(this))
    , m_messageModel(new MessageListModel(*m_names, this))
    , m_memberModel(new MemberListModel(*m_names, this))
{
    // Present conversations newest-first without disturbing the source's
    // insertion order; the proxy re-sorts live as last_activity changes.
    m_conversationProxy->setSourceModel(m_conversationModel);
    m_conversationProxy->setSortRole(ConversationListModel::LastActivityRole);
    m_conversationProxy->setDynamicSortFilter(true);
    m_conversationProxy->sort(0, Qt::DescendingOrder);

    setChatStatus(ChatBackendSimpleSource::Stopped);
    setDeliveryAdopted(false);
    setDeliveryPreset(requestedDeliveryPreset());
    setMyAddress(QString());
    setMyLabel(QString());
    setMyInitials(QString());
    setMyAccountName(QString());
    setCurrentConversationId(QString());
    setLoadedConversationId(QString());
    setLogDir(QString());
    setDebugData({});
    setDebugDataShown(false);
    syncCurrentConversationMeta();
    connect(this, &ChatBackendSimpleSource::debugDataShownChanged, this, [this](bool shown) {
        if (shown)
            refreshDebugData();
    });
    // The models follow the table themselves; what is derived from it here
    // follows it too.
    connect(m_names, &NameBook::changed, this, [this] {
        setMyAccountName(m_names->name(myAddress()));
        syncCurrentConversationMeta();
    });

    // As early as this plugin can reach: the QML engine has not loaded the view
    // yet, so its warnings are caught too. The lines are held in memory until
    // openRunLogs finds somewhere to put them.
    ProcessLog::install();
}

void ChatBackend::onContextReady()
{
    // Fires after the framework has wired modules(), so the typed chat_module
    // surface is live and the QtRO source is registered — the right point to
    // initialise the module and arm event subscriptions.
    initialiseModule();
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

    ChatModule::ChatConfig config;
    config.delivery_preset = deliveryPreset();
    config.log_level = QString::fromLatin1(kChatLogLevel);
    const LogosResult res = modules().chat_module.init(config);
    if (!res.success) {
        const QString reason = res.getError<QString>();
        setChatStatus(ChatBackendSimpleSource::Error);
        reportFailure(QStringLiteral("Failed to initialise chat"), reason);
        return;
    }

    m_moduleInitialised = true;

    // After init, because that is when the module opens the file it names.
    openRunLogs();

    // The address is settled once init succeeds, so the view shows it while
    // delivery is still coming up.
    refreshMyAddress();

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
    const ChatModule::Status status = modules().chat_module.status();
    applyDeliveryState(status.delivery_state, status.detail, status.delivery_adopted);

    startHealthProbe();
}

bool ChatBackend::deliveryRunning() const
{
    return chatStatus() == ChatBackendSimpleSource::Online
        || chatStatus() == ChatBackendSimpleSource::Offline;
}

void ChatBackend::startHealthProbe()
{
    m_healthProbe = new QTimer(this);
    m_healthProbe->setInterval(kHealthIntervalMs);
    connect(m_healthProbe, &QTimer::timeout, this, [this] {
        // Guarded rather than captured raw: the reply arrives on a later turn of
        // the event loop, by which time this backend may be gone.
        QPointer<ChatBackend> self(this);
        modules().chat_module.healthAsyncResult([self](const logos::AsyncResult<bool>& answer) {
            if (self)
                self->onHealthAnswer(answer.ok(), answer.error.code == "object_unavailable");
        }, Timeout(kHealthTimeoutMs));
    });
    m_healthProbe->start();
}

void ChatBackend::onHealthAnswer(bool answered, bool unreachable)
{
    const bool gone = m_healthMisses >= kHealthMissesBeforeGone;
    if (answered) {
        m_healthMisses = 0;
        // It was only busy past the probes, on a slow registry lookup say, and
        // its events kept arriving meanwhile: reading its state back is the
        // whole recovery. Deferred, since status() is a synchronous read.
        if (gone) {
            deferToEventLoop([this] {
                const ChatModule::Status status = modules().chat_module.status();
                applyDeliveryState(status.delivery_state, status.detail, status.delivery_adopted);
            });
        }
        return;
    }
    if (!gone && ++m_healthMisses < kHealthMissesBeforeGone)
        return;

    // A busy module answers again, while an unreachable one has no process left
    // to answer: asking it again would spend the acquire timeout on every tick
    // for the rest of the run.
    if (unreachable)
        m_healthProbe->stop();
    if (gone)
        return;

    setChatStatus(ChatBackendSimpleSource::Error);
    report(QStringLiteral("The chat module stopped responding and has probably crashed. "
                          "Its log for this run is where the reason will be."));
}

void ChatBackend::openRunLogs()
{
    m_moduleLogPath = modules().chat_module.get_log_path();
    if (m_moduleLogPath.isEmpty()) {
        report(QStringLiteral("Failed to open this run's logs: the chat module opened none"));
        return;
    }

    // The chat module's instance directory, borrowed: the platform assigns a view
    // module none of its own, and picking one would mean two instances writing
    // into the same folder. The two writers' runs stay apart because each list is
    // grouped by the stem of the file its own writer announced.
    // TODO: write into this view's own instance directory once
    // LogosUiPluginContext hands one over.
    const QString directory = QFileInfo(m_moduleLogPath).absolutePath();
    setLogDir(directory);

    if (ProcessLog::openIn(directory))
        m_viewLogPath = ProcessLog::path();
    else
        report(QStringLiteral("Failed to open this view's log: cannot write to ") + directory);

    refreshSessionLogs();
}

void ChatBackend::report(const QString& message)
{
    // A reason can come from a server's response body, line breaks and all.
    const QString line = message.simplified();
    m_errors.add(line, QDateTime::currentDateTime());
    setErrors(m_errors.published());
    // Through Qt's logging, so the line reaches this run's log by the same route
    // and in the same order as everything else the view writes.
    qWarning().noquote() << "chat_ui:" << line;
    emit error(line);
}

void ChatBackend::reportFailure(const QString& action, const QString& reason)
{
    report(reason.isEmpty() ? action + QStringLiteral(": the chat module gave no reason")
                            : action + QStringLiteral(": ") + reason);
}

void ChatBackend::subscribeToEvents()
{
    auto& chat = modules().chat_module;
    chat.on(QStringLiteral("message_received"),
            [this](const QVariantList& a) { applyMessageReceived(a); });
    chat.on(QStringLiteral("message_sent"),
            [this](const QVariantList& a) { applyMessageSent(a); });
    chat.on(QStringLiteral("message_delivered"),
            [this](const QVariantList& a) { applyMessageDelivered(a); });
    chat.on(QStringLiteral("conversation_created"),
            [this](const QVariantList& a) { applyConversationCreated(a); });
    chat.on(QStringLiteral("conversation_updated"),
            [this](const QVariantList& a) { applyConversationUpdated(a); });
    chat.on(QStringLiteral("members_changed"),
            [this](const QVariantList& a) { applyMembersChanged(a); });
    chat.on(QStringLiteral("conversation_deleted"),
            [this](const QVariantList& a) { applyConversationDeleted(a); });
    chat.on(QStringLiteral("delivery_state_changed"), [this](const QVariantList& a) {
        applyDeliveryState(a.value(0).toString(), a.value(1).toString(), a.value(2).toBool());
    });
    chat.on(QStringLiteral("display_name_changed"), [this](const QVariantList& a) {
        m_names->record(a.value(0).toString(), a.value(1).toString());
    });
}

void ChatBackend::lookUpName(const QString& address)
{
    if (address.isEmpty())
        return;
    // A failed read must not stand for "no name": the name known so far stays.
    QPointer<ChatBackend> self(this);
    modules().chat_module.get_display_nameAsyncResult(
        address, [self, address](const logos::AsyncResult<QString>& answer) {
            if (!self)
                return;
            if (!answer.ok()) {
                self->reportFailure(QStringLiteral("Could not read a name"),
                                    QString::fromStdString(answer.error.message));
                return;
            }
            self->m_names->record(address, answer.value);
        });
}

void ChatBackend::rehydrateConversations()
{
    if (!m_moduleInitialised) return;

    const QList<ChatModule::Conversation> convos = modules().chat_module.list_conversations();
    // Unread counts live only here, so carry them across the rebuild.
    const auto unread = m_conversationModel->unreadCounts();
    m_conversationModel->clear();
    for (const ChatModule::Conversation& convo : convos) {
        const QString& convoId = convo.convo_id;
        if (convoId.isEmpty()) continue;
        const QString nickname = convo.nickname.value_or(QString());
        const QString name = convo.name.value_or(QString());
        const QString description = convo.description.value_or(QString());
        const QString preview = convo.preview.value_or(QString());
        const qint64 lastActivity = convo.last_activity_ms;
        const bool isGroup = convo.kind == QStringLiteral("group");
        const QString peer = convo.peer.value_or(QString());
        // Local nickname wins, then the group's shared name; with neither, the
        // model names it.
        const QString displayName = !nickname.isEmpty() ? nickname : name;
        m_conversationModel->addConversation(convoId, displayName, description,
                                             msToDateTime(lastActivity), isGroup, preview,
                                             convo.history_only);
        m_conversationModel->updateProtocol(convoId, convo.protocol.value_or(QString()));
        m_conversationModel->updatePeer(convoId, peer);
        lookUpName(peer);
    }
    m_conversationModel->restoreUnreadCounts(unread);
    // The rebuilt list may now know the current conversation's kind/name.
    syncCurrentConversationMeta();
}

void ChatBackend::refreshMyAddress()
{
    if (!m_moduleInitialised)
        return;

    const QString address = modules().chat_module.get_address();
    if (address.isEmpty()) {
        report(QStringLiteral("Failed to get your address"));
        return;
    }
    setMyAddress(address);
    setMyLabel(Identity::shortLabel(address));
    setMyInitials(Identity::initials(address));
    setMyAccountName(m_names->name(address));
    lookUpName(address);
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
    setCurrentHistoryOnly(m_conversationModel->historyOnlyFor(id));
    setCurrentProtocol(m_conversationModel->protocolFor(id));
    const QString peer = m_conversationModel->peerFor(id);
    setCurrentPeerAddress(peer);
    setCurrentPeerLabel(peer.isEmpty() ? QString() : Identity::shortLabel(peer));
    // The row's own avatar, so the header keeps the face the clicked row showed.
    const QModelIndex row = m_conversationModel->index(m_conversationModel->indexOf(id));
    setCurrentAvatarInitials(row.data(ConversationListModel::AvatarInitialsRole).toString());
    setCurrentAvatarRamp(row.data(ConversationListModel::AvatarRampRole).toInt());
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
    const QList<ChatModule::Message> msgs = modules().chat_module.get_messages(convoId, &err);
    if (!err.ok()) {
        const QString reason = QString::fromStdString(err.message);
        reportFailure(QStringLiteral("Could not load messages"), reason);
        return false;
    }

    QVector<MessageItem> rows;
    rows.reserve(msgs.size());
    QSet<QString> senders;
    for (const ChatModule::Message& msg : msgs) {
        rows.append({ msg.from_self ? QString() : msg.sender.value_or(QString()),
                      msg.content, msToDateTime(msg.timestamp_ms), msg.from_self,
                      msg.message_id.value_or(QString()), msg.delivered });
        senders.insert(rows.constLast().sender);
    }
    m_messageModel->setMessages(std::move(rows));
    for (const QString& sender : senders)
        lookUpName(sender);
    return true;
}

void ChatBackend::deferToEventLoop(std::function<void()> work)
{
    QMetaObject::invokeMethod(this, std::move(work), Qt::QueuedConnection);
}

// ── .rep slot implementations ───────────────────────────────────────────────

void ChatBackend::createConversation(QString peerAddress)
{
    if (!deliveryRunning() || !isContextReady()) {
        report(QStringLiteral("Failed to create DM: chat is not online"));
        return;
    }
    if (peerAddress.isEmpty()) {
        report(QStringLiteral("Failed to create DM: address cannot be empty"));
        return;
    }

    const LogosResult res = modules().chat_module.create_conversation(peerAddress);
    if (!res.success) {
        const QString reason = res.getError<QString>();
        reportFailure(QStringLiteral("Failed to create DM"), reason);
    }
    // The conversation_created event surfaces via the push subscription — the
    // appliers handle the UI side from there.
}

void ChatBackend::createGroupConversation(QString name, QString description)
{
    if (!deliveryRunning() || !isContextReady()) {
        report(QStringLiteral("Failed to create group: chat is not online"));
        return;
    }

    const LogosResult res = modules().chat_module.create_group_conversation(name, description);
    if (!res.success) {
        const QString reason = res.getError<QString>();
        reportFailure(QStringLiteral("Failed to create group"), reason);
    }
    // The group starts with only this member; conversation_created selects it.
    // Members are added afterwards from the members panel.
}

void ChatBackend::addGroupMembers(QString conversationId, QStringList peerAddresses)
{
    if (!deliveryRunning() || !isContextReady()) {
        report(QStringLiteral("Failed to add member: chat is not online"));
        return;
    }
    if (conversationId.isEmpty()) {
        report(QStringLiteral("Failed to add member: no conversation selected"));
        return;
    }
    if (peerAddresses.isEmpty() || peerAddresses.contains(QString())) {
        report(QStringLiteral("Failed to add member: address cannot be empty"));
        return;
    }

    const LogosResult res = modules().chat_module.add_group_members(conversationId, peerAddresses);
    if (!res.success) {
        const QString reason = res.getError<QString>();
        reportFailure(QStringLiteral("Failed to add member"), reason);
        return;
    }
    // The commit is async, so the peers join the roster as pending busy rows;
    // the members_changed event reconciles them once committed.
    if (conversationId == currentConversationId())
        refreshMembers();
}

void ChatBackend::sendMessage(QString conversationId, QString content)
{
    if (!deliveryRunning() || !isContextReady()) {
        report(QStringLiteral("Failed to send message: chat is not online"));
        return;
    }
    if (conversationId.isEmpty() || content.isEmpty()) return;

    const LogosResult res = modules().chat_module.send_message(conversationId, content);
    if (!res.success) {
        const QString reason = res.getError<QString>();
        reportFailure(QStringLiteral("Failed to send message"), reason);
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
    // Reset the roster on every switch: a conversation whose roster we can't
    // fetch right now (offline) must show empty, not the previous conversation's
    // members. refreshMembers then loads the new roster when it can, and keeps
    // the last-known one across a transient offline of the same conversation.
    // User-driven, so its synchronous read is safe here (not in an event callback).
    m_memberModel->clear();
    setMemberCount(0);
    setPendingMemberCount(0);
    refreshMembers();
    setDebugData({});
    if (debugDataShown())
        refreshDebugData();
    if (loaded)
        setLoadedConversationId(conversationId);
}

void ChatBackend::refreshMembers()
{
    const QString convoId = currentConversationId();
    if (convoId.isEmpty()) {
        m_memberModel->clear();
        setMemberCount(0);
        setPendingMemberCount(0);
        return;
    }
    if (!deliveryRunning() || !isContextReady())
        return; // can't fetch now; keep the last-known roster

    // Telling our own entry from the others needs our address; recover it here
    // if init could not.
    if (myAddress().isEmpty())
        refreshMyAddress();

    const QList<ChatModule::GroupMember> members = modules().chat_module.list_group_members(convoId);
    QVector<MemberItem> rows;
    rows.reserve(members.size());
    int committed = 0;
    for (const ChatModule::GroupMember& member : members) {
        const QString& address = member.address;
        // An empty address is the roster's "no confirmed account" signal; keep
        // it — the model renders it as "unknown_account". Only a real account
        // address can be self.
        rows.append({ address, !address.isEmpty() && address == myAddress(), member.pending });
        if (!member.pending)
            ++committed;
        lookUpName(address);
    }

    m_memberModel->setMembers(rows);
    // Committed roster size only; pending invites appear in the list and are
    // counted separately.
    setMemberCount(committed);
    setPendingMemberCount(static_cast<int>(members.size()) - committed);
}

void ChatBackend::refreshSessionLogs()
{
    QVariantList published;
    appendRuns(published, QStringLiteral("chat_ui"), m_viewLogPath);
    appendRuns(published, QStringLiteral("chat_module"), m_moduleLogPath);
    setLogRuns(published);
}

void ChatBackend::refreshDebugData()
{
    const QString convoId = currentConversationId();
    // A conversation from a previous session has no live group to ask.
    if (convoId.isEmpty() || currentHistoryOnly() || !m_moduleInitialised) {
        setDebugData({});
        return;
    }
    const LogosResult res = modules().chat_module.get_debug_data(convoId);
    if (!res.success) {
        reportFailure(QStringLiteral("Failed to read the debug data"), res.error.toString());
        return;
    }
    setDebugData(debugDataView(res.value.toMap(), myAddress()));
}

void ChatBackend::refreshDebugDataAfter(const QString& convoId)
{
    if (!debugDataShown())
        return;
    deferToEventLoop([this, convoId] {
        if (convoId == currentConversationId())
            refreshDebugData();
    });
}

// ── event handlers ────────────────────────────────────────────────────────────

void ChatBackend::applyDeliveryState(const QString& state, const QString& detail, bool adopted)
{
    ChatBackendSimpleSource::ChatStatus next = ChatBackendSimpleSource::Stopped;
    if (state == QStringLiteral("online")) {
        next = ChatBackendSimpleSource::Online;
    } else if (state == QStringLiteral("initialising")) {
        next = ChatBackendSimpleSource::Initialising;
    } else if (state == QStringLiteral("offline")) {
        next = ChatBackendSimpleSource::Offline;
    } else if (state == QStringLiteral("error")) {
        next = ChatBackendSimpleSource::Error;
    } else if (state == QStringLiteral("stopped")) {
        next = ChatBackendSimpleSource::Stopped;
    } else {
        return;
    }

    const bool becameOnline =
        next == ChatBackendSimpleSource::Online && chatStatus() != ChatBackendSimpleSource::Online;
    const bool becameError =
        next == ChatBackendSimpleSource::Error && chatStatus() != ChatBackendSimpleSource::Error;

    // Before the status, so a view reacting to Online reads the node it is on.
    setDeliveryAdopted(adopted);
    setChatStatus(next);

    // The connectivity label says only that delivery is in error; what went
    // wrong reaches the user here or not at all.
    if (becameError)
        report(detail.isEmpty() ? QStringLiteral("Delivery error")
                                : QStringLiteral("Delivery error: ") + detail);

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
        m_conversationModel->addConversation(convoId, QString(), QString(), when, false, preview, false);
        deferToEventLoop([this] { rehydrateConversations(); });
    } else {
        m_conversationModel->updateLastActivity(convoId, when);
    }
    m_conversationModel->updatePreview(convoId, preview);

    if (convoId == currentConversationId()) {
        m_messageModel->addMessage(sender, content, when, false);
        // Deferred, as every module call from inside an event callback is.
        deferToEventLoop([this, sender] { lookUpName(sender); });
        // A message from someone not yet on the roster means the group grew;
        // refetch (deferred: sync module read from inside an event callback).
        if (!sender.isEmpty() && !m_memberModel->contains(sender))
            deferToEventLoop([this] { refreshMembers(); });
        refreshDebugDataAfter(convoId);
    } else {
        m_conversationModel->incrementUnread(convoId);
    }
}

void ChatBackend::applyMessageSent(const QVariantList& args)
{
    const QString convoId = args.value(0).toString();
    if (convoId.isEmpty()) return;
    const QString content = args.value(1).toString();
    const qint64 ts = args.value(2).toLongLong();
    const QString messageId = args.value(3).toString();
    const QDateTime when = msToDateTime(ts);
    const QString preview = content.left(kPreviewMaxChars);

    if (!m_conversationModel->contains(convoId)) {
        m_conversationModel->addConversation(convoId, QString(), QString(), when, false, preview, false);
    } else {
        m_conversationModel->updateLastActivity(convoId, when);
    }
    m_conversationModel->updatePreview(convoId, preview);

    if (convoId == currentConversationId()) {
        m_messageModel->addMessage(QString(), content, when, true, messageId);
        refreshDebugDataAfter(convoId);
    }
}

void ChatBackend::applyMessageDelivered(const QVariantList& args)
{
    // A conversation not on screen reads the flag back when it is opened.
    if (args.value(0).toString() == currentConversationId())
        m_messageModel->markDelivered(args.value(1).toString());
}

void ChatBackend::applyConversationCreated(const QVariantList& args)
{
    const QString convoId = args.value(0).toString();
    if (convoId.isEmpty()) return;
    const bool isOutgoing = args.value(1).toBool();
    const bool isGroup = args.value(3).toString() == QStringLiteral("group");
    const QString name = args.value(4).toString();
    const QString description = args.value(5).toString();
    const QDateTime now = QDateTime::currentDateTime();

    if (!m_conversationModel->contains(convoId)) {
        m_conversationModel->addConversation(convoId, name, description, now, isGroup, QString(), false);
    } else {
        m_conversationModel->updateDisplayName(convoId, name);
        m_conversationModel->updateDescription(convoId, description);
        m_conversationModel->updateLastActivity(convoId, now);
    }
    // The event names no protocol; the list does. Deferred: this
    // runs inside a module event callback (see deferToEventLoop).
    deferToEventLoop([this] { rehydrateConversations(); });

    // Open a conversation we just created, so creating a chat or group lands
    // the user in it. Deferred: selectConversation makes a synchronous module
    // read and this runs inside a module event callback (see deferToEventLoop).
    if (isOutgoing) {
        deferToEventLoop([this, convoId] { selectConversation(convoId); });
    } else if (convoId != currentConversationId()) {
        // Being invited comes with no message of its own, so the unread badge is
        // the only thing marking the new row as unseen.
        m_conversationModel->markInvited(convoId);
    }
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
    refreshDebugDataAfter(convoId);
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
        // The roster goes with the conversation. Reached with no current
        // conversation, refreshMembers clears it without a module read.
        refreshMembers();
        setDebugData({});
    }
}

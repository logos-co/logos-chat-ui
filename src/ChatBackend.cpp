#include "ChatBackend.h"
#include <QDebug>
#include <QDateTime>
#include <QRandomGenerator>
#include <QStringList>
#include <QCoreApplication>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
    // 100 adjectives × 100 nouns = 10,000 unique combinations
    const QStringList adjectives = {
        "Swift", "Happy", "Cosmic", "Brave", "Clever", "Mighty", "Silent", "Radiant",
        "Noble", "Mystic", "Golden", "Stellar", "Quantum", "Electric", "Nimble", "Vivid",
        "Zen", "Epic", "Lucky", "Bold", "Calm", "Eager", "Fierce", "Gentle",
        "Jolly", "Keen", "Lively", "Merry", "Quick", "Wise", "Zesty", "Bright",
        "Daring", "Witty", "Smooth", "Prime", "Royal", "Primal", "Astral", "Blazing",
        "Cryptic", "Divine", "Frosty", "Gleaming", "Hyper", "Infinite", "Jade", "Kinetic",
        "Lunar", "Neon", "Omega", "Phantom", "Rising", "Shadow", "Turbo", "Ultra",
        "Velvet", "Warp", "Apex", "Binary", "Cyber", "Delta", "Flux", "Glitch",
        "Holo", "Iron", "Jazz", "Karma", "Logic", "Matrix", "Nova", "Onyx",
        "Pixel", "Quest", "Retro", "Sonic", "Terra", "Unity", "Vortex", "Xenon",
        "Alpha", "Beta", "Gamma", "Sigma", "Theta", "Zeta", "Plasma", "Prism",
        "Rapid", "Solar", "Storm", "Titan", "Vapor", "Zero", "Atomic", "Blaze",
        "Chrome", "Drift", "Ember", "Flash"
    };

    const QStringList nouns = {
        "Panda", "Phoenix", "Falcon", "Wolf", "Dragon", "Tiger", "Owl", "Dolphin",
        "Raven", "Fox", "Lion", "Bear", "Hawk", "Lynx", "Otter", "Jaguar",
        "Koala", "Penguin", "Badger", "Heron", "Viper", "Crane", "Gecko", "Bison",
        "Lemur", "Manta", "Cobra", "Ibis", "Puma", "Newt", "Coyote", "Osprey",
        "Shark", "Eagle", "Panther", "Rogue", "Ninja", "Samurai", "Knight", "Wizard",
        "Ranger", "Hunter", "Pilot", "Voyager", "Scout", "Rider", "Seeker", "Walker",
        "Runner", "Glider", "Diver", "Surfer", "Hacker", "Coder", "Maker", "Builder",
        "Crafter", "Weaver", "Keeper", "Warden", "Guard", "Sage", "Oracle", "Mystic",
        "Spirit", "Specter", "Wraith", "Shade", "Ghost", "Sprite", "Nymph", "Titan",
        "Giant", "Golem", "Sphinx", "Griffin", "Hydra", "Kraken", "Leviathan", "Chimera",
        "Mantis", "Hornet", "Beetle", "Spider", "Scorpion", "Raptor", "Condor", "Pelican",
        "Walrus", "Moose", "Elk", "Stag", "Boar", "Rhino", "Hippo", "Gator",
        "Turtle", "Parrot", "Toucan", "Finch"
    };

    QString generateFunUsername() {
        auto* rng = QRandomGenerator::global();
        QString adj = adjectives[rng->bounded(adjectives.size())];
        QString noun = nouns[rng->bounded(nouns.size())];
        return adj + noun;
    }
}

ChatBackend::ChatBackend(LogosAPI* logosAPI, QObject* parent)
    : QObject(parent),
      m_status(NotInitialized),
      m_logosAPI(nullptr),
      m_logos(nullptr),
      m_currentChannel(""),
      m_mixnodePoolSize(0),
      m_lightpushPeersCount(0),
      m_metricsTimer(nullptr),
      m_metricsSlowMode(false),
      m_discoveryMode(ExtKadOnly),
      m_bootstrapNodes(),
      m_storeNode()
{
    qDebug() << "Initializing ChatBackend...";

    // Load settings before initializing
    loadSettings();

    if (logosAPI) {
        m_logosAPI = logosAPI;
    } else {
        m_logosAPI = new LogosAPI("core", this);
    }

    m_logos = new LogosModules(m_logosAPI);

    // Generate a fresh random username for each session
    m_username = generateFunUsername();
    qDebug() << "Generated username:" << m_username;

    // Setup metrics polling timer (starts at 5s, slows to 30s once 3+ mixnodes discovered)
    m_metricsTimer = new QTimer(this);
    connect(m_metricsTimer, &QTimer::timeout, this, &ChatBackend::refreshNetworkMetrics);

    initializeChat();
}

ChatBackend::~ChatBackend()
{
    qDebug() << "Destroying ChatBackend...";
    if (m_metricsTimer) {
        m_metricsTimer->stop();
    }
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

    // Subscribe to network metrics events
    if (!m_logos->chat.on("mixnodePoolSizeResponse", [this](const QVariantList& data) {
            onMixnodePoolSizeResponse(data);
        })) {
        qWarning() << "ChatBackend: failed to subscribe to mixnodePoolSizeResponse events";
    }

    if (!m_logos->chat.on("lightpushPeersCountResponse", [this](const QVariantList& data) {
            onLightpushPeersCountResponse(data);
        })) {
        qWarning() << "ChatBackend: failed to subscribe to lightpushPeersCountResponse events";
    }

    // Build JSON configuration for the chat module
    QStringList addressesList;
    QStringList mixnodesList;
    for (const QVariant& node : m_bootstrapNodes) {
        QVariantMap nodeMap = node.toMap();
        QString address = nodeMap["address"].toString();
        QString mixPubKey = nodeMap["mixPubKey"].toString();
        addressesList.append(address);
        if (!mixPubKey.isEmpty()) {
            mixnodesList.append(address + ":" + mixPubKey);
        }
    }

    QJsonObject config;
    config["mode"] = static_cast<int>(m_discoveryMode);
    config["bootstrapNodes"] = addressesList.join(",");
    config["mixnodes"] = mixnodesList.join(",");
    config["storeNode"] = m_storeNode;
    config["nodeKey"] = m_nodeKey;

    QString configJson = QString::fromUtf8(QJsonDocument(config).toJson(QJsonDocument::Compact));

    bool success = m_logos->chat.initialize(configJson);

    if (!success) {
        setStatus(Error);
        return;
    }

    setStatus(Ready);

    // Start the metrics polling timer (5 seconds initial interval)
    m_metricsTimer->start(5000);

    // Initial metrics fetch
    refreshNetworkMetrics();

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

void ChatBackend::onMixnodePoolSizeResponse(const QVariantList& data)
{
    if (data.isEmpty()) {
        qWarning() << "ChatBackend::onMixnodePoolSizeResponse: Empty data";
        return;
    }

    bool ok;
    int size = data[0].toString().toInt(&ok);
    if (ok && size != m_mixnodePoolSize) {
        m_mixnodePoolSize = size;
        emit mixnodePoolSizeChanged();
        qDebug() << "ChatBackend: Mixnode pool size updated to" << m_mixnodePoolSize;
    }

    // Once we have 3+ mixnodes, slow down polling to reduce log noise
    if (ok && size >= 3 && !m_metricsSlowMode) {
        m_metricsSlowMode = true;
        m_metricsTimer->setInterval(30000);
        qDebug() << "ChatBackend: Discovered" << size << "mixnodes, switching to 30s polling";
    }
}

void ChatBackend::onLightpushPeersCountResponse(const QVariantList& data)
{
    if (data.isEmpty()) {
        qWarning() << "ChatBackend::onLightpushPeersCountResponse: Empty data";
        return;
    }

    bool ok;
    int count = data[0].toString().toInt(&ok);
    if (ok && count != m_lightpushPeersCount) {
        m_lightpushPeersCount = count;
        emit lightpushPeersCountChanged();
        qDebug() << "ChatBackend: Lightpush peers count updated to" << m_lightpushPeersCount;
    }
}

void ChatBackend::refreshNetworkMetrics()
{
    if (m_status != Ready) {
        return;
    }

    m_logos->chat.getMixnodePoolSize();
    m_logos->chat.getLightpushPeersCount();
}

QVariantList ChatBackend::getDefaultBootstrapNodes() const
{
    QVariantList nodes;

    // Default bootstrap nodes with their mixnode public keys
    QVariantMap node1;
    node1["address"] = "/dns4/delivery-01.do-ams3.logos.dev.status.im/tcp/30303/p2p/16Uiu2HAmTUbnxLGT9JvV6mu9oPyDjqHK4Phs1VDJNUgESgNSkuby";
    node1["mixPubKey"] = "c288a425a6209c74ec07e2e8b6816e9b6995d1cd59b1ab482317c3dfb3ba200f";
    nodes.append(node1);

    QVariantMap node2;
    node2["address"] = "/dns4/delivery-02.do-ams3.logos.dev.status.im/tcp/30303/p2p/16Uiu2HAmMK7PYygBtKUQ8EHp7EfaD3bCEsJrkFooK8RQ2PVpJprH";
    node2["mixPubKey"] = "9d92279057940efd2e5e98c8922c079c24e45c083b00360c8dc6a298b1661716";
    nodes.append(node2);

    QVariantMap node3;
    node3["address"] = "/dns4/delivery-01.gc-us-central1-a.logos.dev.status.im/tcp/30303/p2p/16Uiu2HAm4S1JYkuzDKLKQvwgAhZKs9otxXqt8SCGtB4hoJP1S397";
    node3["mixPubKey"] = "fe60e95c50f70db9015525064e1fff962ccc982dde480f8faae30262710ece58";
    nodes.append(node3);

    QVariantMap node4;
    node4["address"] = "/dns4/delivery-02.gc-us-central1-a.logos.dev.status.im/tcp/30303/p2p/16Uiu2HAm8Y9kgBNtjxvCnf1X6gnZJW5EGE4UwwCL3CCm55TwqBiH";
    node4["mixPubKey"] = "312335324231ba7963c0c7524e042d1beac2927dbf810513a7fc8d901ab4e812";
    nodes.append(node4);

    QVariantMap node5;
    node5["address"] = "/dns4/delivery-01.ac-cn-hongkong-c.logos.dev.status.im/tcp/30303/p2p/16Uiu2HAm8YokiNun9BkeA1ZRmhLbtNUvcwRr64F69tYj9fkGyuEP";
    node5["mixPubKey"] = "7d683767f23f5132a79c70587fec877575460122ebd459bb29c887b7b7a32110";
    nodes.append(node5);

    QVariantMap node6;
    node6["address"] = "/dns4/delivery-02.ac-cn-hongkong-c.logos.dev.status.im/tcp/30303/p2p/16Uiu2HAkvwhGHKNry6LACrB8TmEFoCJKEX29XR5dDUzk3UT3UNSE";
    node6["mixPubKey"] = "0894b2852890d244e045f2ff5875e03a6b18f233ccd2e5297f62f7546e93884d";
    nodes.append(node6);

    return nodes;
}

QString ChatBackend::getDefaultStoreNode() const
{
    return "/dns4/delivery-01.do-ams3.logos.dev.status.im/tcp/30303/p2p/16Uiu2HAmTUbnxLGT9JvV6mu9oPyDjqHK4Phs1VDJNUgESgNSkuby";
}

static QString generateNodeKey()
{
    QString hex;
    hex.reserve(64);
    auto* rng = QRandomGenerator::global();
    for (int i = 0; i < 8; ++i) {
        hex += QString::number(rng->generate(), 16).rightJustified(8, '0');
    }
    return hex;
}

void ChatBackend::loadSettings()
{
    QSettings settings("Logos", "ChatUI");

    // Persist nodeKey across sessions so our libp2p identity stays stable
    if (settings.contains("nodeKey")) {
        m_nodeKey = settings.value("nodeKey").toString();
    } else {
        m_nodeKey = generateNodeKey();
        settings.setValue("nodeKey", m_nodeKey);
        settings.sync();
        qDebug() << "ChatBackend: Generated new nodeKey";
    }

    m_discoveryMode = ExtKadOnly;
    m_bootstrapNodes = getDefaultBootstrapNodes();
    m_storeNode = getDefaultStoreNode();

    qDebug() << "ChatBackend: Loaded settings - nodeKey present, discoveryMode:" << m_discoveryMode
             << ", bootstrapNodes count:" << m_bootstrapNodes.size()
             << ", storeNode:" << m_storeNode;
}

void ChatBackend::saveSettings()
{
    QSettings settings("Logos", "ChatUI");

    settings.setValue("nodeKey", m_nodeKey);
    settings.setValue("discoveryMode", static_cast<int>(m_discoveryMode));
    settings.setValue("bootstrapNodes", m_bootstrapNodes);
    settings.setValue("storeNode", m_storeNode);
    settings.sync();

    qDebug() << "ChatBackend: Saved settings - discoveryMode:" << m_discoveryMode
             << ", bootstrapNodes count:" << m_bootstrapNodes.size()
             << ", storeNode:" << m_storeNode;
}

void ChatBackend::setDiscoveryMode(DiscoveryMode mode)
{
    if (m_discoveryMode != mode) {
        m_discoveryMode = mode;
        emit discoveryModeChanged();
        qDebug() << "ChatBackend: Discovery mode changed to" << m_discoveryMode;
    }
}

void ChatBackend::setStoreNode(const QString& storeNode)
{
    if (m_storeNode != storeNode) {
        m_storeNode = storeNode;
        emit storeNodeChanged();
        qDebug() << "ChatBackend: Store node changed to" << m_storeNode;
    }
}

void ChatBackend::addBootstrapNode(const QString& address, const QString& mixPubKey)
{
    if (address.trimmed().isEmpty()) {
        return;
    }

    // Check if address already exists
    for (const QVariant& node : m_bootstrapNodes) {
        QVariantMap nodeMap = node.toMap();
        if (nodeMap["address"].toString() == address.trimmed()) {
            qDebug() << "ChatBackend: Bootstrap node already exists:" << address;
            return;
        }
    }

    QVariantMap newNode;
    newNode["address"] = address.trimmed();
    newNode["mixPubKey"] = mixPubKey.trimmed();
    m_bootstrapNodes.append(newNode);
    emit bootstrapNodesChanged();
    qDebug() << "ChatBackend: Added bootstrap node:" << address << "with mixPubKey:" << mixPubKey;
}

void ChatBackend::updateBootstrapNodeMixKey(int index, const QString& mixPubKey)
{
    if (index >= 0 && index < m_bootstrapNodes.size()) {
        QVariantMap nodeMap = m_bootstrapNodes[index].toMap();
        nodeMap["mixPubKey"] = mixPubKey.trimmed();
        m_bootstrapNodes[index] = nodeMap;
        emit bootstrapNodesChanged();
        qDebug() << "ChatBackend: Updated mixPubKey for node at index" << index;
    }
}

void ChatBackend::removeBootstrapNode(int index)
{
    if (index >= 0 && index < m_bootstrapNodes.size()) {
        QVariantMap removed = m_bootstrapNodes.takeAt(index).toMap();
        emit bootstrapNodesChanged();
        qDebug() << "ChatBackend: Removed bootstrap node:" << removed["address"].toString();
    }
}

void ChatBackend::requestRestart()
{
    qDebug() << "ChatBackend: Restart requested";
    emit restartRequested();
}

void ChatBackend::resetPeerId()
{
    m_nodeKey = generateNodeKey();
    QSettings settings("Logos", "ChatUI");
    settings.setValue("nodeKey", m_nodeKey);
    settings.sync();
    qDebug() << "ChatBackend: Peer ID reset, new nodeKey generated";
    emit restartRequested();
}

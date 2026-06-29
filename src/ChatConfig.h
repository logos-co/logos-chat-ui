#pragma once

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRandomGenerator>
#include <QFile>
#include <QDir>
#include <cstdlib>

/**
 * Configuration for the Chat/Waku connection.
 * 
 * Defaults can be overridden via environment variables:
 *   - CHAT_NAME: Identity name (default: "LogosUser")
 *   - CHAT_PORT: Waku port, 0 for random (default: 0)
 *   - CHAT_CLUSTER_ID: Waku cluster ID (default: 2)
 *   - CHAT_SHARD_ID: Waku shard ID (default: 1)
 *   - CHAT_STATIC_PEER: Static peer multiaddr (optional)
 * 
 * Configuration values from libchat.h:
 *   configJson: JSON object with fields:
 *     - "name": string - identity name (default: "anonymous")
 *     - "port": int - Waku port (optional)
 *     - "clusterId": int - Waku cluster ID (optional)
 *     - "shardId": int - Waku shard ID (optional)
 *     - "staticPeers": array of strings - static peer multiaddrs (optional)
 */
namespace ChatConfig {

// Default values - can be changed here for different deployments
constexpr int DEFAULT_PORT = 0;           // 0 = random port
constexpr int DEFAULT_CLUSTER_ID = 2;    // Waku cluster ID
constexpr int DEFAULT_SHARD_ID = 1;       // Waku shard ID

inline QString defaultName() {
    // Generate a random suffix for the default name
    return QString("LogosUser_%1").arg(QRandomGenerator::global()->bounded(1000), 3, 10, QChar('0'));
}

/**
 * Get configuration value from environment or use default
 */
inline QString getEnvOrDefault(const char* envName, const QString& defaultValue) {
    const char* envValue = std::getenv(envName);
    return envValue ? QString::fromUtf8(envValue) : defaultValue;
}

inline int getEnvOrDefault(const char* envName, int defaultValue) {
    const char* envValue = std::getenv(envName);
    if (envValue) {
        bool ok;
        int value = QString::fromUtf8(envValue).toInt(&ok);
        if (ok) return value;
    }
    return defaultValue;
}

/**
 * testnet-0.2 fleet entry points: the kad-DHT bootstrap seed and the default
 * filter/receive peers. These are public node addresses, NOT credentials. Two
 * healthy nodes are enough — the rest of the mix fleet is discovered over the DHT
 * (a single bootstrap reaches the whole 5-node pool in ~6s). Override at runtime
 * with CHAT_KAD_BOOTSTRAP / CHAT_STATIC_PEER.
 */
inline QStringList defaultFleetEntryNodes() {
    return {
        QStringLiteral("/dns4/node-01.ih-eu-mda1.misc.vaclab.status.im/tcp/30304/"
                       "p2p/16Uiu2HAm8PDGahpTZ86SKxBqFodPVxpGonXLucUR9bscFWxqJuZr"),
        QStringLiteral("/dns4/node-03.ih-eu-mda1.misc.vaclab.status.im/tcp/30304/"
                       "p2p/16Uiu2HAmMgeAACqTTEKVuyBmbtyAqg6qznevmyF5k6qRcL6eXsqS"),
    };
}

/**
 * Stage the selected demo-user's RLN credential so the mix-RLN plugin can load it.
 *
 * Reads the chosen membership keystore + the shared rln_tree.db from CHAT_CREDS_DIR
 * (dev override) or the embedded :/fleet-creds resource, copies them into the process
 * cwd (where the plugin looks them up), and points CHAT_RLN_KEYSTORE at the membership.
 * A fresh RANDOM peerId is used (identity decoupled from membership). Network entry
 * points (kad bootstrap + filter peers) are NOT creds and are handled by buildConfigJson.
 * Returns false (no-op) if index <= 0 or no creds source is available.
 */
inline bool stageDemoUserCreds(int index) {
    if (index <= 0) return false;
    const QString cwd = QDir::currentPath();
    const QString envDir = getEnvOrDefault("CHAT_CREDS_DIR", QString());

    // Pick the creds source: an explicit CHAT_CREDS_DIR (dev override) wins; otherwise
    // fall back to the creds embedded in the app (Qt resource ":/fleet-creds"), so the
    // app is self-contained with no manual setup. QFile reads ":/..." resources and
    // filesystem paths transparently.
    QString credsRoot;     // dir holding rln_tree.db + users/userN/keystore
    QString keystoreSrc;   // the chosen user's keystore file
    bool fromResource = false;
    if (!envDir.isEmpty()) {
        credsRoot = envDir;
        const QString userDir = envDir + "/users/user" + QString::number(index);
        const QStringList keystores =
            QDir(userDir).entryList(QStringList() << "rln_keystore_*.json", QDir::Files);
        if (keystores.isEmpty()) {
            qWarning("stageDemoUserCreds: no keystore in %s", qPrintable(userDir));
            return false;
        }
        keystoreSrc = userDir + "/" + keystores.first();
    }
#ifdef EMBEDDED_CREDS
    else {
        credsRoot = QStringLiteral(":/fleet-creds");
        keystoreSrc = QStringLiteral(":/fleet-creds/users/user%1/keystore.json").arg(index);
        fromResource = true;
        if (!QFile::exists(keystoreSrc)) {
            qWarning("stageDemoUserCreds: embedded keystore missing for user %d", index);
            return false;
        }
    }
#endif
    if (credsRoot.isEmpty()) {
        qWarning("stageDemoUserCreds: no creds source (set CHAT_CREDS_DIR or build with embedded creds)");
        return false;
    }

    // Stage the chosen membership keystore (peerId-agnostic source) + the shared tree
    // into cwd, where mountMix loads them. logos-chat copies the keystore to
    // rln_keystore_<itsOwnRandomPeerId>.json before mountMix, so the libp2p identity
    // stays UNIQUE while the RLN membership is the chosen one (instances can share a
    // membership without sharing a peerId).
    const QString memb = cwd + "/rln_membership.json";
    const QString tree = cwd + "/rln_tree.db";
    QFile::remove(memb);
    QFile::remove(tree);
    QFile::copy(keystoreSrc, memb);
    QFile::copy(credsRoot + "/rln_tree.db", tree);
    if (fromResource) {
        // Files copied out of a Qt resource inherit read-only perms; make them
        // writable so the RLN keystore/tree can be opened/locked normally.
        QFile::setPermissions(memb, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        QFile::setPermissions(tree, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    }

    // Point the mix-RLN plugin at the chosen membership. CHAT_NODEKEY is deliberately
    // NOT set -> the node gets a fresh RANDOM peerId (identity decoupled from membership).
    // Network entry points (kad bootstrap + filter/receive peers) are NOT credentials;
    // buildConfigJson supplies them from the fleet defaults (override via
    // CHAT_KAD_BOOTSTRAP / CHAT_STATIC_PEER), so this routine only stages creds.
    qputenv("CHAT_RLN_KEYSTORE", memb.toUtf8());
    qInfo("stageDemoUserCreds: demo user %d -> membership staged (%s, random peerId)",
          index, fromResource ? "embedded" : "CHAT_CREDS_DIR");
    return true;
}

/**
 * Build the configuration JSON string for chat_new()
 * 
 * @param name Optional override for identity name
 * @param port Optional override for port
 * @param clusterId Optional override for cluster ID
 * @param shardId Optional override for shard ID
 * @param staticPeer Optional static peer multiaddr
 */
inline QString buildConfigJson(
    const QString& name = QString(),
    int port = -1,
    int clusterId = -1,
    int shardId = -1,
    const QString& staticPeer = QString(),
    bool mixEnabled = false)
{
    QJsonObject config;
    
    // Name - use parameter, then env, then default
    if (!name.isEmpty()) {
        config["name"] = name;
    } else {
        config["name"] = getEnvOrDefault("CHAT_NAME", defaultName());
    }
    
    // Port - use parameter, then env, then default
    if (port >= 0) {
        config["port"] = port;
    } else {
        config["port"] = getEnvOrDefault("CHAT_PORT", DEFAULT_PORT);
    }
    
    // Cluster ID - use parameter, then env, then default
    if (clusterId >= 0) {
        config["clusterId"] = clusterId;
    } else {
        config["clusterId"] = getEnvOrDefault("CHAT_CLUSTER_ID", DEFAULT_CLUSTER_ID);
    }
    
    // Shard ID - use parameter, then env, then default
    if (shardId >= 0) {
        config["shardId"] = shardId;
    } else {
        config["shardId"] = getEnvOrDefault("CHAT_SHARD_ID", DEFAULT_SHARD_ID);
    }
    
    // Static peer(s) for the filter/receive path: the chat is a light-client and
    // receives only by filter-subscribing to these (kad discovery fills the mix pool
    // for *sending* only). Use the parameter, then CHAT_STATIC_PEER (single multiaddr
    // or comma-separated list), then the fleet entry-point defaults — 2 nodes give
    // redundancy against a flaky one without subscribing to the whole fleet.
    QStringList peerList;
    if (!staticPeer.isEmpty()) {
        peerList = staticPeer.split(',', Qt::SkipEmptyParts);
    } else {
        const QString peerEnv = getEnvOrDefault("CHAT_STATIC_PEER", QString());
        peerList = peerEnv.isEmpty() ? defaultFleetEntryNodes()
                                     : peerEnv.split(',', Qt::SkipEmptyParts);
    }
    QJsonArray staticPeers;
    for (const QString& p : peerList) {
        const QString t = p.trimmed();
        if (!t.isEmpty()) staticPeers.append(t);
    }
    if (!staticPeers.isEmpty()) {
        config["staticPeers"] = staticPeers;
    }

    // Optional fixed identity: a 64-char hex secp256k1 key. Used to adopt a
    // provisioned identity (e.g. a mix-sim chat credential) so the peer-ID-
    // derived RLN keystore resolves. Empty => random identity.
    const QString nodekey = getEnvOrDefault("CHAT_NODEKEY", QString());
    if (!nodekey.isEmpty()) {
        config["nodekey"] = nodekey;
    }

    // Mix sender-anonymity (global Required/None mode, applied at init).
    // When Required, route via the mixnet; the mix node multiaddrs come from
    // CHAT_MIX_NODES (comma-separated) and the minimum healthy pool size from
    // CHAT_MIN_MIX_POOL (default 4). logos-chat refuses to send while the pool
    // is below the minimum.
    config["mixEnabled"] = mixEnabled;
    if (mixEnabled) {
        QJsonArray mixNodes;
        const QString nodes = getEnvOrDefault("CHAT_MIX_NODES", QString());
        for (const QString& node : nodes.split(',', Qt::SkipEmptyParts)) {
            const QString trimmed = node.trimmed();
            if (!trimmed.isEmpty()) mixNodes.append(trimmed);
        }
        config["mixNodes"] = mixNodes;
        config["minMixPoolSize"] = getEnvOrDefault("CHAT_MIN_MIX_POOL", 4);

        // Fleet mode: discover mix nodes via kad service discovery instead of a static
        // CHAT_MIX_NODES list with pubkeys. The DHT is seeded from a couple of fleet
        // entry points (the fleet defaults, or CHAT_KAD_BOOTSTRAP to override);
        // logos-chat then discovers every mix node + curve25519 pubkey over the DHT, so
        // the rest of the fleet need not be listed. Leave CHAT_MIX_NODES empty.
        const QString kadNodes = getEnvOrDefault("CHAT_KAD_BOOTSTRAP", QString());
        const QStringList kadList = kadNodes.isEmpty()
            ? defaultFleetEntryNodes()
            : kadNodes.split(',', Qt::SkipEmptyParts);
        QJsonArray kadBootstrap;
        for (const QString& node : kadList) {
            const QString trimmed = node.trimmed();
            if (!trimmed.isEmpty()) kadBootstrap.append(trimmed);
        }
        if (!kadBootstrap.isEmpty()) config["kadBootstrapNodes"] = kadBootstrap;

        // Chosen RLN membership keystore (staged by stageDemoUserCreds). The node
        // loads it under its own random peerId — identity decoupled from membership.
        const QString rlnSrc = getEnvOrDefault("CHAT_RLN_KEYSTORE", QString());
        if (!rlnSrc.isEmpty()) config["rlnKeystoreSource"] = rlnSrc;
    }

    return QJsonDocument(config).toJson(QJsonDocument::Compact);
}

/**
 * Get a human-readable description of the current configuration
 */
inline QString getConfigDescription(const QString& configJson) {
    QJsonDocument doc = QJsonDocument::fromJson(configJson.toUtf8());
    if (!doc.isObject()) return "Invalid configuration";
    
    QJsonObject obj = doc.object();
    return QString("Name: %1, Port: %2, Cluster: %3, Shard: %4%5")
        .arg(obj["name"].toString())
        .arg(obj["port"].toInt())
        .arg(obj["clusterId"].toInt())
        .arg(obj["shardId"].toInt())
        .arg(obj.contains("staticPeer") 
             ? QString(", Peer: %1").arg(obj["staticPeer"].toString().left(30) + "...") 
             : "");
}

} // namespace ChatConfig

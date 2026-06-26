#pragma once

#include <QString>
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
 * Stage the selected demo-user's fleet credential so the mix-RLN plugin can load it.
 *
 * Reads CHAT_CREDS_DIR/users/userN/{nodekey.txt, rln_keystore_<peerId>.json} and the
 * shared CHAT_CREDS_DIR/{rln_tree.db, fleet_bootstrap.txt}, copies the keystore + tree
 * into the process cwd (where the plugin looks them up by peerId), and sets CHAT_NODEKEY
 * (-> fixed peerId so the keystore resolves) + CHAT_KAD_BOOTSTRAP (fleet discovery).
 * Returns false (no-op) if index <= 0 or CHAT_CREDS_DIR is unset/incomplete.
 */
inline bool stageDemoUserCreds(int index) {
    if (index <= 0) return false;
    const QString credsDir = getEnvOrDefault("CHAT_CREDS_DIR", QString());
    if (credsDir.isEmpty()) {
        qWarning("stageDemoUserCreds: CHAT_CREDS_DIR not set; cannot load demo user %d", index);
        return false;
    }
    const QString userDir = credsDir + "/users/user" + QString::number(index);
    const QString cwd = QDir::currentPath();

    // fleet kad bootstrap multiaddrs (skip comments/blanks)
    QStringList kad;
    QFile bsFile(credsDir + "/fleet_bootstrap.txt");
    if (bsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QStringList lines = QString::fromUtf8(bsFile.readAll()).split('\n');
        for (const QString& raw : lines) {
            const QString line = raw.trimmed();
            if (!line.isEmpty() && !line.startsWith('#')) kad.append(line);
        }
        bsFile.close();
    }

    // The chosen membership keystore -> staged as a peerId-agnostic source file.
    // logos-chat copies it to rln_keystore_<itsOwnRandomPeerId>.json before mountMix,
    // so the libp2p identity stays UNIQUE while the RLN membership is the chosen one
    // (multiple instances can share a membership without sharing a peerId).
    QDir uDir(userDir);
    const QStringList keystores = uDir.entryList(QStringList() << "rln_keystore_*.json", QDir::Files);
    if (keystores.isEmpty()) {
        qWarning("stageDemoUserCreds: no keystore in %s", qPrintable(userDir));
        return false;
    }
    const QString src = cwd + "/rln_membership.json";
    QFile::remove(src);
    QFile::copy(userDir + "/" + keystores.first(), src);

    // shared tree (loaded by mountMix from cwd)
    QFile::remove(cwd + "/rln_tree.db");
    QFile::copy(credsDir + "/rln_tree.db", cwd + "/rln_tree.db");

    // Feed the credential source + kad bootstrap to buildConfigJson (read from env).
    // CHAT_NODEKEY is deliberately NOT set -> the node gets a fresh RANDOM peerId.
    qputenv("CHAT_RLN_KEYSTORE", src.toUtf8());
    qputenv("CHAT_KAD_BOOTSTRAP", kad.join(',').toUtf8());
    qInfo("stageDemoUserCreds: demo user %d -> membership staged (random peerId, %lld kad bootstraps)",
          index, static_cast<long long>(kad.size()));
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
    
    // Static peer - use parameter, then env
    QString peer = staticPeer.isEmpty() 
        ? getEnvOrDefault("CHAT_STATIC_PEER", QString())
        : staticPeer;
    
    // logos-chat parses "staticPeers" (an array), so emit the plural key.
    if (!peer.isEmpty()) {
        config["staticPeers"] = QJsonArray{ peer };
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

        // Fleet mode: discover mix nodes via kad service discovery instead of a
        // static CHAT_MIX_NODES list with pubkeys. CHAT_KAD_BOOTSTRAP is the
        // comma-separated fleet bootstrap multiaddrs; logos-chat discovers the mix
        // nodes + their curve25519 pubkeys from them. Leave CHAT_MIX_NODES empty.
        QJsonArray kadBootstrap;
        const QString kadNodes = getEnvOrDefault("CHAT_KAD_BOOTSTRAP", QString());
        for (const QString& node : kadNodes.split(',', Qt::SkipEmptyParts)) {
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

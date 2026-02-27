#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QRandomGenerator>
#include <cstdlib>

/**
 * Configuration for the ChatSDK/Waku connection.
 * 
 * Defaults can be overridden via environment variables:
 *   - CHATSDK_NAME: Identity name (default: "LogosUser")
 *   - CHATSDK_PORT: Waku port, 0 for random (default: 0)
 *   - CHATSDK_CLUSTER_ID: Waku cluster ID (default: 2)
 *   - CHATSDK_SHARD_ID: Waku shard ID (default: 1)
 *   - CHATSDK_STATIC_PEER: Static peer multiaddr (optional)
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
    const QString& staticPeer = QString())
{
    QJsonObject config;
    
    // Name - use parameter, then env, then default
    if (!name.isEmpty()) {
        config["name"] = name;
    } else {
        config["name"] = getEnvOrDefault("CHATSDK_NAME", defaultName());
    }
    
    // Port - use parameter, then env, then default
    if (port >= 0) {
        config["port"] = port;
    } else {
        config["port"] = getEnvOrDefault("CHATSDK_PORT", DEFAULT_PORT);
    }
    
    // Cluster ID - use parameter, then env, then default
    if (clusterId >= 0) {
        config["clusterId"] = clusterId;
    } else {
        config["clusterId"] = getEnvOrDefault("CHATSDK_CLUSTER_ID", DEFAULT_CLUSTER_ID);
    }
    
    // Shard ID - use parameter, then env, then default
    if (shardId >= 0) {
        config["shardId"] = shardId;
    } else {
        config["shardId"] = getEnvOrDefault("CHATSDK_SHARD_ID", DEFAULT_SHARD_ID);
    }
    
    // Static peer - use parameter, then env
    QString peer = staticPeer.isEmpty() 
        ? getEnvOrDefault("CHATSDK_STATIC_PEER", QString())
        : staticPeer;
    
    if (!peer.isEmpty()) {
        config["staticPeer"] = peer;
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

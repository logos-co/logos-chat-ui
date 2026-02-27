#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QSettings>
#include "logos_api.h"
#include "logos_api_client.h"
#include "logos_sdk.h"

class ChatBackend : public QObject {
    Q_OBJECT

public:
    enum ChatStatus {
        NotInitialized = 0,
        Initializing,
        Ready,
        Error
    };
    Q_ENUM(ChatStatus)

    enum DiscoveryMode {
        ExtKadOnly = 0,
        StdDiscovery,
        All
    };
    Q_ENUM(DiscoveryMode)

    Q_PROPERTY(ChatStatus status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString currentChannel READ currentChannel NOTIFY currentChannelChanged)
    Q_PROPERTY(QString username READ username NOTIFY usernameChanged)
    Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)
    Q_PROPERTY(int mixnodePoolSize READ mixnodePoolSize NOTIFY mixnodePoolSizeChanged)
    Q_PROPERTY(int lightpushPeersCount READ lightpushPeersCount NOTIFY lightpushPeersCountChanged)
    Q_PROPERTY(DiscoveryMode discoveryMode READ discoveryMode WRITE setDiscoveryMode NOTIFY discoveryModeChanged)
    Q_PROPERTY(QVariantList bootstrapNodes READ bootstrapNodes NOTIFY bootstrapNodesChanged)
    Q_PROPERTY(QString storeNode READ storeNode WRITE setStoreNode NOTIFY storeNodeChanged)

    explicit ChatBackend(LogosAPI* logosAPI = nullptr, QObject* parent = nullptr);
    ~ChatBackend();

    ChatStatus status() const { return m_status; }
    QString currentChannel() const { return m_currentChannel; }
    QString username() const { return m_username; }
    QVariantList messages() const { return m_messages; }
    int mixnodePoolSize() const { return m_mixnodePoolSize; }
    int lightpushPeersCount() const { return m_lightpushPeersCount; }
    DiscoveryMode discoveryMode() const { return m_discoveryMode; }
    QVariantList bootstrapNodes() const { return m_bootstrapNodes; }
    QString storeNode() const { return m_storeNode; }

    void setDiscoveryMode(DiscoveryMode mode);
    void setStoreNode(const QString& storeNode);

public slots:
    Q_INVOKABLE void joinChannel(const QString& channelName);
    Q_INVOKABLE void sendMessage(const QString& message);
    Q_INVOKABLE void clearMessages();
    Q_INVOKABLE void saveSettings();
    Q_INVOKABLE void addBootstrapNode(const QString& address, const QString& mixPubKey = QString());
    Q_INVOKABLE void updateBootstrapNodeMixKey(int index, const QString& mixPubKey);
    Q_INVOKABLE void removeBootstrapNode(int index);
    Q_INVOKABLE void requestRestart();

signals:
    void statusChanged();
    void currentChannelChanged();
    void usernameChanged();
    void messagesChanged();
    void mixnodePoolSizeChanged();
    void lightpushPeersCountChanged();
    void discoveryModeChanged();
    void bootstrapNodesChanged();
    void storeNodeChanged();
    void restartRequested();

private slots:
    void onChatMessage(const QVariantList& data);
    void onHistoryMessage(const QVariantList& data);
    void onMixnodePoolSizeResponse(const QVariantList& data);
    void onLightpushPeersCountResponse(const QVariantList& data);
    void refreshNetworkMetrics();

private:
    void setStatus(ChatStatus newStatus);
    void setCurrentChannel(const QString& channel);
    void initializeChat();
    void addMessage(const QString& sender, const QString& message, bool isHistory = false);
    void loadSettings();
    QVariantList getDefaultBootstrapNodes() const;
    QString getDefaultStoreNode() const;

    ChatStatus m_status;
    QString m_currentChannel;
    QString m_username;
    QString m_nodeKey;
    QVariantList m_messages;
    int m_mixnodePoolSize;
    int m_lightpushPeersCount;
    QTimer* m_metricsTimer;
    bool m_metricsSlowMode;
    DiscoveryMode m_discoveryMode;
    QVariantList m_bootstrapNodes;
    QString m_storeNode;

    LogosAPI* m_logosAPI;
    LogosModules* m_logos;
};

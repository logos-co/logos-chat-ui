#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QDateTime>
#include <QMap>

class ConversationListPanel : public QWidget {
    Q_OBJECT

public:
    explicit ConversationListPanel(QWidget* parent = nullptr);
    ~ConversationListPanel() = default;

signals:
    void conversationSelected(const QString& conversationId);
    void newConversationRequested();
    void myBundleRequested();

public slots:
    void addConversation(const QString& id, const QString& name, const QDateTime& lastActivity);
    void updateConversation(const QString& id, const QDateTime& lastActivity);
    void removeConversation(const QString& id);
    void clearConversations();
    void selectConversation(const QString& id);
    void incrementUnread(const QString& id);
    void clearUnread(const QString& id);

private slots:
    void onItemClicked(QListWidgetItem* item);
    void onNewConversationClicked();
    void onMyBundleClicked();

private:
    void setupUI();
    QString formatRelativeTime(const QDateTime& dateTime);
    void updateConversationDisplay(QListWidgetItem* item, const QString& name, const QDateTime& lastActivity, int unreadCount);

    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_headerLayout;
    QLabel* m_titleLabel;
    QPushButton* m_newConversationButton;
    QListWidget* m_conversationList;
    QPushButton* m_myBundleButton;

    // Map conversation ID to list item
    QMap<QString, QListWidgetItem*> m_conversationItems;
    // Store conversation data
    struct ConversationData {
        QString name;
        QDateTime lastActivity;
        int unreadCount = 0;
    };
    QMap<QString, ConversationData> m_conversationData;
};

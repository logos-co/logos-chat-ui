#pragma once

// Test double for the build-generated logos_sdk.h: a LogosModules whose
// chat_module answers from scripted state instead of over QtRO.
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <functional>
#include <optional>
#include <string>

struct LogosResult {
    bool success = false;
    QVariant value;
    QVariant error;

    template <typename T>
    T getError() const { return error.value<T>(); }
};

namespace logos {
struct CallError {
    std::string code;
    std::string message;
    bool ok() const { return code.empty(); }
};

template <typename T>
struct AsyncResult {
    T value{};
    CallError error;
    bool ok() const { return error.ok(); }
};
} // namespace logos

struct Timeout {
    explicit Timeout(int ms = 20000) : ms(ms) {}
    int ms;
};

class ChatModule {
public:
    struct Conversation {
        QString convo_id{};
        std::optional<QString> nickname{};
        qlonglong message_count{};
        qlonglong last_activity_ms{};
        QString kind{};
        std::optional<QString> name{};
        std::optional<QString> description{};
        std::optional<QString> preview{};
        bool history_only{};
        bool removed{};
    };
    struct Message {
        bool from_self{};
        QString content{};
        qlonglong timestamp_ms{};
        std::optional<QString> sender{};
    };
    struct Status {
        qlonglong convo_count{};
        QString delivery_state{};
        QString detail{};
        bool delivery_adopted{};
    };
    struct GroupMember {
        QString address{};
        bool pending{};
    };
    struct ChatConfig {
        std::optional<QString> delivery_preset{};
        std::optional<QString> log_level{};
    };

    using EventCallback = std::function<void(const QVariantList&)>;

    // The delivery state status() reports, and the node it is on.
    QString deliveryState = QStringLiteral("initialising");
    bool deliveryAdopted = false;
    // The address init settles on.
    QString address = QStringLiteral("fake-account-address-0123456789abcdef");
    QString logPath;
    // Runs while list_conversations is answering. The real client waits for a
    // reply in a nested event loop, which is where an event pushed meanwhile
    // is dispatched.
    std::function<void()> whileListingConversations;
    // What list_conversations answers.
    QList<Conversation> conversations;
    // What list_group_members answers, for any conversation.
    QList<GroupMember> members;
    // The reason create_conversation fails with; it succeeds while empty.
    QString createConversationError;
    // The reason remove_group_member fails with; it succeeds while empty.
    QString removeGroupMemberError;
    // Every remove_group_member call, as its conversation and address.
    QList<QStringList> removeGroupMemberCalls;

    void goOnline()
    {
        deliveryState = QStringLiteral("online");
        if (auto handler = m_handlers.value(QStringLiteral("delivery_state_changed")))
            handler({QStringLiteral("online"), QString(), deliveryAdopted});
    }

    bool on(const QString& name, EventCallback cb)
    {
        m_handlers.insert(name, std::move(cb));
        return true;
    }
    // Delivers an event as the module pushes it.
    void emitEvent(const QString& name, const QVariantList& args)
    {
        if (auto handler = m_handlers.value(name))
            handler(args);
    }

    LogosResult init(const ChatConfig&, logos::CallError* = nullptr)
    {
        m_initialised = true;
        return {true, {}, {}};
    }
    LogosResult shutdown(logos::CallError* = nullptr)
    {
        m_initialised = false;
        return {true, {}, {}};
    }
    QString get_log_path(logos::CallError* = nullptr) { return logPath; }
    QString get_address(logos::CallError* = nullptr) { return m_initialised ? address : QString(); }
    QList<Conversation> list_conversations(logos::CallError* = nullptr)
    {
        if (whileListingConversations)
            whileListingConversations();
        return conversations;
    }
    Status status(logos::CallError* = nullptr) { return {0, deliveryState, QString(), deliveryAdopted}; }
    void healthAsyncResult(std::function<void(logos::AsyncResult<bool>)>, Timeout = Timeout()) {}
    QList<Message> get_messages(const QString&, logos::CallError* = nullptr) { return {}; }
    QList<GroupMember> list_group_members(const QString&, logos::CallError* = nullptr) { return members; }
    LogosResult create_conversation(const QString&, logos::CallError* = nullptr)
    {
        if (!createConversationError.isEmpty())
            return {false, {}, createConversationError};
        return {true, {}, {}};
    }
    LogosResult create_group_conversation(const QString&, const QString&, logos::CallError* = nullptr)
    {
        return {true, {}, {}};
    }
    LogosResult add_group_member(const QString&, const QString&, logos::CallError* = nullptr)
    {
        return {true, {}, {}};
    }
    LogosResult remove_group_member(const QString& convoId, const QString& peerAddress,
                                    logos::CallError* = nullptr)
    {
        removeGroupMemberCalls.append({convoId, peerAddress});
        if (!removeGroupMemberError.isEmpty())
            return {false, {}, removeGroupMemberError};
        return {true, {}, {}};
    }
    LogosResult send_message(const QString&, const QString&, logos::CallError* = nullptr)
    {
        return {true, {}, {}};
    }

private:
    QHash<QString, EventCallback> m_handlers;
    bool m_initialised = false;
};

struct LogosModules {
    ChatModule chat_module;
};

#pragma once

// Test double for the build-generated logos_sdk.h: a LogosModules whose
// chat_module answers from scripted state instead of over QtRO.
#include <QHash>
#include <QList>
#include <QString>
#include <QTimer>
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
        std::optional<QString> protocol{};
        std::optional<QString> peer{};
        std::optional<QString> name{};
        std::optional<QString> description{};
        std::optional<QString> preview{};
        bool history_only{};
    };
    struct Message {
        bool from_self{};
        QString content{};
        qlonglong timestamp_ms{};
        std::optional<QString> sender{};
        std::optional<QString> message_id{};
        bool delivered{};
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
        std::optional<QString> account{};
    };

    using EventCallback = std::function<void(const QVariantList&)>;

    // The delivery state status() reports, and the node it is on.
    QString deliveryState = QStringLiteral("initialising");
    bool deliveryAdopted = false;
    // The address init settles on without an account.
    QString address = QStringLiteral("fake-account-address-0123456789abcdef");
    QString installationKey = QStringLiteral("fake-installation-key-0123456789abcdef");
    // The reason init refuses an account with, keeping the key; it accepts one
    // while empty.
    QString initError;
    // The reason init fails with after taking an account up, which replaces the
    // key as accepting it does.
    QString initFailsAfterTakingAccount;
    // The code the init call itself fails with, as on a timeout.
    std::string initCallError;
    // How many inits the module took, and the account the last one ran as.
    int inits = 0;
    QString initialisedAccount;
    QString logPath;
    // Runs while list_conversations is answering. The real client waits for a
    // reply in a nested event loop, which is where an event pushed meanwhile
    // is dispatched.
    std::function<void()> whileListingConversations;
    // What list_conversations answers.
    QList<Conversation> conversations;
    // What get_messages answers, for every conversation.
    QList<Message> messages;
    // What list_group_members answers, for every conversation.
    QList<GroupMember> members;
    // The name each address set, as get_display_name answers it.
    QHash<QString, QString> displayNames;
    // The error code get_display_name fails with; it answers while empty.
    std::string displayNameError;
    // The reason create_conversation fails with; it succeeds while empty.
    QString createConversationError;
    // How many create_conversation calls succeeded.
    int conversationsCreated = 0;
    // The delivery_preset init was called with.
    QString initialisedPreset;

    void goOnline()
    {
        deliveryState = QStringLiteral("online");
        if (auto handler = m_handlers.value(QStringLiteral("delivery_state_changed")))
            handler({QStringLiteral("online"), QString(), deliveryAdopted});
    }
    void goOffline()
    {
        deliveryState = QStringLiteral("offline");
        if (auto handler = m_handlers.value(QStringLiteral("delivery_state_changed")))
            handler({QStringLiteral("offline"), QString(), deliveryAdopted});
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

    // Answers on a later turn of the event loop, as the real client does.
    void initAsyncResult(const ChatConfig& config,
                         std::function<void(logos::AsyncResult<LogosResult>)> callback,
                         Timeout = Timeout())
    {
        logos::AsyncResult<LogosResult> answer;
        const QString account = config.account.value_or(QString());
        if (!initCallError.empty()) {
            answer.error = { initCallError, "init did not answer in time" };
        } else if (m_initialised) {
            // A second init succeeds and ignores its config.
            ++inits;
            answer.value = {true, {}, {}};
        } else if (!account.isEmpty() && !initError.isEmpty()) {
            answer.value = {false, {}, initError};
        } else if (!account.isEmpty() && !initFailsAfterTakingAccount.isEmpty()) {
            replaceInstallationKey();
            answer.value = {false, {}, initFailsAfterTakingAccount};
        } else {
            ++inits;
            initialisedPreset = config.delivery_preset.value_or(QString());
            initialisedAccount = account;
            if (!account.isEmpty())
                address = account;
            m_initialised = true;
            replaceInstallationKey();
            answer.value = {true, {}, {}};
        }
        QTimer::singleShot(0, [callback, answer] { callback(answer); });
    }
    LogosResult shutdown(logos::CallError* = nullptr)
    {
        m_initialised = false;
        return {true, {}, {}};
    }
    QString get_log_path(logos::CallError* = nullptr) { return logPath; }
    QString get_address(logos::CallError* = nullptr) { return m_initialised ? address : QString(); }
    QString get_installation_key(logos::CallError* = nullptr) { return installationKey; }
    QList<Conversation> list_conversations(logos::CallError* = nullptr)
    {
        if (whileListingConversations)
            whileListingConversations();
        return conversations;
    }
    Status status(logos::CallError* = nullptr) { return {0, deliveryState, QString(), deliveryAdopted}; }
    void healthAsyncResult(std::function<void(logos::AsyncResult<bool>)>, Timeout = Timeout()) {}
    QList<Message> get_messages(const QString&, logos::CallError* = nullptr) { return messages; }
    QList<GroupMember> list_group_members(const QString&, logos::CallError* = nullptr) { return members; }
    // Answers on a later turn of the event loop, as the real client does.
    void get_display_nameAsyncResult(const QString& address,
                                     std::function<void(logos::AsyncResult<QString>)> callback,
                                     Timeout = Timeout())
    {
        logos::AsyncResult<QString> answer;
        if (!displayNameError.empty())
            answer.error = { displayNameError, "the read failed" };
        else if (m_initialised)
            answer.value = displayNames.value(address);
        QTimer::singleShot(0, [callback, answer] { callback(answer); });
    }
    LogosResult create_conversation(const QString&, logos::CallError* = nullptr)
    {
        if (!createConversationError.isEmpty())
            return {false, {}, createConversationError};
        ++conversationsCreated;
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
    LogosResult send_message(const QString&, const QString&, logos::CallError* = nullptr)
    {
        return {true, {}, {}};
    }

private:
    void replaceInstallationKey()
    {
        installationKey = QStringLiteral("fake-installation-key-%1").arg(++m_keysMade);
    }

    QHash<QString, EventCallback> m_handlers;
    bool m_initialised = false;
    int m_keysMade = 0;
};

struct LogosModules {
    ChatModule chat_module;
};

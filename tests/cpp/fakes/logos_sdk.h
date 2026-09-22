#pragma once

// Test double for the build-generated logos_sdk.h: a LogosModules whose
// chat_module answers from scripted state instead of over QtRO.
#include <QHash>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <functional>
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
    int code = 0;
    std::string message;
    bool ok() const { return code == 0; }
};
} // namespace logos

struct Timeout {
    explicit Timeout(int ms = 20000) : ms(ms) {}
    int ms;
};

class FakeChatModule {
public:
    using EventCallback = std::function<void(const QVariantList&)>;

    // The delivery state status() reports.
    QString deliveryState = QStringLiteral("initialising");
    // The address init settles on.
    QString address = QStringLiteral("fake-account-address-0123456789abcdef");
    QString logPath;
    // Runs while list_conversations is answering. The real client waits for a
    // reply in a nested event loop, which is where an event pushed meanwhile
    // is dispatched.
    std::function<void()> whileListingConversations;

    void goOnline()
    {
        deliveryState = QStringLiteral("online");
        if (auto handler = m_handlers.value(QStringLiteral("delivery_state_changed")))
            handler({QStringLiteral("online"), QString()});
    }

    bool on(const QString& name, EventCallback cb)
    {
        m_handlers.insert(name, std::move(cb));
        return true;
    }

    LogosResult init(const QVariantMap&, logos::CallError* = nullptr)
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
    QVariantList list_conversations(logos::CallError* = nullptr)
    {
        if (whileListingConversations)
            whileListingConversations();
        return {};
    }
    QVariant status(logos::CallError* = nullptr)
    {
        return QVariantMap{{QStringLiteral("convo_count"), 0},
                           {QStringLiteral("delivery_state"), deliveryState},
                           {QStringLiteral("detail"), QString()}};
    }
    void healthAsync(std::function<void(bool)>, Timeout = Timeout()) {}
    QVariantList get_messages(const QString&, logos::CallError* = nullptr) { return {}; }
    QVariantList list_group_members(const QString&, logos::CallError* = nullptr) { return {}; }
    LogosResult create_conversation(const QString&, logos::CallError* = nullptr) { return {true, {}, {}}; }
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
    QHash<QString, EventCallback> m_handlers;
    bool m_initialised = false;
};

struct LogosModules {
    FakeChatModule chat_module;
};

#ifndef CHAT_UI_PLUGIN_H
#define CHAT_UI_PLUGIN_H

#include <QObject>
#include <QString>
#include <QtPlugin>
#include "chat_ui_interface.h"
#include "LogosViewPluginBase.h"

class LogosAPI;
class ChatBackend;

class ChatUiPlugin : public QObject,
                     public ChatUiInterface,
                     public ChatBackendViewPluginBase
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ChatUiInterface_iid FILE "../metadata.json")
    Q_INTERFACES(ChatUiInterface)

public:
    explicit ChatUiPlugin(QObject* parent = nullptr);
    ~ChatUiPlugin() override;

    QString name()    const override { return "chat_ui"; }
    QString version() const override { return "1.0.0"; }

    Q_INVOKABLE void initLogos(LogosAPI* api);

private:
    ChatBackend* m_backend = nullptr;
};

#endif

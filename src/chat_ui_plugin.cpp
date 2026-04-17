#include "chat_ui_plugin.h"
#include "ChatBackend.h"

#include <QDebug>

ChatUiPlugin::ChatUiPlugin(QObject* parent)
    : QObject(parent)
{
}

ChatUiPlugin::~ChatUiPlugin()
{
    if (m_backend) {
        m_backend->stopChat();
    }
}

void ChatUiPlugin::initLogos(LogosAPI* api)
{
    if (m_backend) return;
    m_backend = new ChatBackend(api, this);
    setBackend(m_backend);
    qDebug() << "ChatUiPlugin: backend initialized";
}

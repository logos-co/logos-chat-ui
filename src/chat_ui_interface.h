#ifndef CHAT_UI_INTERFACE_H
#define CHAT_UI_INTERFACE_H

#include "interface.h"

class ChatUiInterface : public PluginInterface
{
public:
    virtual ~ChatUiInterface() = default;
};

#define ChatUiInterface_iid "org.logos.ChatUiInterface"
Q_DECLARE_INTERFACE(ChatUiInterface, ChatUiInterface_iid)

#endif

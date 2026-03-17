#include "ChatUIComponent.h"
#include "src/ChatWindow.h"

QWidget* ChatUIComponent::createWidget(LogosAPI* logosAPI) {
    // Pass LogosAPI to ChatWindow for chat module integration
    return new ChatWindow(logosAPI);
}

void ChatUIComponent::destroyWidget(QWidget* widget) {
    delete widget;
}

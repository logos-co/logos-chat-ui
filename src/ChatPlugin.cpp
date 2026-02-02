#include "ChatPlugin.h"
#include "ChatBackend.h"
#include <QQuickWidget>
#include <QQmlContext>
#include <QQmlEngine>
#include <QDebug>
#include <QFileInfo>
#include <QFile>

QWidget* ChatPlugin::createWidget(LogosAPI* logosAPI) {
    qDebug() << "ChatPlugin::createWidget called";

    QQuickWidget* quickWidget = new QQuickWidget();
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    qmlRegisterType<ChatBackend>("ChatBackend", 1, 0, "ChatBackend");

    ChatBackend* backend = new ChatBackend(logosAPI, quickWidget);
    
    quickWidget->rootContext()->setContextProperty("backend", backend);

    // For development: check environment variable, otherwise use qrc
    QString qmlPath = "qrc:/ChatView.qml";
    QString envPath = qgetenv("CHAT_UI_QML_PATH");
    if (!envPath.isEmpty() && QFile::exists(envPath)) {
        qmlPath = QUrl::fromLocalFile(QFileInfo(envPath).absoluteFilePath()).toString();
        qDebug() << "Loading QML from file system:" << qmlPath;
    }
    
    quickWidget->setSource(QUrl(qmlPath));
    
    if (quickWidget->status() == QQuickWidget::Error) {
        qWarning() << "ChatPlugin: Failed to load QML:" << quickWidget->errors();
    }

    return quickWidget;
}

void ChatPlugin::destroyWidget(QWidget* widget) {
    delete widget;
}

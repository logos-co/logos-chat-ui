#include "ChatPlugin.h"
#include "ChatBackend.h"
#include <QQuickWidget>
#include <QQmlContext>
#include <QQmlEngine>
#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QCoreApplication>

QWidget* ChatPlugin::createWidget(LogosAPI* logosAPI) {
    qDebug() << "ChatPlugin::createWidget called";

    QQuickWidget* quickWidget = new QQuickWidget();
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    qmlRegisterType<ChatBackend>("ChatBackend", 1, 0, "ChatBackend");

    ChatBackend* backend = new ChatBackend(logosAPI, quickWidget);
    
    quickWidget->rootContext()->setContextProperty("backend", backend);

    bool showSettings = QCoreApplication::instance()->property("showSettings").toBool();
    quickWidget->rootContext()->setContextProperty("showSettings", showSettings);

    // For development: check environment variable, otherwise use qrc
    QString qmlPath = "qrc:/ChatView.qml";
    QString envPath = qgetenv("CHAT_UI_QML_PATH");
    if (!envPath.isEmpty() && QFile::exists(envPath)) {
        qmlPath = QUrl::fromLocalFile(QFileInfo(envPath).absoluteFilePath()).toString();
        qDebug() << "Loading QML from file system:" << qmlPath;
    }
    
    quickWidget->setSource(QUrl(qmlPath));
    
    // Connect QML engine's quit signal to application quit
    // Use QueuedConnection to ensure all QML cleanup happens first
    QObject::connect(quickWidget->engine(), &QQmlEngine::quit,
                     QCoreApplication::instance(), &QCoreApplication::quit,
                     Qt::QueuedConnection);
    
    if (quickWidget->status() == QQuickWidget::Error) {
        qWarning() << "ChatPlugin: Failed to load QML:" << quickWidget->errors();
    }

    return quickWidget;
}

void ChatPlugin::destroyWidget(QWidget* widget) {
    delete widget;
}

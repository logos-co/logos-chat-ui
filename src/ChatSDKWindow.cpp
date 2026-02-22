#include "ChatSDKWindow.h"
#include "ChatConfig.h"
#include "ChatPanel.h"
#include "ConversationListPanel.h"
#include <QAction>
#include <QClipboard>
#include <QDebug>
#include <QApplication>
#include <QGuiApplication>
#include <QPalette>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMessageBox>
#include <QRegularExpression>

ChatSDKWindow::ChatSDKWindow(LogosAPI *logosAPI, QWidget *parent)
    : QMainWindow(parent), m_logosAPI(logosAPI), m_ownsLogosAPI(false),
      m_logos(nullptr), m_chatInitialized(false), m_chatRunning(false),
      m_pendingBundleRequest(false), m_initChatAction(nullptr),
      m_startChatAction(nullptr), m_stopChatAction(nullptr) {
  // Create our own LogosAPI if none was provided
  if (!m_logosAPI) {
    m_logosAPI = new LogosAPI("core", this);
    m_ownsLogosAPI = true;
  }

  // Initialize LogosModules
  m_logos = new LogosModules(m_logosAPI);

  setupUI();
  setupMenu();
  setupEventHandlers();
}

ChatSDKWindow::~ChatSDKWindow() {
  // Stop and cleanup chat if running
  if (m_chatRunning && m_logos) {
    m_logos->chatsdk_module.stopChat();
  }

  delete m_logos;
  m_logos = nullptr;

  // Only delete LogosAPI if we created it ourselves
  if (m_ownsLogosAPI) {
    delete m_logosAPI;
  }
  m_logosAPI = nullptr;
}

void ChatSDKWindow::setupUI() {
  // Set window properties
  setWindowTitle("> \xce\xbb chat");
  setMinimumSize(800, 600);
  resize(1000, 700);

  // Set application-wide monospace font
  QFont appFont("JetBrains Mono", 12);
  appFont.setStyleHint(QFont::Monospace);
  QApplication::setFont(appFont);

  // Dark title bar (macOS and general)
  QPalette darkPalette = palette();
  darkPalette.setColor(QPalette::Window, QColor("#000000"));
  darkPalette.setColor(QPalette::WindowText, QColor("#FAFAFA"));
  setPalette(darkPalette);
  setStyleSheet("QMainWindow { background-color: #000000; }");

  // Create splitter - terminal theme
  m_splitter = new QSplitter(Qt::Horizontal, this);
  m_splitter->setHandleWidth(1);
  m_splitter->setStyleSheet("QSplitter {"
                            "  background-color: #0A0A0A;"
                            "}"
                            "QSplitter::handle {"
                            "  background-color: #2a2a2a;"
                            "}");

  // Create panels
  m_conversationList = new ConversationListPanel(m_splitter);
  m_chatPanel = new ChatPanel(m_splitter);

  // Set panel sizes (left: 250px, right: rest)
  m_splitter->addWidget(m_conversationList);
  m_splitter->addWidget(m_chatPanel);
  m_splitter->setSizes({250, 750});
  m_splitter->setCollapsible(0, false);
  m_splitter->setCollapsible(1, false);

  // Set minimum widths
  m_conversationList->setMinimumWidth(200);
  m_chatPanel->setMinimumWidth(400);

  setCentralWidget(m_splitter);

  // Create status bar - terminal theme
  m_statusBar = new QStatusBar(this);
  m_statusBar->setStyleSheet("QStatusBar {"
                             "  background-color: #0A0A0A;"
                             "  color: #6B7280;"
                             "  border-top: 1px solid #2a2a2a;"
                             "}");
  setStatusBar(m_statusBar);
  m_statusBar->showMessage("Ready");

  // Connect signals
  connect(m_conversationList, &ConversationListPanel::conversationSelected,
          this, &ChatSDKWindow::onConversationSelected);
  connect(m_conversationList, &ConversationListPanel::newConversationRequested,
          this, &ChatSDKWindow::onNewConversationRequested);
  connect(m_conversationList, &ConversationListPanel::myBundleRequested, this,
          &ChatSDKWindow::onMyBundleRequested);
  connect(m_chatPanel, &ChatPanel::messageSent, this,
          &ChatSDKWindow::onMessageSent);
}

void ChatSDKWindow::setupMenu() {
  // File menu
  QMenu *fileMenu = menuBar()->addMenu("&File");

  QAction *exitAction = fileMenu->addAction("E&xit");
  exitAction->setShortcut(QKeySequence::Quit);
  connect(exitAction, &QAction::triggered, this, &QMainWindow::close);

  // Chat menu - lifecycle management
  QMenu *chatMenu = menuBar()->addMenu("&Chat");

  m_initChatAction = chatMenu->addAction("&Initialize Chat");
  m_initChatAction->setShortcut(QKeySequence("Ctrl+I"));
  connect(m_initChatAction, &QAction::triggered, this,
          &ChatSDKWindow::onInitChat);

  m_startChatAction = chatMenu->addAction("&Start Chat");
  m_startChatAction->setShortcut(QKeySequence("Ctrl+Shift+S"));
  connect(m_startChatAction, &QAction::triggered, this,
          &ChatSDKWindow::onStartChat);
  m_startChatAction->setEnabled(false); // Disabled until initialized

  m_stopChatAction = chatMenu->addAction("Sto&p Chat");
  m_stopChatAction->setShortcut(QKeySequence("Ctrl+Shift+P"));
  connect(m_stopChatAction, &QAction::triggered, this,
          &ChatSDKWindow::onStopChat);
  m_stopChatAction->setEnabled(false); // Disabled until started

  // Help menu
  QMenu *helpMenu = menuBar()->addMenu("&Help");

  QAction *aboutAction = helpMenu->addAction("&About");
  connect(aboutAction, &QAction::triggered, this,
          &ChatSDKWindow::onAboutAction);
}

void ChatSDKWindow::setupEventHandlers() {
  if (!m_logos) {
    qWarning() << "ChatSDKWindow: LogosModules not available, event handlers "
                  "not set up";
    return;
  }

  // Subscribe to chatsdk module events
  m_logos->chatsdk_module.on(
      "chatsdkInitResult", [this](const QVariantList &data) {
        QMetaObject::invokeMethod(
            this, [this, data]() { onChatsdkInitResult(data); },
            Qt::QueuedConnection);
      });

  m_logos->chatsdk_module.on(
      "chatsdkStartResult", [this](const QVariantList &data) {
        QMetaObject::invokeMethod(
            this, [this, data]() { onChatsdkStartResult(data); },
            Qt::QueuedConnection);
      });

  m_logos->chatsdk_module.on(
      "chatsdkStopResult", [this](const QVariantList &data) {
        QMetaObject::invokeMethod(
            this, [this, data]() { onChatsdkStopResult(data); },
            Qt::QueuedConnection);
      });

  m_logos->chatsdk_module.on(
      "chatsdkCreateIntroBundleResult", [this](const QVariantList &data) {
        QMetaObject::invokeMethod(
            this, [this, data]() { onChatsdkCreateIntroBundleResult(data); },
            Qt::QueuedConnection);
      });

  m_logos->chatsdk_module.on(
      "chatsdkNewMessage", [this](const QVariantList &data) {
        QMetaObject::invokeMethod(
            this, [this, data]() { onChatsdkNewMessage(data); },
            Qt::QueuedConnection);
      });

  m_logos->chatsdk_module.on(
      "chatsdkNewConversation", [this](const QVariantList &data) {
        QMetaObject::invokeMethod(
            this, [this, data]() { onChatsdkNewConversation(data); },
            Qt::QueuedConnection);
      });

  m_logos->chatsdk_module.on(
      "chatsdkNewPrivateConversationResult", [this](const QVariantList &data) {
        QMetaObject::invokeMethod(
            this,
            [this, data]() { onChatsdkNewPrivateConversationResult(data); },
            Qt::QueuedConnection);
      });

  m_logos->chatsdk_module.on(
      "chatsdkSendMessageResult", [this](const QVariantList &data) {
        QMetaObject::invokeMethod(
            this, [this, data]() { onChatsdkSendMessageResult(data); },
            Qt::QueuedConnection);
      });

  qDebug() << "ChatSDKWindow: Event handlers set up successfully";
}

void ChatSDKWindow::updateChatMenuState() {
  if (m_initChatAction) {
    m_initChatAction->setEnabled(!m_chatInitialized);
  }
  if (m_startChatAction) {
    m_startChatAction->setEnabled(m_chatInitialized && !m_chatRunning);
  }
  if (m_stopChatAction) {
    m_stopChatAction->setEnabled(m_chatRunning);
  }
}

void ChatSDKWindow::onConversationSelected(const QString &conversationId) {
  if (!m_conversations.contains(conversationId)) {
    return;
  }

  m_currentConversationId = conversationId;
  const auto &convo = m_conversations[conversationId];
  m_chatPanel->setConversation(conversationId, convo.name);
  m_statusBar->showMessage(QString("Conversation: %1").arg(convo.name), 3000);
}

void ChatSDKWindow::onNewConversationRequested() {
  // Check if chat is initialized and running
  if (!m_chatInitialized || !m_chatRunning) {
    QMessageBox::warning(this, "Chat Not Running",
                         "Please initialize and start chat first (Chat menu) "
                         "before creating conversations.");
    return;
  }

  if (!m_logos) {
    QMessageBox::warning(this, "Error", "LogosAPI not available.");
    return;
  }

  bool ok;
  QString bundle = QInputDialog::getMultiLineText(
      this, "New Conversation",
      "Paste the other user's intro bundle (JSON):", "", &ok);

  if (ok && !bundle.isEmpty()) {
    // Validate it looks like JSON
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(bundle.toUtf8(), &error);
    if (doc.isNull()) {
      QMessageBox::warning(
          this, "Invalid Bundle",
          QString("The bundle doesn't appear to be valid JSON:\n%1")
              .arg(error.errorString()));
      return;
    }

    m_statusBar->showMessage("Creating new conversation...");

    // Create the private conversation with an initial greeting message
    // Content must be hex-encoded for the libchat API
    QString initialMessage = "Hello!";
    QString initialMessageHex =
        QString::fromLatin1(initialMessage.toUtf8().toHex());

    bool success = m_logos->chatsdk_module.newPrivateConversation(
        bundle, initialMessageHex);

    if (!success) {
      QMessageBox::warning(this, "Error",
                           "Failed to initiate conversation creation. Check "
                           "the logs for details.");
      m_statusBar->showMessage("Failed to create conversation", 3000);
    }
    // Result will come via onChatsdkNewPrivateConversationResult
  }
}

void ChatSDKWindow::onMyBundleRequested() {
  // Check if chat is initialized and running
  if (!m_chatInitialized || !m_chatRunning) {
    QMessageBox::warning(this, "Chat Not Running",
                         "Please initialize and start chat first (Chat menu) "
                         "before getting your bundle.");
    return;
  }

  if (!m_logos) {
    QMessageBox::warning(this, "Error",
                         "LogosAPI not available. Cannot retrieve bundle.");
    return;
  }

  // Set flag to show dialog when result comes back
  m_pendingBundleRequest = true;
  m_statusBar->showMessage("Requesting intro bundle...");

  // Call the actual createIntroBundle
  bool success = m_logos->chatsdk_module.createIntroBundle();
  if (!success) {
    m_pendingBundleRequest = false;
    QMessageBox::warning(
        this, "Error",
        "Failed to request intro bundle. Please check that chat is running.");
    m_statusBar->showMessage("Failed to request bundle", 3000);
  }
}

// ============================================================================
// Chat Lifecycle Menu Actions
// ============================================================================

void ChatSDKWindow::onInitChat() {
  if (!m_logos) {
    QMessageBox::warning(
        this, "Error",
        "LogosAPI not available. Cannot initialize chat.\n\n"
        "Make sure the application was started with proper module loading.");
    return;
  }

  if (m_chatInitialized) {
    m_statusBar->showMessage("Chat already initialized", 3000);
    return;
  }

  // Build configuration from ChatConfig defaults (which use env vars as
  // overrides)
  QString configJson = ChatConfig::buildConfigJson();
  QString configDesc = ChatConfig::getConfigDescription(configJson);

  qDebug() << "ChatSDKWindow: Initializing chat with config:" << configJson;
  m_statusBar->showMessage(
      QString("Initializing chat... (%1)").arg(configDesc));

  bool success = m_logos->chatsdk_module.initChat(configJson);
  if (!success) {
    QMessageBox::warning(
        this, "Initialization Failed",
        "Failed to initialize chat. Check the logs for details.");
    m_statusBar->showMessage("Chat initialization failed", 3000);
  }
  // Result will come via onChatsdkInitResult
}

void ChatSDKWindow::onStartChat() {
  if (!m_logos) {
    QMessageBox::warning(this, "Error", "LogosAPI not available.");
    return;
  }

  if (!m_chatInitialized) {
    QMessageBox::warning(
        this, "Not Initialized",
        "Please initialize chat first (Chat > Initialize Chat).");
    return;
  }

  if (m_chatRunning) {
    m_statusBar->showMessage("Chat already running", 3000);
    return;
  }

  qDebug() << "ChatSDKWindow: Starting chat...";
  m_statusBar->showMessage("Starting chat...");

  // Set the event callback before starting
  m_logos->chatsdk_module.setEventCallback();

  bool success = m_logos->chatsdk_module.startChat();
  if (!success) {
    QMessageBox::warning(this, "Start Failed",
                         "Failed to start chat. Check the logs for details.");
    m_statusBar->showMessage("Chat start failed", 3000);
  }
  // Result will come via onChatsdkStartResult
}

void ChatSDKWindow::onStopChat() {
  if (!m_logos) {
    QMessageBox::warning(this, "Error", "LogosAPI not available.");
    return;
  }

  if (!m_chatRunning) {
    m_statusBar->showMessage("Chat is not running", 3000);
    return;
  }

  qDebug() << "ChatSDKWindow: Stopping chat...";
  m_statusBar->showMessage("Stopping chat...");

  bool success = m_logos->chatsdk_module.stopChat();
  if (!success) {
    QMessageBox::warning(this, "Stop Failed",
                         "Failed to stop chat. Check the logs for details.");
    m_statusBar->showMessage("Chat stop failed", 3000);
  }
  // Result will come via onChatsdkStopResult
}

// ============================================================================
// Event Handlers for ChatSDK Module
// ============================================================================

void ChatSDKWindow::onChatsdkInitResult(const QVariantList &data) {
  qDebug() << "ChatSDKWindow: Init result received:" << data;

  // data format: [success (bool), returnCode (int), message (QString),
  // timestamp (QString)]
  bool success = data.size() > 0 ? data[0].toBool() : false;
  int returnCode = data.size() > 1 ? data[1].toInt() : -1;
  QString message = data.size() > 2 ? data[2].toString() : "";

  if (success) {
    m_chatInitialized = true;
    m_statusBar->showMessage("Chat initialized successfully", 5000);
    updateChatMenuState();

    QMessageBox::information(
        this, "Chat Initialized",
        "Chat has been initialized successfully.\n\n"
        "You can now start the chat using Chat > Start Chat.");
  } else {
    m_statusBar->showMessage(
        QString("Chat initialization failed (code: %1)").arg(returnCode), 5000);
    QMessageBox::warning(
        this, "Initialization Failed",
        QString("Failed to initialize chat.\nError code: %1\n%2")
            .arg(returnCode)
            .arg(message));
  }
}

void ChatSDKWindow::onChatsdkStartResult(const QVariantList &data) {
  qDebug() << "ChatSDKWindow: Start result received:" << data;

  // data format: [success (bool), returnCode (int), message (QString),
  // timestamp (QString)]
  bool success = data.size() > 0 ? data[0].toBool() : false;
  int returnCode = data.size() > 1 ? data[1].toInt() : -1;
  QString message = data.size() > 2 ? data[2].toString() : "";

  if (success) {
    m_chatRunning = true;
    m_statusBar->showMessage("Chat started - connected to network", 5000);
    updateChatMenuState();
  } else {
    m_statusBar->showMessage(
        QString("Chat start failed (code: %1)").arg(returnCode), 5000);
    QMessageBox::warning(this, "Start Failed",
                         QString("Failed to start chat.\nError code: %1\n%2")
                             .arg(returnCode)
                             .arg(message));
  }
}

void ChatSDKWindow::onChatsdkStopResult(const QVariantList &data) {
  qDebug() << "ChatSDKWindow: Stop result received:" << data;

  // data format: [success (bool), returnCode (int), message (QString),
  // timestamp (QString)]
  bool success = data.size() > 0 ? data[0].toBool() : false;
  int returnCode = data.size() > 1 ? data[1].toInt() : -1;

  if (success) {
    m_chatRunning = false;
    m_statusBar->showMessage("Chat stopped", 5000);
    updateChatMenuState();
  } else {
    m_statusBar->showMessage(
        QString("Chat stop failed (code: %1)").arg(returnCode), 5000);
  }
}

void ChatSDKWindow::onChatsdkCreateIntroBundleResult(const QVariantList &data) {
  qDebug() << "ChatSDKWindow: Create intro bundle result received:" << data;

  if (!m_pendingBundleRequest) {
    // Bundle request wasn't from us
    return;
  }
  m_pendingBundleRequest = false;

  // data format: [success (bool), returnCode (int), bundleJson (QString),
  // timestamp (QString)]
  bool success = data.size() > 0 ? data[0].toBool() : false;
  int returnCode = data.size() > 1 ? data[1].toInt() : -1;
  QString bundleJson = data.size() > 2 ? data[2].toString() : "";

  if (!success || bundleJson.isEmpty()) {
    QMessageBox::warning(
        this, "Error",
        QString("Failed to create bundle.\nError code: %1").arg(returnCode));
    m_statusBar->showMessage("Failed to get bundle", 3000);
    return;
  }

  // Show the bundle dialog
  QMessageBox msgBox(this);
  msgBox.setWindowTitle("My Bundle");
  msgBox.setText("Share this bundle with others to start a conversation:");
  msgBox.setInformativeText(bundleJson);
  msgBox.setIcon(QMessageBox::Information);
  msgBox.setStandardButtons(QMessageBox::Ok);
  msgBox.setTextInteractionFlags(Qt::TextSelectableByMouse);

  QPushButton *copyBtn =
      msgBox.addButton("Copy to Clipboard", QMessageBox::ActionRole);
  msgBox.exec();

  if (msgBox.clickedButton() == copyBtn) {
    QGuiApplication::clipboard()->setText(bundleJson);
    m_statusBar->showMessage("Bundle copied to clipboard", 3000);
  }
}

void ChatSDKWindow::onChatsdkNewMessage(const QVariantList &data) {
  qDebug() << "ChatSDKWindow: New message received:" << data;

  if (data.isEmpty())
    return;

  // Parse the JSON message
  QString jsonStr = data[0].toString();
  QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
  if (!doc.isObject())
    return;

  QJsonObject obj = doc.object();
  QString conversationId = obj["conversationId"].toString();
  if (conversationId.isEmpty()) {
    conversationId = obj["conversation_id"].toString();
  }

  QString content = obj["content"].toString();
  // If content looks like hex, decode it
  if (content.contains(QRegularExpression("^[0-9a-fA-F]+$")) &&
      content.length() % 2 == 0) {
    QByteArray contentBytes = QByteArray::fromHex(content.toUtf8());
    content = QString::fromUtf8(contentBytes);
  }

  QString sender = obj["sender"].toString();
  if (sender.isEmpty()) {
    sender = obj["from"].toString();
  }
  if (sender.isEmpty()) {
    sender = "Peer";
  }

  // Update conversation list with new activity
  if (m_conversations.contains(conversationId)) {
    m_conversations[conversationId].lastActivity = QDateTime::currentDateTime();
  }

  // If this is the currently selected conversation, show the message
  if (conversationId == m_currentConversationId) {
    m_chatPanel->addMessage(sender, content, QDateTime::currentDateTime(),
                            false);
  }

  // Show notification
  m_statusBar->showMessage(QString("New message from %1").arg(sender), 3000);
}

void ChatSDKWindow::onChatsdkNewConversation(const QVariantList &data) {
  qDebug() << "ChatSDKWindow: New conversation received:" << data;

  if (data.isEmpty())
    return;

  // Parse the JSON
  QString jsonStr = data[0].toString();
  QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
  if (!doc.isObject())
    return;

  QJsonObject obj = doc.object();
  QString conversationId = obj["conversationId"].toString();
  QString conversationType = obj["conversationType"].toString();

  // Add to conversation list
  m_conversationList->addConversation(
      conversationId, QString("Chat %1").arg(conversationId.left(8)),
      QDateTime::currentDateTime());

  m_conversations[conversationId] = {
      QString("Chat %1").arg(conversationId.left(8)),
      QDateTime::currentDateTime()};

  m_statusBar->showMessage(
      QString("New %1 conversation created").arg(conversationType), 3000);
}

void ChatSDKWindow::onChatsdkNewPrivateConversationResult(
    const QVariantList &data) {
  qDebug() << "ChatSDKWindow: New private conversation result:" << data;

  // data format: [success (bool), returnCode (int), conversationJson (QString),
  // timestamp (QString)]
  bool success = data.size() > 0 ? data[0].toBool() : false;
  int returnCode = data.size() > 1 ? data[1].toInt() : -1;
  QString conversationJson = data.size() > 2 ? data[2].toString() : "";

  if (!success) {
    QMessageBox::warning(
        this, "Error",
        QString("Failed to create conversation.\nError code: %1")
            .arg(returnCode));
    m_statusBar->showMessage("Failed to create conversation", 3000);
    return;
  }

  // Parse the conversation JSON to get the ID
  QJsonDocument doc = QJsonDocument::fromJson(conversationJson.toUtf8());
  QString conversationId;
  QString peerName = "Peer";

  if (doc.isObject()) {
    QJsonObject obj = doc.object();
    conversationId = obj["conversationId"].toString();
    if (conversationId.isEmpty()) {
      conversationId = obj["id"].toString();
    }
    // Try to get peer name from the response
    if (obj.contains("peerName")) {
      peerName = obj["peerName"].toString();
    }
  }

  if (conversationId.isEmpty()) {
    // Generate a temporary ID if none returned
    conversationId =
        QString("conv-%1").arg(QDateTime::currentMSecsSinceEpoch());
  }

  // Add to conversation list
  QString displayName = QString("Chat with %1").arg(peerName);
  m_conversationList->addConversation(conversationId, displayName,
                                      QDateTime::currentDateTime());

  m_conversations[conversationId] = {displayName, QDateTime::currentDateTime()};

  // Auto-select the new conversation
  m_conversationList->selectConversation(conversationId);
  onConversationSelected(conversationId);

  m_statusBar->showMessage("New conversation created!", 3000);
  QMessageBox::information(
      this, "Conversation Created",
      "New private conversation has been created.\nYou can now send messages!");
}

void ChatSDKWindow::onChatsdkSendMessageResult(const QVariantList &data) {
  qDebug() << "ChatSDKWindow: Send message result:" << data;

  // data format: [success (bool), returnCode (int), resultJson (QString),
  // timestamp (QString)]
  bool success = data.size() > 0 ? data[0].toBool() : false;
  int returnCode = data.size() > 1 ? data[1].toInt() : -1;

  if (success) {
    m_statusBar->showMessage("Message sent", 2000);
  } else {
    m_statusBar->showMessage(
        QString("Failed to send message (code: %1)").arg(returnCode), 3000);
  }
}

void ChatSDKWindow::onMessageSent(const QString &conversationId,
                                  const QString &content) {
  if (!m_chatRunning || !m_logos) {
    m_statusBar->showMessage("Cannot send - chat not running", 3000);
    return;
  }

  if (conversationId.isEmpty()) {
    m_statusBar->showMessage("No conversation selected", 3000);
    return;
  }

  qDebug() << "ChatSDKWindow: Sending message to conversation:"
           << conversationId << "content:" << content;

  // Content must be hex-encoded for the libchat API
  QString contentHex = QString::fromLatin1(content.toUtf8().toHex());

  // Send via the chatsdk_module
  bool success =
      m_logos->chatsdk_module.sendMessage(conversationId, contentHex);

  if (success) {
    m_statusBar->showMessage("Sending message...", 2000);
  } else {
    m_statusBar->showMessage("Failed to send message", 3000);
  }
  // Result will come via onChatsdkSendMessageResult
}

void ChatSDKWindow::onAboutAction() {
  QMessageBox::about(this, "About Logos Chat SDK UI",
                     "Logos Chat SDK UI\n\n"
                     "Version 1.0.0\n\n"
                     "A Qt-based chat interface for the Logos platform.\n\n"
                     "Phase 1: UI Skeleton with mock data\n"
                     "Phase 2: Backend integration (coming soon)");
}

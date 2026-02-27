# logos-chatsdk-ui Specification

## Overview

A Qt-based UI module for the Logos platform that provides a chat interface using the `logos-chatsdk-module` backend. This module follows the same architectural patterns as `logos-chat-ui` but implements a two-panel conversation-based chat interface.

## Architecture

### Module Structure

```
logos-chatsdk-ui/
├── app/                           # Standalone app (loads the plugin)
│   ├── CMakeLists.txt             # App build configuration
│   ├── main.cpp                   # App entry point (starts Logos core)
│   ├── mainwindow.h               # App main window header
│   └── mainwindow.cpp             # App main window (loads plugin via QPluginLoader)
├── interfaces/
│   └── IComponent.h               # Component interface (same as logos-chat-ui)
├── resources/
│   └── resources.qrc              # Qt resource file (empty root)
├── generated_code/
│   └── logos_sdk.cpp              # Pre-generated Logos SDK bindings (nix)
├── src/                           # Plugin UI widgets
│   ├── ChatConfig.h               # Chat configuration helpers (env-driven)
│   ├── ChatSDKWindow.h            # Main window (QMainWindow)
│   ├── ChatSDKWindow.cpp
│   ├── ConversationListPanel.h    # Left panel widget
│   ├── ConversationListPanel.cpp
│   ├── ChatPanel.h                # Right panel widget
│   ├── ChatPanel.cpp
│   ├── MessageBubble.h            # Custom message display widget
│   └── MessageBubble.cpp
├── nix/
│   ├── default.nix                # Common build configuration
│   ├── lib.nix                    # Library/plugin build
│   └── app.nix                    # Standalone app build
├── ChatSDKUIComponent.h           # Plugin component header
├── ChatSDKUIComponent.cpp         # Plugin component implementation
├── CMakeLists.txt                 # Root build configuration
├── metadata.json                  # Module metadata
├── flake.nix                      # Nix flake
├── flake.lock                     # Nix flake lock
├── .gitignore                     # Git ignore file
└── spec.md                        # This file
```

### Dependencies

| Dependency | Purpose |
|------------|---------|
| `Qt6::Core` | Core Qt functionality |
| `Qt6::Widgets` | UI widgets |
| `Qt6::RemoteObjects` | LogosAPI integration and module bindings |
| `logos-cpp-sdk` | LogosAPI, generator for module bindings |
| `logos-liblogos` | Core Logos library (logoscore, logos_host) |
| `logos-chatsdk-module` | Chat SDK backend module |
| `logos-capability-module` | Capability/auth module (standalone app) |

### Build Targets

| Target | Output | Description |
|--------|--------|-------------|
| `chatsdk_ui` (lib) | `chatsdk_ui.dylib` / `.so` | Qt plugin library |
| `logos-chatsdk-ui-app` (app) | `logos-chatsdk-ui-app` | Standalone executable |

---

## UI Layout

### Main Window

The main window (`ChatSDKWindow`) is split into two panels using a `QSplitter` and uses a dark, terminal-inspired theme with a monospace font:

```
┌─────────────────────────────────────────────────────────────────────┐
│  Menu Bar (File | Chat | Help)                                      │
├──────────────────────┬──────────────────────────────────────────────┤
│                      │                                              │
│   > lambda chat      │          CHAT PANEL                          │
│   + new              │                                              │
│                      │    Conversation Title                        │
│  ┌────────────────┐  │  ───────────────────────────────────────     │
│  │ Alice          │  │                                              │
│  │ 2 min ago      │  │     [Message from counterparty]              │
│  └────────────────┘  │      10:30 AM                                │
│                      │                                              │
│  ┌────────────────┐  │                        [My message]          │
│  │ Bob            │  │                         10:31 AM             │
│  │ 5 min ago      │  │                                              │
│  └────────────────┘  │     [Message from counterparty]              │
│                      │      10:32 AM                                │
│                      │                                              │
│                      │  ───────────────────────────────────────     │
│                      │  ┌──────────────────────────────┐  ┌──────┐  │
│                      │  │ Type a message...            │  │  >>  │  │
│  ┌────────────────┐  │  └──────────────────────────────┘  └──────┘  │
│  │ Generate Intro │  │                                              │
│  │ Bundle         │  │                                              │
│  └────────────────┘  │                                              │
├──────────────────────┴──────────────────────────────────────────────┤
│  Status Bar (includes identity label)                               │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Component Specifications

### 1. ConversationListPanel (Left Panel)

**Class**: `ConversationListPanel : public QWidget`

#### Layout
- **Header**: Horizontal layout containing:
  - `QLabel` with text "> lambda chat" (bold, larger font)
  - `QPushButton` labeled "+ new"
- **Conversation List**: `QListWidget` showing all conversations
  - Each item displays:
    - Conversation name (bold)
    - Relative timestamp of last activity (e.g., "2 min ago", "Yesterday")
- **Footer**: `QPushButton` labeled "Generate Intro Bundle" spanning full width

#### Signals
```cpp
signals:
    void conversationSelected(const QString& conversationId);
    void newConversationRequested();
    void myBundleRequested();
```

#### Slots
```cpp
public slots:
    void addConversation(const QString& id, const QString& name, const QDateTime& lastActivity);
    void updateConversation(const QString& id, const QDateTime& lastActivity);
    void removeConversation(const QString& id);
    void clearConversations();
    void selectConversation(const QString& id);
    void incrementUnread(const QString& id);
    void clearUnread(const QString& id);
```

#### Behavior
- Clicking a conversation item emits `conversationSelected(id)`
- Clicking "+ new" emits `newConversationRequested()`
- Clicking "Generate Intro Bundle" emits `myBundleRequested()`
- Selected conversation should be visually highlighted
- Unread messages show a red badge with a capped count (99+)

---

### 2. ChatPanel (Right Panel)

**Class**: `ChatPanel : public QWidget`

#### Layout (No Conversation Selected)
- Centered `QLabel` with text "Select a conversation or start a new one"
- Gray, italic styling

#### Layout (Conversation Selected)
- **Header**: `QLabel` with conversation name as title (bold, larger font)
- **Messages Area**: `QScrollArea` containing `QVBoxLayout` with message bubbles
  - Scrolls to bottom when new messages arrive
- **Input Area**: Horizontal layout containing:
  - `QLineEdit` for message input (placeholder: "Type a message...")
  - `QPushButton` labeled ">>"

#### Message Display
Each message is displayed using `MessageBubble` widget:
- **My messages**: Right-aligned, green background (`#10B981`)
- **Counterparty messages**: Left-aligned, dark background (`#1F1F1F`) with subtle border (`#2a2a2a`)
- **Timestamp**: Small, muted text below message content

#### Signals
```cpp
signals:
    void messageSent(const QString& conversationId, const QString& content);
```

#### Slots
```cpp
public slots:
    void setConversation(const QString& id, const QString& name);
    void clearConversation();
    void addMessage(const QString& sender, const QString& content, 
                    const QDateTime& timestamp, bool isMe);
    void clearMessages();
```

#### Behavior
- Send button click or Enter key press:
  1. Validates message is not empty
  2. Emits `messageSent(conversationId, content)`
  3. Adds the message to the UI immediately (optimistic update)
  4. Clears input field
- Messages auto-scroll to bottom on new message arrival
- Input is disabled when no conversation is selected

---

### 3. MessageBubble Widget

**Class**: `MessageBubble : public QWidget`

#### Properties
- `QString content` - The message text
- `QDateTime timestamp` - When the message was sent
- `bool isMe` - Whether this message is from the current user

#### Visual Design
```
My Message (right-aligned):
                                    ┌─────────────────────┐
                                    │ Hello, how are you? │
                                    └─────────────────────┘
                                              10:31 AM

Counterparty Message (left-aligned):
┌─────────────────────────┐
│ I'm doing great, thanks!│
└─────────────────────────┘
        10:32 AM
```

#### Styling
- Border radius: 8px
- Padding: 16px (internal), with outer margins for spacing
- My messages: Background `#10B981`, aligned right
- Counterparty: Background `#1F1F1F`, aligned left, border `#2a2a2a`
- Timestamp: Font size 10px, muted color; aligned with the bubble
- Content text is selectable

---

### 4. ChatSDKWindow (Main Window)

**Class**: `ChatSDKWindow : public QMainWindow`

#### Menu Structure
- **File**
  - Exit (`Ctrl+Q`)
- **Chat**
  - Initialize Chat (`Ctrl+I`)
  - Start Chat (`Ctrl+Shift+S`)
  - Stop Chat (`Ctrl+Shift+P`)
- **Help**
  - About

#### Components
- `ConversationListPanel* conversationList`
- `ChatPanel* chatPanel`
- `QSplitter* splitter` (horizontal, to allow resizing panels)
- `QStatusBar* statusBar`
- Identity label in the status bar (right side)
- Window title uses a lambda glyph (rendered as "> lambda chat") and JetBrains Mono as the app font

#### Dialog Handlers

##### New Conversation Dialog
When `newConversationRequested()` is received and chat is running, a dialog requests:
- Intro bundle (multi-line)
- Intro message (single-line)

If valid, it calls `chatsdk_module.newPrivateConversation(bundle, messageHex)`.

##### My Bundle Dialog
When `myBundleRequested()` is received and chat is running, the UI calls
`chatsdk_module.createIntroBundle()` and shows the returned bundle with
"Copy to Clipboard" support.

##### Chat Lifecycle
The window auto-initializes chat on launch, and can auto-start when init succeeds.
Menu actions enable/disable based on chat state.

---

### 5. ChatSDKUIComponent (Plugin)

**Class**: `ChatSDKUIComponent : public QObject, public IComponent`

```cpp
class ChatSDKUIComponent : public QObject, public IComponent {
    Q_OBJECT
    Q_INTERFACES(IComponent)
    Q_PLUGIN_METADATA(IID IComponent_iid FILE "metadata.json")

public:
    Q_INVOKABLE QWidget* createWidget(LogosAPI* logosAPI = nullptr) override;
    void destroyWidget(QWidget* widget) override;
};
```

---

## Chat Configuration

The plugin uses `ChatConfig` to build the JSON payload passed to
`chatsdk_module.initChat()`. Defaults can be overridden via environment variables:

- `CHATSDK_NAME` (identity name)
- `CHATSDK_PORT` (Waku port, 0 for random)
- `CHATSDK_CLUSTER_ID`
- `CHATSDK_SHARD_ID`
- `CHATSDK_STATIC_PEER` (optional multiaddr)

## Event Handling

The UI listens to chatsdk module events and keeps local state:

- `chatsdkInitResult`, `chatsdkStartResult`, `chatsdkStopResult`
- `chatsdkCreateIntroBundleResult`
- `chatsdkNewConversation`
- `chatsdkNewPrivateConversationResult`
- `chatsdkNewMessage`
- `chatsdkSendMessageResult`
- `chatsdkGetIdResult`

Message content is hex-encoded for sending and decoded on receipt when the
payload looks like hex.

---

## Styling Guidelines

### Colors
| Element | Color Code |
|---------|------------|
| App background | `#000000` |
| Panel background | `#0A0A0A` |
| Panel divider | `#2a2a2a` |
| Accent button | `#10B981` |
| My message background | `#10B981` |
| Counterparty message background | `#1F1F1F` |
| Counterparty message border | `#2a2a2a` |
| Timestamp text | `#4B5563` |
| Selected conversation | `#1F1F1F` |

### Fonts
- **Primary**: JetBrains Mono (monospace)
- **Headers**: Bold, 14pt
- **Conversation name**: Bold, 12pt
- **Conversation timestamp**: Normal, 10pt, muted gray
- **Message content**: Normal, 13pt
- **Message timestamp**: Normal, 10pt

### Dimensions
- Minimum window size: 800x600
- Default left panel width: 250px
- Minimum left panel width: 200px
- Maximum message bubble width: roughly half the panel (spacer-based layout)
- Message bubble border radius: 8px
- Message bubble padding: 16px

---

## Build Configuration

### metadata.json

```json
{
  "name": "chatsdk_ui",
  "version": "1.0.0",
  "description": "Chat App for Logos - Private messaging interface",
  "author": "Logos Core Team",
  "type": "ui",
  "main": "chatsdk_ui",
  "dependencies": ["chatsdk_module"],
  "category": "chat",
  "build": {
    "type": "cmake",
    "files": [
      "src/ChatSDKWindow.cpp",
      "src/ChatSDKWindow.h",
      "src/ConversationListPanel.cpp",
      "src/ConversationListPanel.h",
      "src/ChatPanel.cpp",
      "src/ChatPanel.h",
      "src/MessageBubble.cpp",
      "src/MessageBubble.h"
    ]
  },
  "capabilities": [
    "ui_components",
    "private_messaging"
  ]
}
```

### CMakeLists.txt Key Points

```cmake
cmake_minimum_required(VERSION 3.16)
project(ChatSDKUIPlugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets RemoteObjects)

# Require dependency roots (provided by nix or env)
if(NOT DEFINED LOGOS_LIBLOGOS_ROOT)
  message(FATAL_ERROR "LOGOS_LIBLOGOS_ROOT must be defined")
endif()
if(NOT DEFINED LOGOS_CPP_SDK_ROOT)
  message(FATAL_ERROR "LOGOS_CPP_SDK_ROOT must be defined")
endif()

set(SOURCES
  ChatSDKUIComponent.cpp
  src/ChatSDKWindow.cpp
  src/ConversationListPanel.cpp
  src/ChatPanel.cpp
  src/MessageBubble.cpp
  resources/resources.qrc
  generated_code/logos_sdk.cpp
)

add_library(chatsdk_ui SHARED ${SOURCES})

find_library(LOGOS_SDK_LIB logos_sdk PATHS ${LOGOS_CPP_SDK_ROOT}/lib NO_DEFAULT_PATH REQUIRED)

target_link_libraries(chatsdk_ui PRIVATE
  Qt6::Core
  Qt6::Widgets
  Qt6::RemoteObjects
  component-interfaces
  ${LOGOS_SDK_LIB}
)
```

---

## Implementation Phases

### Phase 1: UI Skeleton (Done)
- [x] Create spec document
- [x] Create project structure
- [x] Implement `MessageBubble` widget
- [x] Implement `ConversationListPanel` 
- [x] Implement `ChatPanel`
- [x] Implement `ChatSDKWindow`
- [x] Implement `ChatSDKUIComponent`
- [x] Setup CMakeLists.txt and build configuration
- [x] Setup flake.nix

### Phase 2: Backend Integration (Done)
- [x] Connect to `logos-chatsdk-module`
- [x] Implement `initChat`, `startChat`, `stopChat`
- [x] Implement `sendMessage` and receive events
- [x] Handle chatsdk module event callbacks

### Phase 3: Identity & Bundle (Done)
- [x] Implement `getIdentity` → display user info
- [x] Implement `createIntroBundle` → "Generate Intro Bundle" feature
- [x] Implement `newPrivateConversation` → start new chats

### Phase 4: Persistence (Future)
- [ ] Load existing conversations on startup
- [ ] Persist message history

---

## How to Run

### Option 1: Using Nix (Recommended)

```bash
# Build and run the app
nix run '.#app'

# Or build just the library
nix build '.#lib'

# Build the default package (library)
nix build

# Enter development shell
nix develop
```

### Option 2: Manual CMake Build

```bash
cd logos-chatsdk-ui
mkdir build && cd build
cmake .. -GNinja \
  -DLOGOS_CPP_SDK_ROOT=/path/to/logos-cpp-sdk \
  -DLOGOS_LIBLOGOS_ROOT=/path/to/logos-liblogos
ninja

# Run the app
./bin/logos-chatsdk-ui-app
```

The standalone app starts Logos Core, loads `capability_module` then
`chatsdk_module`, and finally loads the `chatsdk_ui` Qt plugin.

---

## Nix Flake Configuration

### Inputs

```nix
inputs = {
  nixpkgs.follows = "logos-liblogos/nixpkgs";
  logos-cpp-sdk.url = "github:logos-co/logos-cpp-sdk";
  logos-liblogos.url = "github:logos-co/logos-liblogos";
  logos-chatsdk-module.url = "git+file:///Users/sirotin/Repositories/logos/logos-chatsdk-module?submodules=1";
  logos-capability-module.url = "github:logos-co/logos-capability-module";
};
```

### Outputs

| Output | Description |
|--------|-------------|
| `packages.${system}.lib` | The plugin library (`chatsdk_ui.dylib`/`.so`) |
| `packages.${system}.app` | The standalone application |
| `packages.${system}.default` | Same as `lib` |
| `devShells.${system}.default` | Development shell with all dependencies |

---

## App Directory Details

The `app/` directory contains a minimal standalone application that:

1. **Initializes Qt** - Creates `QApplication`
2. **Sets up plugins directory** - Points to `../modules` relative to executable
3. **Starts Logos core** - Calls `logos_core_start()`
4. **Loads backend modules** - `capability_module`, then `chatsdk_module`
5. **Loads the chatsdk_ui plugin** - Uses `QPluginLoader` to load the plugin
6. **Creates main window** - Instantiates the plugin widget via `createWidget()`
7. **Runs event loop** - `app.exec()`
8. **Cleans up** - Calls `logos_core_cleanup()` on exit and terminates child processes

This follows the exact same pattern as `logos-chat-ui/app/`.

---

## Implementation Order

1. Create directory structure (`mkdir -p`)
2. Copy interface (`IComponent.h` from logos-chat-ui)
3. Create `MessageBubble` - Simplest widget, no dependencies
4. Create `ConversationListPanel` - Left panel with signals
5. Create `ChatPanel` - Right panel, uses MessageBubble
6. Create `ChatSDKWindow` - Main window, connects panels
7. Create `ChatConfig.h` - Environment-driven chat configuration
8. Create `ChatSDKUIComponent` - Plugin wrapper
9. Create root `CMakeLists.txt` - Build config
10. Create `metadata.json` - Module metadata
11. Create `app/` files - Standalone application
12. Create `nix/` files - Nix build configuration
13. Create `flake.nix` - Nix flake
14. Create `.gitignore` - Utility files
15. Test build and verify UI

---

## Notes

- Conversations and messages are ephemeral (no persistence yet)
- Backend calls are live through `logos-chatsdk-module`
- The module follows the same patterns as `logos-chat-ui` for consistency
- Qt signals/slots are used for component communication to maintain loose coupling
- The standalone app requires `logos-liblogos` for the core runtime
- The plugin can also be loaded by other Logos applications

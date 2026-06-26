# logos-chat-ui

A QML + C++ backend UI module for the [Logos](https://logos.co) platform that provides a private messaging interface built on top of [Logos Chat](https://github.com/logos-messaging/logos-chat).

> **This is the `chat_ui_mix` variant** — **sender-anonymous** messaging routed over the Logos **testnet-0.2 mix fleet** (3-hop Sphinx mixnet + RLN spam protection). It ships **self-contained** (bundled testnet credentials, no setup) — see **[Try it](#try-it)** below.

The UI connects to [`logos-chat-module`](https://github.com/logos-co/logos-chat-module) via the Logos Core module system for all chat operations — identity, conversations, and message exchange happen over the Logos network.

Built with [`logos-module-builder`](https://github.com/logos-co/logos-module-builder) using the `mkLogosQmlModule` pattern (QML frontend + C++ backend with Qt Remote Objects).

## Try it

This build is **self-contained** — it ships with disposable testnet credentials and connects to the live mix fleet automatically. **No config files, no `CHAT_CREDS_DIR`, no bundle to download.**

```bash
nix run 'github:logos-co/logos-chat-ui?ref=feat/logos-testnetv02-mix' --accept-flake-config
```

On launch:

1. **Pick a demo user (1–10)** in the startup popup → **Start**. Each maps to one pre-provisioned RLN membership; your libp2p identity is randomized per run (so the identity isn't tied to the credential).
2. The app **discovers the 5 fleet mix nodes** and loads your credential automatically — watch the bottom status bar fill to **`MIX 4/4`**. The send box enables once the mix pool is ready.
3. **Message anonymously:** in one instance click **My Bundle** and share it; in another, **+ new** → paste the bundle + a first message → create. Messages route **sender-unlinkably** through the 3-hop mixnet (the recipient can't see who sent them).

> **Testnet demo notes:**
> - The bundled credentials are **shared and disposable** — everyone running this build has them. Don't treat them as private.
> - Conversations are **ephemeral** (gone when the app closes).
> - Running **multiple** instances? Pick **different** user indices (1–10) — sharing one index can collide on the RLN per-epoch rate limit.

Once published to [`logos-modules-release`](https://github.com/logos-co/logos-modules-release) it can also be installed into **Logos Basecamp** via the package manager (module id `chat_ui_mix`).

## What It Does

The application provides a two-panel chat interface with a dark, terminal-inspired theme:

- **Conversation list** (left panel) — shows active conversations with timestamps and unread indicators
- **Chat panel** (right panel) — displays messages and a text input for the selected conversation

Core functionality:

- **Identity** — on startup, initializes a chat identity and displays the user's ID in the status bar
- **Intro bundles** — generate your intro bundle ("My Bundle" button) and share it with others to let them start a conversation with you
- **New conversations** — paste another user's intro bundle and an initial message to open a private conversation
- **Messaging** — send and receive messages in real-time over the Logos network
- **Chat lifecycle** — auto-initializes and starts on launch; status shown in the bottom bar

Conversations are **ephemeral** — messages and identity exist only while the app is running.

## How to Run

### Standalone (recommended for development)

```bash
# Run directly
nix run

# With local workspace overrides (if testing local changes)
nix run --override-input chat_module_mix path:../logos-chat-module \
        --override-input chat_module_mix/logos-module-builder path:../logos-module-builder
```

The standalone app starts Logos Core, loads `capability_module` and `chat_module_mix`, then launches the QML UI via an isolated `ui-host` process.

### In Basecamp

Build the `.lgx` package and install it:

```bash
# Build LGX
nix build .#lgx

# Install into Basecamp's plugin directory
lgpm --ui-plugins-dir ~/Library/Application\ Support/Logos/LogosBasecampDev/plugins \
     install --file result/*.lgx
```

Or from the workspace:

```bash
ws bundle logos-chat-ui --auto-local
```

### Build Targets

```bash
nix build            # default — combined plugin + QML output
nix build .#lgx      # .lgx package for distribution
nix build .#install  # lgpm-installed output (modules/ + plugins/)
nix run              # standalone app with chat_module_mix
nix develop          # enter development shell
```

## Module Structure

```
logos-chat-ui/
├── flake.nix                  # mkLogosQmlModule (3-line flake)
├── metadata.json              # Module config (ui_qml type)
├── CMakeLists.txt             # logos_module() macro
└── src/
    ├── ChatBackend.rep        # RemoteObject interface (properties, slots, signals)
    ├── ChatBackend.h/cpp      # Business logic (extends ChatBackendSimpleSource)
    ├── chat_ui_plugin.h/cpp   # Thin plugin entry point
    ├── chat_ui_interface.h    # Plugin interface marker
    ├── ChatConfig.h           # Chat/Waku configuration builder
    ├── ConversationListModel.h/cpp  # QAbstractListModel for conversations
    ├── MessageListModel.h/cpp       # QAbstractListModel for messages
    └── qml/
        └── ChatView.qml      # QML frontend
```

### Key Components

| File | Role |
|------|------|
| `ChatBackend.rep` | Defines the C++/QML boundary — `ChatStatus` enum, state props, lifecycle slots, signals |
| `ChatBackend` | Extends auto-generated `ChatBackendSimpleSource`. Manages chat lifecycle, conversations, messages via `chat_module_mix` events |
| `chat_ui_plugin` | Thin wrapper — creates `ChatBackend` in `initLogos()`, calls `setBackend()` |
| `ConversationListModel` | Roles: `conversationId`, `displayName`, `peerId`, `lastActivity`, `unreadCount` |
| `MessageListModel` | Roles: `sender`, `content`, `timestamp`, `isMe` |

## Requirements

> [!TIP]
> When using Nix, all requirements are acquired automatically.

### Dependencies

| Dependency | Purpose |
|---|---|
| Qt6 Core, RemoteObjects, Declarative | UI framework + IPC |
| [`logos-module-builder`](https://github.com/logos-co/logos-module-builder) | Build system (mkLogosQmlModule) |
| [`logos-chat-module`](https://github.com/logos-co/logos-chat-module) | Chat backend module — the mix variant `chat_module_mix` (`feat/logos-testnetv02-mix` branch) |

## Related Repositories

| Repository | Role |
|---|---|
| [`logos-chat-module`](https://github.com/logos-co/logos-chat-module) | Chat backend — this UI's required dependency (mix variant `chat_module_mix`, `feat/logos-testnetv02-mix`) |
| [`logos-chat`](https://github.com/logos-messaging/logos-chat) | Logos Chat library (provides `liblogoschat`) |
| [`logos-module-builder`](https://github.com/logos-co/logos-module-builder) | Module build system |
| [`logos-liblogos`](https://github.com/logos-co/logos-liblogos) | Logos Core platform |

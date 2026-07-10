# logos-chat-ui

A QML + C++ backend UI module for the [Logos](https://logos.co) platform that provides a private messaging interface built on top of [Logos Chat](https://github.com/logos-messaging/libchat).

The UI connects to [`logos-chat-module`](https://github.com/logos-co/logos-chat-module) via the Logos Core module system for all chat operations — identity, conversations, and message exchange happen over the Logos network.

Built with [`logos-module-builder`](https://github.com/logos-co/logos-module-builder) using the `mkLogosQmlModule` pattern (QML frontend + C++ backend with Qt Remote Objects).

## What It Does

The application provides a terminal-inspired chat interface with a conversation
list (left), a message thread (center), and a members panel (right) that appears
only for group conversations:

- **Conversation list** (left panel) — active conversations with timestamps and unread indicators; group rows are prefixed with `#`
- **Message thread** (center panel) — messages and a text input for the selected conversation; group messages show a short sender label above incoming bubbles
- **Members panel** (right panel, groups only) — the group's roster, with an add-member field

Core functionality:

- **Identity** — on startup, initializes a chat identity and displays the user's ID in the status bar
- **Addresses** — show your address (the **Show My Address** button) and share it with others to let them start a conversation with you
- **New conversations** — paste another user's address to open a private (1:1) conversation
- **Group conversations** — start a group with **+ group**, then invite peers by address from the members panel (see below)
- **Messaging** — send and receive messages in real-time over the Logos network
- **Chat lifecycle** — auto-initializes and starts on launch; status shown in the bottom bar

Conversations are **ephemeral** — messages and identity exist only while the app is running.

### Group conversations

- **+ group** creates a group with you as its only member (no dialog; the group opens immediately).
- Collect peers' addresses (each shares theirs via **Show My Address**), paste one into the members panel's add-member field, and press **+** to invite.
- Membership changes are asynchronous: on devnet the group's steward commits an add only after a ~60s commit-inactivity window, then the welcome is delivered, so a peer joins **minutes** after the invite. The roster refreshes on selection, a message from a new member, the **↻** button, or your own add.
- Any member can add another; the invite routes from whoever proposed it.
- During the brief windows while the group is finalizing a membership change, de-mls rejects sends; these surface as an error toast, so retry after a moment.

## How to Run

### Standalone (recommended for development)

```bash
# Run directly
nix run

# With local workspace overrides (if testing local changes)
nix run --override-input chat_module path:../logos-chat-module \
        --override-input chat_module/logos-module-builder path:../logos-module-builder
```

The standalone app starts Logos Core, loads `capability_module` and `chat_module`, then launches the QML UI via an isolated `ui-host` process.

### Running multiple instances on one machine

To try a real conversation or group locally, run two or more standalone apps
side by side on the same host. Each instance needs its own module data directory
and its own delivery-node port; the UI-to-backend QtRO socket name is randomized
per instance, so nothing else has to be set:

```bash
# window A
CHAT_MODULE_INSTANCE_PATH=~/.local/share/chat_a CHAT_MODULE_DELIVERY_PORT=60000 nix run
# window B
CHAT_MODULE_INSTANCE_PATH=~/.local/share/chat_b CHAT_MODULE_DELIVERY_PORT=60001 nix run
```

Add further windows the same way, giving each a fresh instance path and delivery
port (`chat_c` / `60002`, and so on).

| Variable | Purpose |
|---|---|
| `CHAT_MODULE_INSTANCE_PATH` | The `chat_module` instance's data directory. Two instances must not share one. Defaults to a directory under the app's data location. |
| `CHAT_MODULE_DELIVERY_PORT` | The local delivery (Waku) node's port, bound by the node, so it must be distinct per instance. Defaults to a compiled-in port. |
| `QML_INSPECTOR_PORT` | Only needed when attaching the [logos-qt-mcp](https://github.com/logos-co/logos-qt-mcp) inspector to drive an instance programmatically (default 3768); give each a distinct one then. Interactive use does not need it. |

Each node joins the `logos.test` Waku fleet and publishes its key package during
init, so this needs internet and ~5-20s per window to reach **Online**. Then
share one window's address (**Show My Address**) and paste it into another
(**+ new** for a 1:1, or **+ group** then the members panel for a group). For
the full walkthrough with screenshots, and the scripted drivers that automate it
(`doctests/exchange/run-exchange.sh` for a two-party exchange,
`doctests/group/run-group.sh` for a three-party group), see
[Two-instance message exchange](docs/two-instance-exchange.md).

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
nix run              # standalone app with chat_module
nix develop          # enter development shell
```

## Documentation

- [Two-instance message exchange](docs/two-instance-exchange.md) — two windows
  exchanging encrypted messages end-to-end (with screenshots), plus how to run
  two instances locally.
- Doc-test tutorials — executable walkthroughs that CI runs and publishes as an
  HTML report under `https://logos-co.github.io/logos-chat-ui/`:
  [The Logos Chat UI](doctests/chat-ui.test.yaml) (connect + share your
  address) and
  [Run the automated message-exchange test](doctests/chat-ui-exchange.test.yaml)
  (the real two-party round-trip, captured).

  Enabling the report links is a one-time repo setup: Settings -> Pages ->
  "Deploy from a branch", branch `gh-pages` / `(root)` (the CI publish-report
  job creates the `gh-pages` branch on its first run).

## Module Structure

```
logos-chat-ui/
├── flake.nix                  # mkLogosQmlModule
├── metadata.json              # Module config (ui_qml, interface: universal)
├── CMakeLists.txt             # logos_module() macro
└── src/
    ├── ChatBackend.rep        # QtRO interface (ChatStatus enum, props, slots, signals)
    ├── ChatBackend.h/cpp      # Backend: chat lifecycle, conversations, messages
    ├── ConversationListModel.h/cpp  # QAbstractListModel for conversations
    ├── MessageListModel.h/cpp       # QAbstractListModel for messages
    └── qml/
        └── ChatView.qml       # QML frontend
```

The plugin entry point and QtRO replica/source glue are generated by
`mkLogosQmlModule` from `metadata.json#codegen` (`rep` / `backend_class` /
`backend_header`); the repo carries only the backend, the two models, and the
view.

### Key Components

| File | Role |
|------|------|
| `ChatBackend.rep` | Defines the C++/QML boundary — `ChatStatus` enum, state props, lifecycle slots, signals |
| `ChatBackend` | Derives `ChatBackendSimpleSource` + `LogosUiPluginContext`; initialises the module and subscribes to `chat_module` events in `onContextReady()`; drives the two models |
| `ConversationListModel` | Roles: `conversationId`, `displayName`, `lastActivity`, `unreadCount` |
| `MessageListModel` | Roles: `sender`, `content`, `timestamp`, `isMe` |

## Requirements

> [!TIP]
> When using Nix, all requirements are acquired automatically.

### Dependencies

| Dependency | Purpose |
|---|---|
| Qt6 Core, RemoteObjects, Declarative | UI framework + IPC |
| [`logos-module-builder`](https://github.com/logos-co/logos-module-builder) | Build system (mkLogosQmlModule) |
| [`logos-chat-module`](https://github.com/logos-co/logos-chat-module) | Chat backend module |
| [`logos-delivery-module`](https://github.com/logos-co/logos-delivery-module) | Transport (Waku) — runtime dependency, pinned at v0.1.2 |

## Related Repositories

| Repository | Role |
|---|---|
| [`logos-chat-module`](https://github.com/logos-co/logos-chat-module) | Chat backend — this UI's required dependency |
| [`logos-delivery-module`](https://github.com/logos-co/logos-delivery-module) | Transport (Waku) — runtime dependency, pinned at v0.1.2 |
| [`libchat`](https://github.com/logos-messaging/libchat) | Chat engine embedded by `chat_module` (E2EE, sessions) |
| [`logos-module-builder`](https://github.com/logos-co/logos-module-builder) | Module build system |
| [`logos-liblogos`](https://github.com/logos-co/logos-liblogos) | Logos Core platform |

# logos-chat-ui — `rust-bindings-exploration` branch

> Experimental QML-only chat UI that drives the Rust-port `chat_module`.
> See `master` for the canonical QML+C++-backend implementation.

A QML UI App for [Basecamp](https://github.com/logos-co/logos-basecamp) that
drives [`chat_module`](https://github.com/logos-co/logos-chat-module/tree/rust-bindings-exploration).
Loaded in-process by Basecamp's plugin loader; calls into the chat module via
`logos.callModule(...)`.

## Build

```bash
nix build      # stages plugins/chat_ui/ tree
```

The output (`result/plugins/chat_ui/`) drops into Basecamp's
`--user-dir/plugins/`. The flake derives the manifest's `variant` field from
the host system tuple (e.g. `linux-amd64-dev`).

## Runtime

Requires `chat_module` (built from the matching `rust-bindings-exploration`
branch of `logos-chat-module`) in the same Basecamp `--user-dir/modules/`
directory. The dependency is declared in `metadata.json`.

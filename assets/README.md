# Packaging artwork

`chat-ui.png` (256x256) is the desktop and AppImage icon, `chat-ui.icns` the
macOS bundle icon. `flake.nix` passes both to the module builder, which derives
the rest of the packaging metadata from `metadata.json`.

Both are provisional marks standing in for real artwork. Replace them before
cutting a release; nothing else has to change.

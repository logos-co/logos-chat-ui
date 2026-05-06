{
  description = "logos-chat-ui (rust-bindings-exploration): QML UI App for chat_module";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs, ... }:
    let
      systems = [ "aarch64-darwin" "x86_64-darwin" "aarch64-linux" "x86_64-linux" ];
      forAllSystems = fn: nixpkgs.lib.genAttrs systems fn;

      # Map a Nix system tuple to the variant string Basecamp's package
      # manager keys plugin manifests by (e.g. "linux-amd64-dev"). The
      # `-dev` suffix matches what `logos-module-builder` emits for
      # Nix-pinned dev builds; chat_module uses the same suffix so the
      # two manifests agree.
      systemToVariant = system:
        let parts = nixpkgs.lib.splitString "-" system; in
        let arch = builtins.elemAt parts 0;
            os   = builtins.elemAt parts 1;
            arches = { "x86_64" = "amd64"; "aarch64" = "arm64"; };
        in "${os}-${arches.${arch}}-dev";
    in
    {
      packages = forAllSystems (system:
        let
          pkgs    = nixpkgs.legacyPackages.${system};
          variant = systemToVariant system;

          # Stage QML files into the `plugins/chat_ui/` shape that
          # Basecamp's PluginLoader expects under `--user-dir/plugins/`.
          # No build step: QML files are read at runtime.
          chatUi = pkgs.runCommand "chat_ui" {
            nativeBuildInputs = [ pkgs.jq ];
          } ''
            mkdir -p $out/plugins/chat_ui
            cp ${./metadata.json} $out/plugins/chat_ui/metadata.json
            cp ${./Chat.qml}      $out/plugins/chat_ui/Chat.qml
            cp -r ${./components} $out/plugins/chat_ui/components

            echo "${variant}" > $out/plugins/chat_ui/variant

            # package_manager enumerates plugins by manifest.json (LGX
            # format), not metadata.json alone. The variant string must
            # match the `chat_module` install (both are emitted with
            # the same `-dev` suffix from Nix dev builds).
            jq -n \
              --arg name        "chat_ui" \
              --arg version     "0.1.0" \
              --arg description "Logos Chat — QML UI App for chat_module" \
              --arg category    "messaging" \
              --arg variant     "${variant}" \
              '{
                manifestVersion: "0.2.0",
                name:        $name,
                version:     $version,
                description: $description,
                author:      "Logos Core Team",
                category:    $category,
                icon:        "",
                type:        "ui_qml",
                view:        "Chat.qml",
                "main":      {},
                dependencies: ["chat_module"],
                hashes: {
                  root: "",
                  variants: "",
                  ("variants/" + $variant): ""
                }
              }' > $out/plugins/chat_ui/manifest.json
          '';
        in
        {
          chat_ui = chatUi;
          default = chatUi;
        }
      );
    };
}

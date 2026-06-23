{
  description = "Logos Chat UI - QML view + C++ backend module";

  inputs = {
    # Must be the same builder chat_module consumes, so the
    # logos-protocol/logos-qt-sdk chain matches across both.
    logos-module-builder.url = "github:logos-co/logos-module-builder";
    nix-bundle-lgx.url = "github:logos-co/nix-bundle-lgx";
    # On the unmerged feat/lp-protocol-migration branch. Pinned to an explicit
    # rev (like logos-delivery-module below) instead of a moving branch ref, so
    # the chat_module contract can't shift under a `nix flake update`. Re-pin to
    # the new rev whenever chat_module is pushed; switch to a release tag once
    # the migration merges.
    chat_module.url = "github:logos-co/logos-chat-module/c9242c43ad8e3a98926fdf6b3f45eb53ee8058b8";
    # Pinned to the commit (post-v0.1.2) carrying the zerokit/RLN nix build fix:
    # delivery-module #49 bumps logos-delivery so zerokit's cargo vendor no longer
    # hits crates.io's python-requests 403. Not yet tagged — re-pin to the release
    # tag once one is cut.
    logos-delivery-module.url = "github:logos-co/logos-delivery-module/2577383f6e0de24793b523d6ea4991aa6339afd8";
  };

  outputs = inputs@{ logos-module-builder, logos-delivery-module, ... }:
    let
      base = logos-module-builder.lib.mkLogosQmlModule {
        src = ./.;
        configFile = ./metadata.json;
        flakeInputs = { delivery_module = logos-delivery-module; } // inputs;
      };

      nixpkgs = logos-module-builder.inputs.nixpkgs;

      # `nix run .#exchange`: drive the real two-party message round-trip and hold
      # the receiving window open showing the result. The doc-test launches this
      # to capture one post-exchange screenshot (see doctests/chat-ui.test.yaml);
      # the full flow lives in docs/two-instance-exchange.md. APP_BIN is this
      # flake's standalone runner; the driver scripts are bundled from
      # ./doctests/exchange.
      exchangeApp = system:
        let
          pkgs = import nixpkgs { inherit system; };
          runner = pkgs.writeShellApplication {
            name = "chat-ui-exchange";
            runtimeInputs = with pkgs; [ nodejs coreutils util-linux procps bash ];
            text = ''
              export APP_BIN="${base.apps.${system}.default.program}"
              exec bash ${./doctests/exchange}/run-exchange-show.sh "$@"
            '';
          };
        in {
          type = "app";
          program = "${runner}/bin/chat-ui-exchange";
        };
    in
      base // {
        apps = builtins.mapAttrs
          (system: sysApps: sysApps // { exchange = exchangeApp system; })
          base.apps;
      };
}

{
  description = "Logos Chat UI - QML view + C++ backend module";

  inputs = {
    # Must be the same builder chat_module consumes, so the
    # logos-protocol/logos-qt-sdk chain matches across both.
    logos-module-builder.url = "github:logos-co/logos-module-builder";
    nix-bundle-lgx.url = "github:logos-co/nix-bundle-lgx";
    # Pinned to the master rev carrying the DirectV1 migration (chat-module #37),
    # which isn't in a release tag yet; re-pin to the tag once one is cut. Delivery
    # stays on the v0.1.3 tag below, matching chat_module's own delivery pin.
    chat_module.url = "github:logos-co/logos-chat-module/5c79789041c75bd940bdbc5720a2c9274df09b35";
    # Pinned to the v0.1.3 release tag, which includes the zerokit/RLN nix build
    # fix (delivery-module #49: zerokit's cargo vendor no longer hits crates.io's
    # python-requests 403). Kept in lockstep with chat_module's delivery pin.
    logos-delivery-module.url = "github:logos-co/logos-delivery-module/v0.1.3";
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
      # to capture one post-exchange screenshot (see doctests/chat-ui-exchange.test.yaml);
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

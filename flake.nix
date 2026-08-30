{
  description = "Logos Chat UI - QML view + C++ backend module";

  nixConfig = {
    extra-substituters = [ "https://cache.nix.logos.co/public" ];
    extra-trusted-public-keys = [ "public:l4HrXgL4nw246+LBh2SOJyhz64BoGegOYLheT/iIAPU=" ];
  };

  inputs = {
    # Follow chat_module's own builder, so the logos-protocol/logos-qt-sdk
    # chain matches across both.
    logos-module-builder.follows = "chat_module/logos-module-builder";
    # Pinned past the v0.2.2 release tag, for what no released module carries
    # yet: libchat's GroupV2 join-time fork fix, without which a group splits
    # into branches that never converge again on the first member added after a
    # voted steward election; and the account of a group that stops moving,
    # which is the protocol's own log at debug plus a report of any send the
    # delivery node gave up on.
    chat_module.url = "github:logos-co/logos-chat-module/a4b938d1d3c262b3cf4ba631a649674e1bd04ace";
    # Follow chat_module's delivery pin, so both build against the same
    # delivery module.
    logos-delivery-module.follows = "chat_module/logos-delivery-module";
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
      exchangeRunner = system:
        let pkgs = import nixpkgs { inherit system; };
        in pkgs.writeShellApplication {
          name = "chat-ui-exchange";
          runtimeInputs = with pkgs; [ nodejs coreutils util-linux procps bash ];
          text = ''
            export APP_BIN="${base.apps.${system}.default.program}"
            exec bash ${./doctests/exchange}/run-exchange-show.sh "$@"
          '';
        };

      # `nix run .#group`: form a real three-party group conversation and hold the
      # newest member's window open showing the result. The doc-test launches this
      # to capture one screenshot of the formed group (see
      # doctests/chat-ui-group.test.yaml). APP_BIN is this flake's standalone
      # runner; the driver scripts are bundled from ./doctests/group.
      groupApp = system:
        let
          pkgs = import nixpkgs { inherit system; };
          runner = pkgs.writeShellApplication {
            name = "chat-ui-group";
            runtimeInputs = with pkgs; [ nodejs coreutils util-linux procps bash ];
            text = ''
              export APP_BIN="${base.apps.${system}.default.program}"
              exec bash ${./doctests/group}/run-group-show.sh "$@"
            '';
          };
        in {
          type = "app";
          program = "${runner}/bin/chat-ui-group";
        };
    in
      base // {
        apps = builtins.mapAttrs
          (system: sysApps: sysApps // {
            exchange = { type = "app"; program = "${exchangeRunner system}/bin/chat-ui-exchange"; };
            group = groupApp system;
          })
          base.apps;
        # Also a package so `nix build .#exchange` resolves: the doc-test runner
        # pre-builds its launch target that way to warm the store before the run.
        packages = builtins.mapAttrs
          (system: sysPkgs: sysPkgs // { exchange = exchangeRunner system; })
          base.packages;
      };
}

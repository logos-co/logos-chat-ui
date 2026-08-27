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
    # Pinned to the chat_module commit that moved onto logos-module-builder
    # master (the typed-record codegen this view now consumes) and publishes an
    # x86_64-windows target. A rev rather than a tag only because no release
    # carries either yet -- re-pin to the tag once one is cut, and keep the
    # lockstep the paragraph above describes.
    chat_module.url = "github:logos-co/logos-chat-module/f24fe1917423e33a672f3301fb1e41468ce4d3b1";
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

      # x86_64-windows is a cross PSEUDO-SYSTEM, and only `packages` means
      # anything under it.
      #
      # `apps` does not, and the whole key has to go rather than just the two
      # added below: mkLogosQmlModule's own `apps.<system>.default` resolves the
      # standalone runner against a NATIVE Windows nixpkgs, which nixpkgs
      # refuses outright ("Package … is not available on the requested
      # hostPlatform"). The two doc-test runners fail one layer earlier still --
      # `import nixpkgs { inherit system; }` for this key dies in cc-wrapper
      # ("called without required argument 'runtimeShell'").
      #
      # Neither a standalone runner nor a doc-test means anything on a cross
      # target, so `apps` keeps the native systems only, and `packages` gains
      # `exchange` only where it can be evaluated. `packages.x86_64-windows.*`
      # itself is untouched and is what the Windows build consumes.
      windowsSystem = "x86_64-windows";

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
          (builtins.removeAttrs base.apps [ windowsSystem ]);
        # Also a package so `nix build .#exchange` resolves: the doc-test runner
        # pre-builds its launch target that way to warm the store before the run.
        packages = builtins.mapAttrs
          (system: sysPkgs: sysPkgs // nixpkgs.lib.optionalAttrs
            (system != windowsSystem) { exchange = exchangeRunner system; })
          base.packages;
      };
}

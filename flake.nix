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
    # Pinned to the master rev that reports delivery_adopted: this view and
    # the module it renders are released in lockstep, so re-pin to the release
    # tag once one carries it.
    chat_module.url = "github:logos-co/logos-chat-module/39d6adb74ce674c0ece7238e7c1f270a95aa8e3a";
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

      # `nix run .#scenes -- <out-dir>`: render the scene catalog (tests/scenes),
      # the view in each of its states on a QML stand-in for the backend, to
      # <out-dir>/<scene>.png. Qt, mesa and the fonts come from this nixpkgs and
      # the environment is emptied, so a commit renders the same bytes on any
      # Linux machine. Xvfb and llvmpipe rather than the offscreen platform, under
      # which Qt Quick falls back to its software renderer and the
      # MultiEffect-tinted icons come out blank.
      scenesApp = system:
        let
          pkgs = import nixpkgs { inherit system; };
          qml = pkgs.qt6.qtdeclarative;
          # Only nix-store faces, the generic families pinned to DejaVu: the
          # design system bundles Public Sans but asks for its mono face as
          # `monospace`. The two renames and the fallback to sans-serif are
          # fontconfig's own; makeFontsConf would pull in /etc/fonts/conf.d.
          fontsConf = pkgs.writeText "scenes-fonts.conf" ''
            <?xml version="1.0"?>
            <!DOCTYPE fontconfig SYSTEM "urn:fontconfig:fonts.dtd">
            <fontconfig>
              <dir>${pkgs.dejavu_fonts}/share/fonts</dir>
              <cachedir prefix="xdg">fontconfig</cachedir>
              <match target="pattern">
                <test qual="any" name="family"><string>sans serif</string></test>
                <edit name="family" mode="assign" binding="same"><string>sans-serif</string></edit>
              </match>
              <match target="pattern">
                <test qual="any" name="family"><string>mono</string></test>
                <edit name="family" mode="assign" binding="same"><string>monospace</string></edit>
              </match>
              <match target="pattern">
                <test qual="all" name="family" compare="not_eq"><string>sans-serif</string></test>
                <test qual="all" name="family" compare="not_eq"><string>serif</string></test>
                <test qual="all" name="family" compare="not_eq"><string>monospace</string></test>
                <edit name="family" mode="append_last"><string>sans-serif</string></edit>
              </match>
              <alias binding="strong"><family>monospace</family><prefer><family>DejaVu Sans Mono</family></prefer></alias>
              <alias binding="strong"><family>sans-serif</family><prefer><family>DejaVu Sans</family></prefer></alias>
              <alias binding="strong"><family>serif</family><prefer><family>DejaVu Serif</family></prefer></alias>
            </fontconfig>
          '';
          runner = pkgs.writeShellApplication {
            name = "chat-ui-scenes";
            runtimeInputs = with pkgs; [ coreutils ];
            text = ''
              out="$(realpath -m "''${1:?usage: nix run .#scenes -- <out-dir>}")"
              mkdir -p "$out"
              home="$(mktemp -d)"
              trap 'rm -rf "$home"' EXIT
              cd "$out"
              # QT_HASH_SEED: without it the mono address's glyphs differ between
              # processes.
              env -i HOME="$home" PATH="${pkgs.lib.makeBinPath (with pkgs; [ xvfb-run coreutils ])}" \
                XDG_CACHE_HOME="$home/cache" XDG_CONFIG_HOME="$home/config" XDG_DATA_HOME="$home/data" \
                FONTCONFIG_FILE="${fontsConf}" \
                QT_PLUGIN_PATH="${pkgs.qt6.qtsvg}/${pkgs.qt6.qtbase.qtPluginPrefix}" \
                QT_QPA_PLATFORM=xcb QT_HASH_SEED=0 QT_FORCE_STDERR_LOGGING=1 \
                __GLX_VENDOR_LIBRARY_NAME=mesa LD_LIBRARY_PATH="${pkgs.mesa}/lib" LIBGL_DRIVERS_PATH="${pkgs.mesa}/lib/dri" \
                xvfb-run -a -s "-screen 0 1600x1200x24" \
                "${qml}/bin/qmltestrunner" \
                  -input "${./.}/tests/scenes" \
                  -import "${./.}/src/qml" \
                  -import "${./.}/tests/scenes/mocks" \
                  -import "${logos-module-builder.inputs.logos-design-system}/src/qml" \
                  -import "${qml}/${pkgs.qt6.qtbase.qtQmlPrefix}"
            '';
          };
        in {
          type = "app";
          program = "${runner}/bin/chat-ui-scenes";
        };

      # `nix develop .#tests`: the checks' toolchain on the Qt this module ships
      # with. The QML tools also need the design system's QML source and Qt's
      # own QML modules on their import path: LOGOS_DESIGN_SYSTEM_QML and
      # QT_QML_DIR. nodejs and imagemagick run tests/scenes/compare.mjs.
      testsShell = system:
        let pkgs = import nixpkgs { inherit system; };
        in pkgs.mkShell {
          packages = with pkgs; [ qt6.qtbase qt6.qtdeclarative qt6.qtremoteobjects cmake ninja nodejs imagemagick ];
          LOGOS_DESIGN_SYSTEM_QML = "${logos-module-builder.inputs.logos-design-system}/src/qml";
          QT_QML_DIR = "${pkgs.qt6.qtdeclarative}/${pkgs.qt6.qtbase.qtQmlPrefix}";
        };
    in
      base // {
        apps = builtins.mapAttrs
          (system: sysApps: sysApps // {
            exchange = { type = "app"; program = "${exchangeRunner system}/bin/chat-ui-exchange"; };
            group = groupApp system;
          } // nixpkgs.lib.optionalAttrs (nixpkgs.lib.hasSuffix "-linux" system) {
            # Xvfb and mesa: Linux only.
            scenes = scenesApp system;
          })
          base.apps;
        # Also a package, so `nix build .#exchange` builds the runner without
        # running it.
        packages = builtins.mapAttrs
          (system: sysPkgs: sysPkgs // { exchange = exchangeRunner system; })
          base.packages;
        devShells = builtins.mapAttrs
          (system: sysShells: sysShells // { tests = testsShell system; })
          base.devShells;
      };
}

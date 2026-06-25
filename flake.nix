{
  description = "Logos Chat UI - QML view + C++ backend module";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder";
    nix-bundle-lgx.url = "github:logos-co/nix-bundle-lgx";
    # TODO(testnet-v02-mix): repin to a pushed logos-co/logos-chat-module rev once
    # the feat/logos-testnetv02-mix branch (getMixStatus) is upstreamed.
    chat_module.url = "git+file:///Users/prem/Code/logos-chat-module-canonical?ref=feat/logos-testnetv02-mix";
  };

  outputs = inputs@{ logos-module-builder, ... }:
    logos-module-builder.lib.mkLogosQmlModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;
    };
}

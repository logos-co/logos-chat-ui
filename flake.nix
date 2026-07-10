{
  description = "Logos Chat UI - QML view + C++ backend module";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder";
    nix-bundle-lgx.url = "github:logos-co/nix-bundle-lgx";
    chat_module_mix.url = "git+https://github.com/logos-co/logos-chat-module?ref=feat/logos-testnetv02-mix";
    # logos-chat 6b4d83a: clientMode kad mount (logos-delivery 4a3db364).
    # Explicit input + follows keeps the UI on that chat SHA even if the module
    # lock lags.
    logos-chat.url = "git+https://github.com/logos-messaging/logos-chat?submodules=1&ref=feat/logos-testnetv02-mix";
    chat_module_mix.inputs.logos-chat.follows = "logos-chat";
  };

  outputs = inputs@{ logos-module-builder, ... }:
    logos-module-builder.lib.mkLogosQmlModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;
    };
}

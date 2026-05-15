{
  description = "Logos Chat UI - QML view + C++ backend module";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder";
    nix-bundle-lgx.url = "github:logos-co/nix-bundle-lgx";
    chat_module.url = "github:logos-co/logos-chat-module/rust-bindings-exploration";
    logos-delivery-module.url = "github:logos-co/logos-delivery-module/fix/send-api-qstring";
  };

  outputs = inputs@{ logos-module-builder, logos-delivery-module, ... }:
    logos-module-builder.lib.mkLogosQmlModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = { delivery_module = logos-delivery-module; } // inputs;
    };
}

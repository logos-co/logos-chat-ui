# Builds the logos-chat-ui-app standalone application
{ pkgs, common, src, logosLiblogos, logosSdk, logosChatModule, logosWakuModule, logosCapabilityModule, logosChatUI }:

pkgs.stdenv.mkDerivation rec {
  pname = "logos-chat-ui-app";
  version = common.version;
  
  inherit src;
  inherit (common) buildInputs cmakeFlags meta env;
  
  # Add logosSdk to nativeBuildInputs for logos-cpp-generator
  nativeBuildInputs = common.nativeBuildInputs ++ [ logosSdk ];
  
  # This is a GUI application, enable Qt wrapping
  dontWrapQtApps = false;
  
  # This is an aggregate runtime layout; avoid stripping to prevent hook errors
  dontStrip = true;
  
  configurePhase = ''
    runHook preConfigure
    
    echo "Configuring logos-chat-ui-app..."
    echo "liblogos: ${logosLiblogos}"
    echo "cpp-sdk: ${logosSdk}"
    echo "chat-module: ${logosChatModule}"
    echo "waku-module: ${logosWakuModule}"
    echo "capability-module: ${logosCapabilityModule}"
    echo "chat-ui: ${logosChatUI}"
    
    # Verify that the built components exist
    test -d "${logosLiblogos}" || (echo "liblogos not found" && exit 1)
    test -d "${logosSdk}" || (echo "cpp-sdk not found" && exit 1)
    test -d "${logosChatModule}" || (echo "chat-module not found" && exit 1)
    test -d "${logosWakuModule}" || (echo "waku-module not found" && exit 1)
    test -d "${logosCapabilityModule}" || (echo "capability-module not found" && exit 1)
    test -d "${logosChatUI}" || (echo "chat-ui not found" && exit 1)
    
    cmake -S app -B build \
      -GNinja \
      -DCMAKE_BUILD_TYPE=Release \
      -DLOGOS_LIBLOGOS_ROOT=${logosLiblogos} \
      -DLOGOS_CPP_SDK_ROOT=${logosSdk}
    
    runHook postConfigure
  '';
  
  buildPhase = ''
    runHook preBuild
    
    cmake --build build
    echo "logos-chat-ui-app built successfully!"
    
    runHook postBuild
  '';
  
  installPhase = ''
    runHook preInstall
    
    # Create output directories
    mkdir -p $out/bin $out/lib $out/modules
    
    # Install our app binary
    if [ -f "build/bin/logos-chat-ui-app" ]; then
      cp build/bin/logos-chat-ui-app "$out/bin/"
      echo "Installed logos-chat-ui-app binary"
    fi
    
    # Copy the core binaries from liblogos
    if [ -f "${logosLiblogos}/bin/logoscore" ]; then
      cp -L "${logosLiblogos}/bin/logoscore" "$out/bin/"
      echo "Installed logoscore binary"
    fi
    if [ -f "${logosLiblogos}/bin/logos_host" ]; then
      cp -L "${logosLiblogos}/bin/logos_host" "$out/bin/"
      echo "Installed logos_host binary"
    fi
    
    # Copy required shared libraries from liblogos
    if ls "${logosLiblogos}/lib/"liblogos_core.* >/dev/null 2>&1; then
      cp -L "${logosLiblogos}/lib/"liblogos_core.* "$out/lib/" || true
    fi
    
    # Copy SDK library if it exists
    if ls "${logosSdk}/lib/"liblogos_sdk.* >/dev/null 2>&1; then
      cp -L "${logosSdk}/lib/"liblogos_sdk.* "$out/lib/" || true
    fi

    # Determine platform-specific plugin extension
    OS_EXT="so"
    case "$(uname -s)" in
      Darwin) OS_EXT="dylib";;
      Linux) OS_EXT="so";;
      MINGW*|MSYS*|CYGWIN*) OS_EXT="dll";;
    esac

    # Copy module plugins into the modules directory
    if [ -f "${logosCapabilityModule}/lib/capability_module_plugin.$OS_EXT" ]; then
      cp -L "${logosCapabilityModule}/lib/capability_module_plugin.$OS_EXT" "$out/modules/"
    fi
    if [ -f "${logosWakuModule}/lib/waku_module_plugin.$OS_EXT" ]; then
      cp -L "${logosWakuModule}/lib/waku_module_plugin.$OS_EXT" "$out/modules/"
    fi
    if [ -f "${logosChatModule}/lib/chat_plugin.$OS_EXT" ]; then
      cp -L "${logosChatModule}/lib/chat_plugin.$OS_EXT" "$out/modules/"
    fi
    
    # Copy libwaku library to modules directory (needed by waku_module_plugin)
    if [ -f "${logosWakuModule}/lib/libwaku.$OS_EXT" ]; then
      cp -L "${logosWakuModule}/lib/libwaku.$OS_EXT" "$out/modules/"
    fi

    # Copy chat_ui Qt plugin to root directory (not modules, as it's loaded differently)
    if [ -f "${logosChatUI}/lib/chat_ui.$OS_EXT" ]; then
      cp -L "${logosChatUI}/lib/chat_ui.$OS_EXT" "$out/"
    fi

    # Create a README for reference
    cat > $out/README.txt <<EOF
Logos Chat UI App - Build Information
=====================================
liblogos: ${logosLiblogos}
cpp-sdk: ${logosSdk}
chat-module: ${logosChatModule}
waku-module: ${logosWakuModule}
capability-module: ${logosCapabilityModule}
chat-ui: ${logosChatUI}

Runtime Layout:
- Binary: $out/bin/logos-chat-ui-app
- Libraries: $out/lib
- Modules: $out/modules
- Qt Plugin: $out/chat_ui.$OS_EXT

Usage:
  $out/bin/logos-chat-ui-app
EOF
    
    runHook postInstall
  '';
}

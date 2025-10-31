# Builds the logos-chat-ui-app standalone application
{ pkgs, common, src, logosLiblogos, logosSdk, logosChatModule, logosWakuModule, logosCapabilityModule, logosChatUI }:

pkgs.stdenv.mkDerivation rec {
  pname = "logos-chat-ui-app";
  version = common.version;
  
  inherit src;
  inherit (common) buildInputs cmakeFlags meta env;
  
  # Add logosSdk to nativeBuildInputs for logos-cpp-generator
  nativeBuildInputs = common.nativeBuildInputs ++ [ logosSdk pkgs.patchelf pkgs.removeReferencesTo ];
  
  # Provide Qt/GL runtime paths so the wrapper can inject them
  qtLibPath = pkgs.lib.makeLibraryPath (
    [
      pkgs.qt6.qtbase
      pkgs.qt6.qtremoteobjects
      pkgs.zstd
      pkgs.krb5
      pkgs.zlib
      pkgs.glib
      pkgs.stdenv.cc.cc
      pkgs.freetype
      pkgs.fontconfig
    ]
    ++ pkgs.lib.optionals pkgs.stdenv.isLinux [
      pkgs.libglvnd
      pkgs.mesa.drivers
      pkgs.xorg.libX11
      pkgs.xorg.libXext
      pkgs.xorg.libXrender
      pkgs.xorg.libXrandr
      pkgs.xorg.libXcursor
      pkgs.xorg.libXi
      pkgs.xorg.libXfixes
      pkgs.xorg.libxcb
    ]
  );
  qtPluginPath = "${pkgs.qt6.qtbase}/lib/qt-6/plugins";
  
  # This is a GUI application, enable Qt wrapping
  dontWrapQtApps = false;
  
  # This is an aggregate runtime layout; avoid stripping to prevent hook errors
  dontStrip = true;
  
  # Ensure proper Qt environment setup via wrapper
  qtWrapperArgs = [
    "--prefix" "LD_LIBRARY_PATH" ":" qtLibPath
    "--prefix" "QT_PLUGIN_PATH" ":" qtPluginPath
  ];
  
  preConfigure = ''
    runHook prePreConfigure
    
    # Set macOS deployment target to match Qt frameworks
    export MACOSX_DEPLOYMENT_TARGET=12.0
    
    # Copy logos-cpp-sdk headers to expected location
    echo "Copying logos-cpp-sdk headers for app..."
    mkdir -p ./logos-cpp-sdk/include/cpp
    cp -r ${logosSdk}/include/cpp/* ./logos-cpp-sdk/include/cpp/
    
    # Also copy core headers
    echo "Copying core headers..."
    mkdir -p ./logos-cpp-sdk/include/core
    cp -r ${logosSdk}/include/core/* ./logos-cpp-sdk/include/core/
    
    # Copy SDK library files to lib directory
    echo "Copying SDK library files..."
    mkdir -p ./logos-cpp-sdk/lib
    if [ -f "${logosSdk}/lib/liblogos_sdk.dylib" ]; then
      cp "${logosSdk}/lib/liblogos_sdk.dylib" ./logos-cpp-sdk/lib/
    elif [ -f "${logosSdk}/lib/liblogos_sdk.so" ]; then
      cp "${logosSdk}/lib/liblogos_sdk.so" ./logos-cpp-sdk/lib/
    elif [ -f "${logosSdk}/lib/liblogos_sdk.a" ]; then
      cp "${logosSdk}/lib/liblogos_sdk.a" ./logos-cpp-sdk/lib/
    fi
    
    runHook postPreConfigure
  '';
  
  # Additional environment variables for Qt and RPATH cleanup
  preFixup = ''
    runHook prePreFixup
    
    # Set up Qt environment variables
    export QT_PLUGIN_PATH="${pkgs.qt6.qtbase}/lib/qt-6/plugins"
    export QML_IMPORT_PATH="${pkgs.qt6.qtbase}/lib/qt-6/qml"
    
    # Remove any remaining references to /build/ in binaries and set proper RPATH
    find $out -type f -executable -exec sh -c '
      if file "$1" | grep -q "ELF.*executable"; then
        # Use patchelf to clean up RPATH if it contains /build/
        if patchelf --print-rpath "$1" 2>/dev/null | grep -q "/build/"; then
          echo "Cleaning RPATH for $1"
          patchelf --remove-rpath "$1" 2>/dev/null || true
        fi
        # Set proper RPATH for the main binary
        if echo "$1" | grep -q "/logos-chat-ui-app$"; then
          echo "Setting RPATH for $1"
          patchelf --set-rpath "$out/lib" "$1" 2>/dev/null || true
        fi
      fi
    ' _ {} \;
    
    # Also clean up shared libraries
    find $out -name "*.so" -exec sh -c '
      if patchelf --print-rpath "$1" 2>/dev/null | grep -q "/build/"; then
        echo "Cleaning RPATH for $1"
        patchelf --remove-rpath "$1" 2>/dev/null || true
      fi
    ' _ {} \;
    
    runHook prePostFixup
  '';
  
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
      -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
      -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=FALSE \
      -DCMAKE_INSTALL_RPATH="" \
      -DCMAKE_SKIP_BUILD_RPATH=TRUE \
      -DLOGOS_LIBLOGOS_ROOT=${logosLiblogos} \
      -DLOGOS_CPP_SDK_ROOT=$(pwd)/logos-cpp-sdk
    
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

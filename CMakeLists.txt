cmake_minimum_required(VERSION 3.16)
project(ChatUIPlugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Allow override from environment or command line
if(NOT DEFINED LOGOS_LIBLOGOS_ROOT)
    set(_parent_liblogos "${CMAKE_SOURCE_DIR}/../logos-liblogos")
    set(_use_vendor ${LOGOS_CHAT_UI_USE_VENDOR})
    if(NOT _use_vendor)
        if(NOT EXISTS "${_parent_liblogos}/interface.h")
            set(_use_vendor ON)
        endif()
    endif()
    if(_use_vendor)
        set(LOGOS_LIBLOGOS_ROOT "${CMAKE_SOURCE_DIR}/vendor/logos-liblogos")
    else()
        set(LOGOS_LIBLOGOS_ROOT "${_parent_liblogos}")
    endif()
endif()

if(NOT DEFINED LOGOS_CPP_SDK_ROOT)
    set(_parent_cpp_sdk "${CMAKE_SOURCE_DIR}/../logos-cpp-sdk")
    set(_use_vendor ${LOGOS_CHAT_UI_USE_VENDOR})
    if(NOT _use_vendor)
        if(NOT EXISTS "${_parent_cpp_sdk}/cpp/logos_api.h")
            set(_use_vendor ON)
        endif()
    endif()
    if(_use_vendor)
        set(LOGOS_CPP_SDK_ROOT "${CMAKE_SOURCE_DIR}/vendor/logos-cpp-sdk")
    else()
        set(LOGOS_CPP_SDK_ROOT "${_parent_cpp_sdk}")
    endif()
endif()

# Check if dependencies are available (support both source and installed layouts)
set(_liblogos_found FALSE)
if(EXISTS "${LOGOS_LIBLOGOS_ROOT}/interface.h")
    set(_liblogos_found TRUE)
    set(_liblogos_is_source TRUE)
elseif(EXISTS "${LOGOS_LIBLOGOS_ROOT}/include/interface.h")
    set(_liblogos_found TRUE)
    set(_liblogos_is_source FALSE)
endif()

set(_cpp_sdk_found FALSE)
if(EXISTS "${LOGOS_CPP_SDK_ROOT}/cpp/logos_api.h")
    set(_cpp_sdk_found TRUE)
    set(_cpp_sdk_is_source TRUE)
elseif(EXISTS "${LOGOS_CPP_SDK_ROOT}/include/cpp/logos_api.h")
    set(_cpp_sdk_found TRUE)
    set(_cpp_sdk_is_source FALSE)
endif()

if(NOT _liblogos_found)
    message(FATAL_ERROR "logos-liblogos not found at ${LOGOS_LIBLOGOS_ROOT}. "
                        "Set LOGOS_LIBLOGOS_ROOT or run git submodule update --init --recursive.")
endif()

if(NOT _cpp_sdk_found)
    message(FATAL_ERROR "logos-cpp-sdk not found at ${LOGOS_CPP_SDK_ROOT}. "
                        "Set LOGOS_CPP_SDK_ROOT or run git submodule update --init --recursive.")
endif()

# Find Qt packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets RemoteObjects)

# Get the real path to handle symlinks correctly
get_filename_component(REAL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" REALPATH)

# Try to find the component-interfaces package first
find_package(component-interfaces QUIET)

# If not found, use the local interfaces folder
if(NOT component-interfaces_FOUND)
    # Include the local interfaces directory
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/interfaces)
    
    # Create a component-interfaces library
    add_library(component-interfaces INTERFACE)
    target_include_directories(component-interfaces INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/interfaces)
endif()

# Source files
set(SOURCES
    ChatUIComponent.cpp
    ChatUIComponent.h
    src/ChatWidget.cpp
    src/ChatWidget.h
    src/ChatWindow.cpp
    src/ChatWindow.h
    src/main.cpp
)

# Add SDK sources (only if source layout, installed layout uses the library)
if(_cpp_sdk_is_source)
    list(APPEND SOURCES
        ${LOGOS_CPP_SDK_ROOT}/cpp/logos_api.cpp
        ${LOGOS_CPP_SDK_ROOT}/cpp/logos_api.h
        ${LOGOS_CPP_SDK_ROOT}/cpp/logos_api_client.cpp
        ${LOGOS_CPP_SDK_ROOT}/cpp/logos_api_client.h
        ${LOGOS_CPP_SDK_ROOT}/cpp/logos_api_consumer.cpp
        ${LOGOS_CPP_SDK_ROOT}/cpp/logos_api_consumer.h
        ${LOGOS_CPP_SDK_ROOT}/cpp/logos_api_provider.cpp
        ${LOGOS_CPP_SDK_ROOT}/cpp/logos_api_provider.h
        ${LOGOS_CPP_SDK_ROOT}/cpp/token_manager.cpp
        ${LOGOS_CPP_SDK_ROOT}/cpp/token_manager.h
        ${LOGOS_CPP_SDK_ROOT}/cpp/module_proxy.cpp
        ${LOGOS_CPP_SDK_ROOT}/cpp/module_proxy.h
    )
endif()

# Run Logos C++ generator on metadata before compilation (only for source layout)
set(METADATA_JSON "${CMAKE_CURRENT_SOURCE_DIR}/metadata.json")

# Only run generator for source layout - nix builds already have generated files
if(_cpp_sdk_is_source)
    # Source layout: build and run the generator
    # Generate into build directory
    set(PLUGINS_OUTPUT_DIR "${CMAKE_BINARY_DIR}/generated_code")
    set(CPP_GENERATOR_BUILD_DIR "${LOGOS_CPP_SDK_ROOT}/../build/cpp-generator")
    set(CPP_GENERATOR "${CPP_GENERATOR_BUILD_DIR}/bin/logos-cpp-generator")
    
    if(NOT TARGET cpp_generator_build)
        add_custom_target(cpp_generator_build
            COMMAND bash "${LOGOS_CPP_SDK_ROOT}/cpp-generator/compile.sh"
            WORKING_DIRECTORY "${LOGOS_CPP_SDK_ROOT}/.."
            COMMENT "Building logos-cpp-generator via ${LOGOS_CPP_SDK_ROOT}/cpp-generator/compile.sh"
            VERBATIM
        )
    endif()
    
    add_custom_target(run_cpp_generator_chat_ui
        COMMAND "${CPP_GENERATOR}" --metadata "${METADATA_JSON}" --general-only --output-dir "${PLUGINS_OUTPUT_DIR}"
        WORKING_DIRECTORY "${LOGOS_CPP_SDK_ROOT}/.."
        COMMENT "Running logos-cpp-generator on ${METADATA_JSON} with output-dir ${PLUGINS_OUTPUT_DIR}"
        VERBATIM
    )
    add_dependencies(run_cpp_generator_chat_ui cpp_generator_build)
    
    # Add generated logos_sdk.cpp - will be generated by run_cpp_generator_chat_ui
    list(APPEND SOURCES ${PLUGINS_OUTPUT_DIR}/logos_sdk.cpp)
    # Mark as generated so CMake knows to wait for it
    set_source_files_properties(
        ${PLUGINS_OUTPUT_DIR}/logos_sdk.cpp
        PROPERTIES GENERATED TRUE
    )
else()
    # Installed/nix layout: files are already in source tree from lib.nix preConfigure
    # lib.nix creates ./generated_code in source directory before CMake runs
    set(PLUGINS_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated_code")
    # For installed/nix layout, logos_sdk.cpp is already in ${PLUGINS_OUTPUT_DIR}
    # (generated by nix build preConfigure phase in lib.nix before CMake runs)
    list(APPEND SOURCES ${PLUGINS_OUTPUT_DIR}/logos_sdk.cpp)
endif()

# Create the plugin library
add_library(chat_ui SHARED ${SOURCES})

# Set output name without lib prefix and with correct name for generator
set_target_properties(chat_ui PROPERTIES
    PREFIX ""
    OUTPUT_NAME "chat_ui"
)

# Ensure generator runs before building the plugin (only for source layout)
if(_cpp_sdk_is_source)
    add_dependencies(chat_ui run_cpp_generator_chat_ui)
endif()

# Include directories
target_include_directories(chat_ui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PLUGINS_OUTPUT_DIR}
)

# Add include directories based on layout type
if(_liblogos_is_source)
    target_include_directories(chat_ui PRIVATE ${LOGOS_LIBLOGOS_ROOT})
else()
    target_include_directories(chat_ui PRIVATE ${LOGOS_LIBLOGOS_ROOT}/include)
endif()

if(_cpp_sdk_is_source)
    target_include_directories(chat_ui PRIVATE 
        ${LOGOS_CPP_SDK_ROOT}/cpp
        ${LOGOS_CPP_SDK_ROOT}/cpp/generated
    )
else()
    # For nix builds, also include the include subdirectory
    # (lib.nix moves header files to ./generated_code/include/)
    target_include_directories(chat_ui PRIVATE 
        ${LOGOS_CPP_SDK_ROOT}/include
        ${LOGOS_CPP_SDK_ROOT}/include/cpp
        ${LOGOS_CPP_SDK_ROOT}/include/core
        ${PLUGINS_OUTPUT_DIR}/include
    )
endif()

# Link against libraries
target_link_libraries(chat_ui PRIVATE 
    Qt6::Core
    Qt6::Widgets
    Qt6::RemoteObjects
    component-interfaces
)

# Link SDK library if using installed layout
if(NOT _cpp_sdk_is_source)
    find_library(LOGOS_SDK_LIB logos_sdk PATHS ${LOGOS_CPP_SDK_ROOT}/lib NO_DEFAULT_PATH REQUIRED)
    target_link_libraries(chat_ui PRIVATE ${LOGOS_SDK_LIB})
endif()

# Link against Abseil libraries if found
find_package(absl QUIET)
if(absl_FOUND)
    target_link_libraries(chat_ui PRIVATE
        absl::base
        absl::strings
        absl::log
        absl::check
    )
endif()

# Set common properties for both platforms
set_target_properties(chat_ui PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"  # For Windows .dll
    BUILD_WITH_INSTALL_RPATH TRUE
    SKIP_BUILD_RPATH FALSE)

if(APPLE)
    # macOS specific settings
    set_target_properties(chat_ui PROPERTIES
        INSTALL_RPATH "@loader_path"
        INSTALL_NAME_DIR "@rpath"
        BUILD_WITH_INSTALL_NAME_DIR TRUE)
    
    add_custom_command(TARGET chat_ui POST_BUILD
        COMMAND install_name_tool -id "@rpath/chat_ui.dylib" $<TARGET_FILE:chat_ui>
        COMMENT "Updating library paths for macOS"
    )
else()
    # Linux specific settings
    set_target_properties(chat_ui PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        INSTALL_RPATH_USE_LINK_PATH FALSE)
endif()

install(TARGETS chat_ui
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/logos/modules
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/logos/modules
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/logos/modules
)

install(FILES ${METADATA_JSON}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/logos-chat-ui
)

install(DIRECTORY "${PLUGINS_OUTPUT_DIR}/"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/logos-chat-ui/generated
    OPTIONAL
)

# Print status messages
message(STATUS "Chat UI Plugin configured successfully") 
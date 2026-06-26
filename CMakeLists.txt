cmake_minimum_required(VERSION 3.14)
project(ChatUiPlugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found. Set LOGOS_MODULE_BUILDER_ROOT.")
endif()

# Self-contained fleet creds: when the embedded testnet-0.2 creds are present
# (fleet-creds/), compile them into the plugin via a Qt resource so the app runs
# with no manual CHAT_CREDS_DIR. CHAT_CREDS_DIR still overrides at runtime (dev).
set(CHAT_EMBED_CREDS OFF)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/fleet-creds/rln_tree.db")
    set(CHAT_EMBED_CREDS ON)
    set(CMAKE_AUTORCC ON)
    message(STATUS "chat_ui: embedding fleet creds (fleet_creds.qrc)")
endif()

logos_module(
    NAME chat_ui_mix
    REP_FILE src/ChatBackend.rep
    SOURCES
        src/chat_ui_interface.h
        src/chat_ui_plugin.h
        src/chat_ui_plugin.cpp
        src/ChatBackend.h
        src/ChatBackend.cpp
        src/ChatConfig.h
        src/ConversationListModel.h
        src/ConversationListModel.cpp
        src/MessageListModel.h
        src/MessageListModel.cpp
)

if(CHAT_EMBED_CREDS)
    target_sources(chat_ui_mix_module_plugin PRIVATE fleet_creds.qrc)
    target_compile_definitions(chat_ui_mix_module_plugin PRIVATE EMBEDDED_CREDS)
endif()

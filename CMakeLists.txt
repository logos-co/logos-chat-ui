cmake_minimum_required(VERSION 3.14)
project(ChatUiPlugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found. Set LOGOS_MODULE_BUILDER_ROOT.")
endif()

logos_module(
    NAME chat_ui
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

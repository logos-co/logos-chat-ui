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
        src/ChatBackend.h
        src/ChatBackend.cpp
        src/ConversationListModel.h
        src/ConversationListModel.cpp
        src/MessageListModel.h
        src/MessageListModel.cpp
        src/MemberListModel.h
        src/MemberListModel.cpp
        src/TimeFormat.h
        src/TimeFormat.cpp
        src/Identity.h
        src/Identity.cpp
    INCLUDE_DIRS
        src
)

# Dev-only QML tooling. Registers the pure-QML ChatUi module as a proper Qt QML
# module so the generated all_qmllint target and .qmltypes can check it
# (NO_PLUGIN + linter-only: nothing here is shipped). OFF by default, so the
# build the bundle uses (Qt Core + RemoteObjects, driven by logos_module above)
# never enters this branch and the packaged .lgx is byte-identical. Enable in a
# Qt6 Qml/Quick environment with -DCHAT_UI_QML_DEVTOOLS=ON, and point qmllint at
# the design system via QML_IMPORT_PATH=<app>/lib so `import Logos.Theme`
# resolves. The runtime module identity is the hand-written src/qml/ChatUi/qmldir,
# not the qmldir this target generates.
option(CHAT_UI_QML_DEVTOOLS "Register chat-ui QML for qmllint/qmltypes (dev only, not shipped)" OFF)
if(CHAT_UI_QML_DEVTOOLS)
    find_package(Qt6 REQUIRED COMPONENTS Qml Quick)
    qt_standard_project_setup(REQUIRES 6.5)
    set_source_files_properties(src/qml/ChatUi/ChatTheme.qml
        PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
    qt_add_qml_module(chat_ui_qml_devtools
        URI ChatUi
        VERSION 1.0
        NO_PLUGIN
        QML_FILES
            src/qml/ChatUi/ChatTheme.qml
            src/qml/ChatUi/ChatStore.qml
            src/qml/ChatUi/PaneHeader.qml
            src/qml/ChatUi/ChatIconButton.qml
            src/qml/ChatUi/Avatar.qml
            src/qml/ChatUi/AccountCard.qml
            src/qml/ChatUi/EmptyState.qml
            src/qml/ChatUi/ClipboardProxy.qml
            src/qml/ChatUi/ChatMenuItem.qml
            src/qml/ChatUi/MessageComposer.qml
            src/qml/ChatUi/Toast.qml
            src/qml/ChatUi/MessageDelegate.qml
            src/qml/ChatUi/DayChip.qml
            src/qml/ChatUi/Facepile.qml
            src/qml/ChatUi/ThreadHeader.qml
            src/qml/ChatUi/MessageSkeleton.qml
            src/qml/ChatUi/ConversationDelegate.qml
            src/qml/ChatUi/MemberDelegate.qml
            src/qml/ChatUi/ConversationsPane.qml
            src/qml/ChatUi/MessageThreadPane.qml
            src/qml/ChatUi/MembersPane.qml
            src/qml/ChatUi/NewConversationDialog.qml
            src/qml/ChatUi/NewGroupDialog.qml
            src/qml/ChatUi/MemberAddInfoDialog.qml
            src/qml/ChatUi/GroupInfoDialog.qml
            src/qml/ChatUi/DmInfoDialog.qml
            src/qml/ChatUi/AddMemberDialog.qml
            src/qml/ChatUi/SelectableText.qml
    )
endif()

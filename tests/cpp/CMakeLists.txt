cmake_minimum_required(VERSION 3.16)
project(ChatUiBackendTests LANGUAGES CXX)

# Standalone on purpose: the plugin itself is built by the module builder's
# macro against the Logos SDK, which a test run has no use for. The backend is
# built against doubles of the SDK headers it includes (fakes/), so everything
# under test here depends on Qt alone.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core RemoteObjects Test)
enable_testing()

add_executable(tst_sessionlogfiles
    tst_sessionlogfiles.cpp
    ../../src/SessionLogFiles.cpp
)
target_include_directories(tst_sessionlogfiles PRIVATE ../../src)
target_link_libraries(tst_sessionlogfiles PRIVATE Qt6::Core Qt6::Test)
add_test(NAME sessionlogfiles COMMAND tst_sessionlogfiles)

add_executable(tst_runlog
    tst_runlog.cpp
    ../../src/RunLog.cpp
    ../../src/SessionLogFiles.cpp
)
target_include_directories(tst_runlog PRIVATE ../../src)
target_link_libraries(tst_runlog PRIVATE Qt6::Core Qt6::Test)
add_test(NAME runlog COMMAND tst_runlog)

add_executable(tst_errorlog
    tst_errorlog.cpp
    ../../src/ErrorLog.cpp
)
target_include_directories(tst_errorlog PRIVATE ../../src)
target_link_libraries(tst_errorlog PRIVATE Qt6::Core Qt6::Test)
add_test(NAME errorlog COMMAND tst_errorlog)

add_executable(tst_chatbackend
    tst_chatbackend.cpp
    ../../src/ChatBackend.cpp
    ../../src/ConversationListModel.cpp
    ../../src/ErrorLog.cpp
    ../../src/Identity.cpp
    ../../src/MemberListModel.cpp
    ../../src/MessageListModel.cpp
    ../../src/ProcessLog.cpp
    ../../src/RunLog.cpp
    ../../src/SessionLogFiles.cpp
    ../../src/TimeFormat.cpp
)
qt6_add_repc_sources(tst_chatbackend ../../src/ChatBackend.rep)
target_include_directories(tst_chatbackend PRIVATE fakes ../../src ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(tst_chatbackend PRIVATE Qt6::Core Qt6::RemoteObjects Qt6::Test)
add_test(NAME chatbackend COMMAND tst_chatbackend)

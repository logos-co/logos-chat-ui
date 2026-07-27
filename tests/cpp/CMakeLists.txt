cmake_minimum_required(VERSION 3.16)
project(ChatUiBackendTests LANGUAGES CXX)

# Standalone on purpose: the plugin itself is built by the module builder's
# macro against the Logos SDK, which a test run has no use for. Everything under
# test here depends on Qt Core alone.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test)
enable_testing()

add_executable(tst_logline
    tst_logline.cpp
    ../../src/LogLine.cpp
)
target_include_directories(tst_logline PRIVATE ../../src)
target_link_libraries(tst_logline PRIVATE Qt6::Core Qt6::Test)
add_test(NAME logline COMMAND tst_logline)

add_executable(tst_sessionlogmodel
    tst_sessionlogmodel.cpp
    ../../src/SessionLogModel.cpp
    ../../src/LogLine.cpp
)
target_include_directories(tst_sessionlogmodel PRIVATE ../../src)
target_link_libraries(tst_sessionlogmodel PRIVATE Qt6::Core Qt6::Test)
add_test(NAME sessionlogmodel COMMAND tst_sessionlogmodel)

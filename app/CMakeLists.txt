cmake_minimum_required(VERSION 3.16)
project(LogosChatUIApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Find Qt packages
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)

# Find logos-liblogos
if(NOT DEFINED LOGOS_LIBLOGOS_ROOT)
    message(FATAL_ERROR "LOGOS_LIBLOGOS_ROOT must be defined")
endif()

# Find logos-cpp-sdk
if(NOT DEFINED LOGOS_CPP_SDK_ROOT)
    message(FATAL_ERROR "LOGOS_CPP_SDK_ROOT must be defined")
endif()

message(STATUS "Using logos-liblogos at: ${LOGOS_LIBLOGOS_ROOT}")
message(STATUS "Using logos-cpp-sdk at: ${LOGOS_CPP_SDK_ROOT}")

# Check if logos_sdk library exists
if(NOT EXISTS "${LOGOS_CPP_SDK_ROOT}/lib/liblogos_sdk.a" AND NOT EXISTS "${LOGOS_CPP_SDK_ROOT}/lib/liblogos_sdk.dylib" AND NOT EXISTS "${LOGOS_CPP_SDK_ROOT}/lib/liblogos_sdk.so")
    message(WARNING "logos_sdk library not found in ${LOGOS_CPP_SDK_ROOT}/lib/")
    message(STATUS "Available files in ${LOGOS_CPP_SDK_ROOT}/lib/:")
    file(GLOB SDK_LIB_FILES "${LOGOS_CPP_SDK_ROOT}/lib/*")
    foreach(file ${SDK_LIB_FILES})
        message(STATUS "  ${file}")
    endforeach()
endif()

# Include directories - the new structure has headers in /include with subdirectories
include_directories(
    ${LOGOS_LIBLOGOS_ROOT}/include
    ${LOGOS_CPP_SDK_ROOT}/include
    ${LOGOS_CPP_SDK_ROOT}/include/cpp
    ${LOGOS_CPP_SDK_ROOT}/include/core
)

# Link directories
link_directories(
    ${LOGOS_LIBLOGOS_ROOT}/lib
    ${LOGOS_CPP_SDK_ROOT}/lib
)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Create the executable
add_executable(logos-chat-ui-app
    main.cpp
    mainwindow.cpp
    mainwindow.h
    resources.qrc
)

# Link libraries
target_link_libraries(logos-chat-ui-app PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    logos_core
    logos_sdk
)

# Set RPATH settings for the executable
if(APPLE)
    set_target_properties(logos-chat-ui-app PROPERTIES
        INSTALL_RPATH "@executable_path/../lib"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
elseif(UNIX)
    set_target_properties(logos-chat-ui-app PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

# Install rules
install(TARGETS logos-chat-ui-app
    RUNTIME DESTINATION bin
)

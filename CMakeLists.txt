cmake_minimum_required(VERSION 3.16)
project(YouCpp VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Core Gui WebEngineWidgets)

qt_standard_project_setup()

# Define source files
set(PROJECT_SOURCES
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/TranscriptWindow.cpp
    src/ui/TranscriptWindow.h
    src/backend/YouTubeService.cpp
    src/backend/YouTubeService.h
    src/backend/GoogleAuth.cpp
    src/backend/GoogleAuth.h
)

add_executable(YouCpp ${PROJECT_SOURCES})

target_link_libraries(YouCpp PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Core
    Qt6::Gui
    Qt6::WebEngineWidgets
)


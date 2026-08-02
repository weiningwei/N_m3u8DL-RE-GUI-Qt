cmake_minimum_required(VERSION 3.19)

project(N_m3u8DL_RE_GUI_Qt
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt6 location is provided by CMAKE_PREFIX_PATH at configure time.
find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

add_executable(N_m3u8DL_RE_GUI_Qt
    src/main.cpp
    src/options.h
    src/options.cpp
    src/stringlistwidget.h
    src/stringlistwidget.cpp
    src/mainwindow.h
    src/mainwindow.cpp
)

target_link_libraries(N_m3u8DL_RE_GUI_Qt PRIVATE Qt6::Widgets)

# Ensure UTF-8 source/literal handling with the MinGW/Clang toolchain.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(N_m3u8DL_RE_GUI_Qt PRIVATE -finput-charset=utf-8)
endif()

# Windows: build a GUI subsystem executable with an icon-less manifest.
if(WIN32)
    set_target_properties(N_m3u8DL_RE_GUI_Qt PROPERTIES
        WIN32_EXECUTABLE ON
    )
endif()

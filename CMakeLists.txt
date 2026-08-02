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
    include/mainwindow.h
    include/options.h
    include/stringlistwidget.h
    src/main.cpp
    src/options.cpp
    src/stringlistwidget.cpp
    src/mainwindow.cpp
    src/mainwindow_ui.cpp
    src/mainwindow_command.cpp
    src/mainwindow_task.cpp
    src/mainwindow_clipboard.cpp
    src/mainwindow_settings.cpp
    src/mainwindow_paths.cpp
    resources/app_icon.qrc
    resources/app_icon.png
)

target_include_directories(N_m3u8DL_RE_GUI_Qt PRIVATE include)

target_link_libraries(N_m3u8DL_RE_GUI_Qt PRIVATE Qt6::Widgets)

# 拷贝 third_party 目录到构建目录（存放 N_m3u8DL-RE.exe / ffmpeg.exe 等第三方工具）
if(EXISTS "${CMAKE_SOURCE_DIR}/third_party")
    add_custom_command(TARGET N_m3u8DL_RE_GUI_Qt POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/third_party"
            "$<TARGET_FILE_DIR:N_m3u8DL_RE_GUI_Qt>/third_party"
        COMMENT "Copying third_party/ to build directory"
    )
endif()

# 拷贝资源文件到构建目录，确保运行时可访问
add_custom_command(TARGET N_m3u8DL_RE_GUI_Qt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/resources/app_icon.png"
        "$<TARGET_FILE_DIR:N_m3u8DL_RE_GUI_Qt>/app_icon.png"
    COMMENT "Copying app_icon.png to build directory"
)

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

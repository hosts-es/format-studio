cmake_minimum_required(VERSION 3.16)

project(format-studio VERSION 0.2.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Svg Network)

qt_standard_project_setup(REQUIRES 6.5)

if(WIN32)
    enable_language(RC)
endif()

# Theme.qml is a QML singleton (pragma Singleton)
set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_executable(formatstudio
    main.cpp
    converterbackend.cpp
    converterbackend.h
    taskmodel.cpp
    taskmodel.h
    iconprovider.h
    githubservice.cpp
    githubservice.h
)

# 版本号只维护 project(... VERSION ...) 一处，C++ 通过这个宏读取。
target_compile_definitions(formatstudio PRIVATE
    FORMATSTUDIO_VERSION="${PROJECT_VERSION}"
)

# Windows 可执行文件图标 + 版本信息。
# 说明：项目路径含空格，CMake 的 RC 编译规则会把 -I 参数拆坏（windres 报 "No such file"），
# 所以这里手工调用 windres，并让它在源码目录用相对路径工作。
if(WIN32)
    set(FS_VERSION_COMMA "${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/app.rc.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/app.rc"
                   @ONLY)

    set(FS_ICON_OBJ "${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc.o")
    add_custom_command(
        OUTPUT "${FS_ICON_OBJ}"
        COMMAND "${CMAKE_RC_COMPILER}" -O coff -i "${CMAKE_CURRENT_BINARY_DIR}/app.rc" -o "${FS_ICON_OBJ}"
        DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/app.rc"
                "${CMAKE_CURRENT_SOURCE_DIR}/app.rc.in"
                "${CMAKE_CURRENT_SOURCE_DIR}/icons/app.ico"
        COMMENT "Compiling application icon and version info"
        VERBATIM)
    target_sources(formatstudio PRIVATE "${FS_ICON_OBJ}")
endif()

qt_add_resources(formatstudio "icons"
    PREFIX "/icons"
    BASE "icons"
    FILES
        icons/all.svg
        icons/alert.svg
        icons/app.ico
        icons/app.png
        icons/check.svg
        icons/chevron-down.svg
        icons/chevron-left.svg
        icons/clock.svg
        icons/close.svg
        icons/doc.svg
        icons/download.svg
        icons/folder.svg
        icons/gauge.svg
        icons/github.svg
        icons/image.svg
        icons/info.svg
        icons/layers.svg
        icons/lock.svg
        icons/moon.svg
        icons/music.svg
        icons/pause.svg
        icons/play.svg
        icons/plus.svg
        icons/refresh.svg
        icons/settings.svg
        icons/sparkles.svg
        icons/stop.svg
        icons/sun.svg
        icons/trash.svg
        icons/video.svg
)

qt_add_qml_module(formatstudio
    URI FormatStudio
    VERSION 1.0
    QML_FILES
        Main.qml
        Theme.qml
        Icon.qml
        IconButton.qml
        ThemeSwitch.qml
        StudioButton.qml
        NavButton.qml
        FormatChip.qml
        TaskCard.qml
        DropZone.qml
        Toast.qml
        EmptyState.qml
        Segmented.qml
        ConvertPage.qml
        SettingsPage.qml
        AboutPage.qml
        FeedbackPage.qml
        DocsPage.qml
)

set_target_properties(formatstudio PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(formatstudio
    PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Svg Qt6::Network
)

# Optional: bundle ffmpeg/ffprobe next to the executable for a self-contained app.
#   cmake -DFS_FFMPEG_DIR="D:/ffmpeg/bin" ..
set(FS_FFMPEG_DIR "" CACHE PATH "Directory containing ffmpeg.exe / ffprobe.exe to bundle")
if(EXISTS "${FS_FFMPEG_DIR}/ffmpeg.exe")
    file(GLOB FS_FFMPEG_FILES "${FS_FFMPEG_DIR}/*.exe" "${FS_FFMPEG_DIR}/*.dll")
    add_custom_command(TARGET formatstudio POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FS_FFMPEG_FILES} "$<TARGET_FILE_DIR:formatstudio>"
        COMMENT "Bundling ffmpeg runtime next to formatstudio"
    )
    install(FILES ${FS_FFMPEG_FILES} DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

install(TARGETS formatstudio
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

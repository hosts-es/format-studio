cmake_minimum_required(VERSION 3.16)

project(format-studio VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Svg Network)

qt_standard_project_setup(REQUIRES 6.5)

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

# Explicit list (no glob): the resource generation rules must be identical on
# every configure, otherwise a stale build tree ends up without a rule for
# qrc_icons.cpp and make fails with "No rule to make target".
qt_add_resources(formatstudio "icons"
    PREFIX "/icons"
    BASE "icons"
    FILES
        icons/all.svg
        icons/alert.svg
        icons/check.svg
        icons/chevron-down.svg
        icons/clock.svg
        icons/close.svg
        icons/chevron-left.svg
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
#   cmake -DFS_FFMPEG_DIR="D:/format studio/format-studio" ..
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

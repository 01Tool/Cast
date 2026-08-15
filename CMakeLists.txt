cmake_minimum_required(VERSION 3.13)
project(ot-cast VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_APP "Build the DTK desktop app (needs Dtk6)" ON)
option(BUILD_CHECKS "Build protocol unit checks" ON)

if(BUILD_APP)
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets DBus Network LinguistTools)
    find_package(Dtk6Core REQUIRED)
    find_package(Dtk6Gui REQUIRED)
    find_package(Dtk6Widget REQUIRED)
elseif(BUILD_CHECKS)
    find_package(Qt6 REQUIRED COMPONENTS Core)
endif()

if(BUILD_APP)
add_executable(ot-cast
    src/main.cpp
    src/ui/mainwindow.cpp
    src/engine/castengine.cpp
    src/discovery/p2pdiscovery.cpp
    src/session/p2psession.cpp
    src/session/nmsecretagent.cpp
    src/session/wfdserver.cpp
    src/session/wfdvideomode.cpp
    src/session/wfdaudiomode.cpp
    src/session/gstencoder.cpp
    src/capture/x11capture.cpp
    src/capture/portalcapture.cpp
)

target_include_directories(ot-cast PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ot-cast PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::DBus
    Qt6::Network
    Dtk6::Core
    Dtk6::Gui
    Dtk6::Widget
)

set(TS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/ot-cast_zh_CN.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/ot-cast_zh_TW.ts
)
qt6_add_translation(QM_FILES ${TS_FILES})
add_custom_target(ot-cast-translations ALL DEPENDS ${QM_FILES})
add_dependencies(ot-cast ot-cast-translations)
add_custom_command(TARGET ot-cast POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:ot-cast>/translations
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${QM_FILES}
            $<TARGET_FILE_DIR:ot-cast>/translations
)
install(FILES ${QM_FILES} DESTINATION share/ot-cast/translations)
install(FILES data/com.01tool.cast.desktop DESTINATION share/applications)
install(FILES data/icons/hicolor/scalable/apps/ot-cast.svg
        DESTINATION share/icons/hicolor/scalable/apps)
endif()

if(BUILD_CHECKS)
add_executable(wfdvideomode-check
    src/session/wfdvideomode.cpp
    tests/wfdvideomode-check.cpp
)
target_include_directories(wfdvideomode-check PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(wfdvideomode-check PRIVATE Qt6::Core)

add_executable(wfdaudiomode-check
    src/session/wfdaudiomode.cpp
    tests/wfdaudiomode-check.cpp
)
target_include_directories(wfdaudiomode-check PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(wfdaudiomode-check PRIVATE Qt6::Core)

add_executable(displaysource-check
    tests/displaysource-check.cpp
)
target_include_directories(displaysource-check PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(displaysource-check PRIVATE Qt6::Core)
endif()

if(BUILD_APP)
install(TARGETS ot-cast RUNTIME DESTINATION bin)
endif()

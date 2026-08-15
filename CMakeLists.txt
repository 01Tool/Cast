cmake_minimum_required(VERSION 3.13)
project(deepin-miracast VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets DBus Network LinguistTools)
find_package(Dtk6Core REQUIRED)
find_package(Dtk6Gui REQUIRED)
find_package(Dtk6Widget REQUIRED)

add_executable(deepin-miracast
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

target_include_directories(deepin-miracast PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(deepin-miracast PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::DBus
    Qt6::Network
    Dtk6::Core
    Dtk6::Gui
    Dtk6::Widget
)

set(TS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/deepin-miracast_zh_CN.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/deepin-miracast_zh_TW.ts
)
qt6_add_translation(QM_FILES ${TS_FILES})
add_custom_target(deepin-miracast-translations ALL DEPENDS ${QM_FILES})
add_dependencies(deepin-miracast deepin-miracast-translations)
add_custom_command(TARGET deepin-miracast POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:deepin-miracast>/translations
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${QM_FILES}
            $<TARGET_FILE_DIR:deepin-miracast>/translations
)
install(FILES ${QM_FILES} DESTINATION share/deepin-miracast/translations)

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

install(TARGETS deepin-miracast RUNTIME DESTINATION bin)

project(startactive)

FIND_PACKAGE(Qt4 REQUIRED)
FIND_PACKAGE(KDE4 REQUIRED)

include_directories(
    ${QT_INCLUDES}
    ${KDE4_INCLUDES}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_subdirectory(splash/themes)
add_subdirectory(modules)

set(startactive_SRC
    StartActive.cpp
    SignalListener.cpp
    ProcessStarter.cpp
    splash/SplashWindow.cpp
    main.cpp
)

set(MODULE_PATH "${DATA_INSTALL_DIR}/startactive/modules/")
set(KDE_DATADIR "${DATA_INSTALL_DIR}")
set(KDE_DEFAULT_HOME ".kde")

message(${MODULE_PATH})

configure_file(
    config-startactive.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/config-startactive.h
)

qt4_add_dbus_adaptor(
    startactive_SRC
    org.kde.StartActive.xml
    StartActive.h StartActive
)

kde4_add_executable(
    startactive
    ${startactive_SRC}
)

target_link_libraries(
    startactive
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${X11_LIBRARIES}
    ${QT_QTDECLARATIVE_LIBRARY}
    ${QT_QTGUI_LIBRARY}

    # ${QT_QTSCRIPT_LIBRARY}
    # ${KDE4_KDECORE_LIBS}
)

install(
    TARGETS
    startactive
    ${INSTALL_TARGETS_DEFAULT_ARGS}
)


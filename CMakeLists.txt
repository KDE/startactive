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
add_subdirectory(systemd)

set(startactive_SRC
    StartActive.cpp
    SignalListener.cpp
    ProcessStarter.cpp
    splash/SplashWindow.cpp
    main.cpp
)

set(KDE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(MODULE_PATH "${DATA_INSTALL_DIR}/startactive/modules/")
set(STARTACTIVE_DATA_PATH "${DATA_INSTALL_DIR}/startactive/")
set(KDE_DATADIR "${DATA_INSTALL_DIR}")
set(KDE_DEFAULT_HOME ".kde")

message(${MODULE_PATH})

configure_file(
    config-startactive.h.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/config-startactive.h
)

configure_file(
    env.conf.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/env.conf
)

qt4_add_dbus_adaptor(
    startactive_SRC
    org.kde.StartActive.xml
    StartActive.h StartActive
)

kde4_add_executable(
    startactive.bin
    ${startactive_SRC}
)

target_link_libraries(
    startactive.bin
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${X11_LIBRARIES}
    ${QT_QTDECLARATIVE_LIBRARY}
    ${QT_QTGUI_LIBRARY}

    # ${QT_QTSCRIPT_LIBRARY}
    # ${KDE4_KDECORE_LIBS}
)

message(${INSTALL_TARGETS_DEFAULT_ARGS})

install(
    TARGETS
    startactive.bin
    ${INSTALL_TARGETS_DEFAULT_ARGS}
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/env.conf
    DESTINATION ${STARTACTIVE_DATA_PATH}
)

install(
    FILES startactive
    DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
    PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

install(
    FILES setup-kde-skel
    DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
    PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

cmake_minimum_required (VERSION 2.8.12 FATAL_ERROR)

project (StartPlasma)

include(FeatureSummary)

# Extra CMake Modules
find_package (ECM 5.18.0 REQUIRED NO_MODULE)
set_package_properties (ECM PROPERTIES TYPE REQUIRED DESCRIPTION "Extra CMake Modules." URL "https://projects.kde.org/projects/kdesupport/extra-cmake-modules")
feature_summary (WHAT REQUIRED_PACKAGES_NOT_FOUND FATAL_ON_MISSING_REQUIRED_PACKAGES)
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

# Including modules we want to use
include (KDEInstallDirs)
include (KDECMakeSettings)
include (KDECompilerSettings NO_POLICY_SCOPE)
include (ECMGenerateHeaders)

# We need Qt
set (QT_MIN_VERSION "5.6.1")
find_package (Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Core DBus)

# And we also need a few KF5 things for the splash screen
set (KF5_MIN_VERSION "5.24.0")
find_package (KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS Package Declarative Config)

set (CMAKE_INCLUDE_CURRENT_DIR ON)

if (IS_ABSOLUTE ${KDE_INSTALL_DATADIR_KF5})
   set (STARTPLASMA_DATA_DIR "${KDE_INSTALL_DATADIR_KF5}/startplasma/")
   set (STARTPLASMA_MODULE_DIR "${KDE_INSTALL_DATADIR_KF5}/startplasma/modules")
else ()
   set (STARTPLASMA_DATA_DIR "${CMAKE_INSTALL_PREFIX}/${KDE_INSTALL_DATADIR_KF5}/startplasma/")
   set (STARTPLASMA_MODULE_DIR "${CMAKE_INSTALL_PREFIX}/${KDE_INSTALL_DATADIR_KF5}/startplasma/modules")
endif()


# add_subdirectory (splash/themes)
add_subdirectory (modules)
# add_subdirectory (systemd)

set (
   startplasma_SRC
   StartPlasma.cpp
   SignalListener.cpp
   ModuleStarter.cpp
   Modules.cpp
   splash/SplashWindow.cpp
   main.cpp
   )

configure_file (
   config-startplasma.h.cmake
   ${CMAKE_CURRENT_BINARY_DIR}/config-startplasma.h
   )

configure_file (
   env.conf.cmake
   ${CMAKE_CURRENT_BINARY_DIR}/env.conf
   )

qt5_add_dbus_adaptor (
   startplasma_SRC
   org.kde.StartPlasma.xml
   StartPlasma.h StartPlasma
   )

add_executable (
   startplasma.bin
   ${startplasma_SRC}
   )

target_link_libraries (
   startplasma.bin
   Qt5::Core
   Qt5::DBus
   KF5::QuickAddons
   KF5::Package
   KF5::ConfigCore
   )

install (
   TARGETS
   startplasma.bin
   ${KF5_INSTALL_TARGETS_DEFAULT_ARGS}
   )

install (
   FILES ${CMAKE_CURRENT_BINARY_DIR}/env.conf
   DESTINATION ${STARTPLASMA_DATA_DIR}
   )

install (
   FILES startplasma
   DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
   PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
   )

install (
   FILES setup-kde-skel
   DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
   PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
   )

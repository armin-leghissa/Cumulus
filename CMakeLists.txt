cmake_minimum_required(VERSION 3.16)

project(plasma-cumulus VERSION 1.0)

set(QT_MIN_VERSION "6.7.0")
set(KF6_MIN_VERSION "6.0.0")

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

# Install targets (docs/decisions.md D9). The applet is a Plasma package (QML files plus a C++ QML module
# inside the package), so the same layout works in the user's home and system-wide:
#   install-user / uninstall-user      <CUMULUS_USER_PREFIX>/share/plasma/plasmoids/<id>, no root
#   install-system / uninstall-system  <CUMULUS_SYSTEM_PREFIX>/share/plasma/plasmoids/<id>, via pkexec
#   install / uninstall                follow CUMULUS_INSTALL_MODE (default USER)
# All of them restart plasmashell afterwards when CUMULUS_RESTART_PLASMASHELL is ON.
set(CUMULUS_INSTALL_MODE "USER" CACHE STRING "What the plain install/uninstall targets do: USER or SYSTEM")
set_property(CACHE CUMULUS_INSTALL_MODE PROPERTY STRINGS USER SYSTEM)
set(CUMULUS_USER_PREFIX "$ENV{HOME}/.local" CACHE PATH "Install prefix of install-user")
set(CUMULUS_SYSTEM_PREFIX "/usr" CACHE PATH "Install prefix of install-system")
option(CUMULUS_RESTART_PLASMASHELL "Restart plasmashell after installing or uninstalling" ON)

if(CUMULUS_INSTALL_MODE STREQUAL "USER")
    set(CMAKE_INSTALL_PREFIX "${CUMULUS_USER_PREFIX}" CACHE PATH "Install prefix, set from CUMULUS_INSTALL_MODE" FORCE)
elseif(CUMULUS_INSTALL_MODE STREQUAL "SYSTEM")
    set(CMAKE_INSTALL_PREFIX "${CUMULUS_SYSTEM_PREFIX}" CACHE PATH "Install prefix, set from CUMULUS_INSTALL_MODE" FORCE)
else()
    message(FATAL_ERROR "CUMULUS_INSTALL_MODE must be USER or SYSTEM, not '${CUMULUS_INSTALL_MODE}'")
endif()

set(KDE_SKIP_UNINSTALL_TARGET ON) # own uninstall targets below

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FeatureSummary)

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Network
    Qml
)

# runtime dependencies of the QML files
find_package(Plasma ${KF6_MIN_VERSION} CONFIG)
set_package_properties(Plasma PROPERTIES TYPE RUNTIME)
find_package(KF6Kirigami ${KF6_MIN_VERSION} CONFIG)
set_package_properties(KF6Kirigami PROPERTIES TYPE RUNTIME)
find_package(KF6QuickCharts ${KF6_MIN_VERSION} CONFIG)
set_package_properties(KF6QuickCharts PROPERTIES TYPE RUNTIME)
find_package(KF6KCMUtils ${KF6_MIN_VERSION} CONFIG)
set_package_properties(KF6KCMUtils PROPERTIES TYPE RUNTIME)

# Package layout, in the build tree (for plasmoidviewer -a <dir>) and when installed
set(CUMULUS_PLUGIN_ID "io.github.leghissa.cumulus")
set(CUMULUS_PACKAGE_BUILD_DIR "${CMAKE_BINARY_DIR}/package")
set(CUMULUS_MODULE_SUBDIR "contents/lib/io/github/leghissa/cumulus")
set(CUMULUS_PACKAGE_INSTALL_DIR "${KDE_INSTALL_DATADIR}/plasma/plasmoids/${CUMULUS_PLUGIN_ID}")

add_custom_target(cumulus_package ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/package" "${CUMULUS_PACKAGE_BUILD_DIR}"
    COMMENT "Assembling applet package in ${CUMULUS_PACKAGE_BUILD_DIR}"
    VERBATIM
)
install(DIRECTORY package/ DESTINATION "${CUMULUS_PACKAGE_INSTALL_DIR}")

add_subdirectory(src)

if(BUILD_TESTING)
    find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Test)
    add_subdirectory(autotests)
endif()

# Install and uninstall targets
set(CUMULUS_RESTART_SCRIPT "${CMAKE_SOURCE_DIR}/tools/restart-plasmashell.sh")
if(CUMULUS_RESTART_PLASMASHELL)
    # Last install step. The script does nothing for DESTDIR (packaging) installs, as root, or outside a Plasma session.
    install(CODE "execute_process(COMMAND \"${CUMULUS_RESTART_SCRIPT}\")")
    set(CUMULUS_RESTART_COMMAND "${CUMULUS_RESTART_SCRIPT}")
else()
    set(CUMULUS_RESTART_COMMAND ${CMAKE_COMMAND} -E true)
endif()

add_custom_target(install-user
    COMMAND ${CMAKE_COMMAND} --install "${CMAKE_BINARY_DIR}" --prefix "${CUMULUS_USER_PREFIX}"
    COMMENT "Installing the applet to ${CUMULUS_USER_PREFIX}/${CUMULUS_PACKAGE_INSTALL_DIR}"
    USES_TERMINAL
    VERBATIM
)

# Run as root through pkexec (graphical password prompt); the install manifest root writes into the
# build directory is removed again so later user installs can write theirs.
add_custom_target(install-system
    COMMAND pkexec sh -c "\"${CMAKE_COMMAND}\" --install \"${CMAKE_BINARY_DIR}\" --prefix \"${CUMULUS_SYSTEM_PREFIX}\"; status=$?; rm -f \"${CMAKE_BINARY_DIR}/install_manifest.txt\"; exit $status"
    COMMAND ${CUMULUS_RESTART_COMMAND}
    COMMENT "Installing the applet to ${CUMULUS_SYSTEM_PREFIX}/${CUMULUS_PACKAGE_INSTALL_DIR}"
    USES_TERMINAL
    VERBATIM
)

# The installed applet is exactly one directory, so uninstalling removes that directory.
add_custom_target(uninstall-user
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${CUMULUS_USER_PREFIX}/${CUMULUS_PACKAGE_INSTALL_DIR}"
    COMMAND ${CUMULUS_RESTART_COMMAND}
    COMMENT "Removing ${CUMULUS_USER_PREFIX}/${CUMULUS_PACKAGE_INSTALL_DIR}"
    USES_TERMINAL
    VERBATIM
)
add_custom_target(uninstall-system
    COMMAND pkexec "${CMAKE_COMMAND}" -E rm -rf "${CUMULUS_SYSTEM_PREFIX}/${CUMULUS_PACKAGE_INSTALL_DIR}"
    COMMAND ${CUMULUS_RESTART_COMMAND}
    COMMENT "Removing ${CUMULUS_SYSTEM_PREFIX}/${CUMULUS_PACKAGE_INSTALL_DIR}"
    USES_TERMINAL
    VERBATIM
)

string(TOLOWER "${CUMULUS_INSTALL_MODE}" _cumulus_mode)
add_custom_target(uninstall)
add_dependencies(uninstall uninstall-${_cumulus_mode})

# Installing builds first, like the plain install target does.
foreach(_target install-user install-system)
    add_dependencies(${_target} cumulus_package cumulusplugin)
endforeach()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

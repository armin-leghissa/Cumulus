# SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
# SPDX-License-Identifier: BSD-2-Clause

# Logic without Qt Quick or network, shared by the QML module and the unit tests.
add_library(cumulus_core STATIC
    forecastrows.cpp
    openmeteo.cpp
    units.cpp
    weathercode.cpp
)
set_target_properties(cumulus_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(cumulus_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cumulus_core PUBLIC Qt6::Core)

# C++ types for the QML files of the applet package. The module is built straight into the
# package layout in the build tree and imported from the QML files by relative path
# (import "../lib/io/github/leghissa/cumulus"), so it needs no QML import path (docs/decisions.md D9).
set(CUMULUS_MODULE_DIR "${CUMULUS_PACKAGE_BUILD_DIR}/${CUMULUS_MODULE_SUBDIR}")

qt_policy(SET QTP0001 NEW) # resources under :/qt/qml/
qt_add_qml_module(cumulusplugin
    URI io.github.leghissa.cumulus
    VERSION 1.0
    PLUGIN_TARGET cumulusplugin
    OUTPUT_DIRECTORY "${CUMULUS_MODULE_DIR}"
    SOURCES
        forecastmodels.cpp forecastmodels.h
        locationsearchmodel.cpp locationsearchmodel.h
        weatherservice.cpp weatherservice.h
)
# QML looks for lib<plugin>.so; KDE's CMake settings drop the prefix for modules
set_target_properties(cumulusplugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CUMULUS_MODULE_DIR}" PREFIX "lib" INSTALL_RPATH "")
target_compile_definitions(cumulusplugin PRIVATE CUMULUS_VERSION_STRING="${PROJECT_VERSION}")
target_link_libraries(cumulusplugin PRIVATE
    cumulus_core
    Qt6::Network
    Qt6::Qml
)

install(TARGETS cumulusplugin LIBRARY DESTINATION "${CUMULUS_PACKAGE_INSTALL_DIR}/${CUMULUS_MODULE_SUBDIR}")
install(FILES
    "${CUMULUS_MODULE_DIR}/qmldir"
    "${CUMULUS_MODULE_DIR}/cumulusplugin.qmltypes"
    DESTINATION "${CUMULUS_PACKAGE_INSTALL_DIR}/${CUMULUS_MODULE_SUBDIR}"
)

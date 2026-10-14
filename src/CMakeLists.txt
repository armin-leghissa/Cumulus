# SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
# SPDX-License-Identifier: BSD-2-Clause

add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_io.github.leghissa.cumulus\")

# Logic without Qt Quick or network, shared by the applet and the unit tests.
add_library(cumulus_core STATIC
    forecastrows.cpp
    openmeteo.cpp
    units.cpp
    weathercode.cpp
)
set_target_properties(cumulus_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(cumulus_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cumulus_core PUBLIC Qt6::Core)

set_source_files_properties(qml/Format.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

plasma_add_applet(io.github.leghissa.cumulus
    QML_SOURCES
        qml/main.qml
        qml/CompactRepresentation.qml
        qml/FullRepresentation.qml
        qml/Format.qml
    CPP_SOURCES
        forecastmodels.cpp
        weatherservice.cpp
    RESOURCES
        config/main.xml
    GENERATE_APPLET_CLASS
)

target_compile_definitions(io.github.leghissa.cumulus PRIVATE CUMULUS_VERSION_STRING="${PROJECT_VERSION}")

target_link_libraries(io.github.leghissa.cumulus PRIVATE
    cumulus_core
    Qt6::Gui
    Qt6::Network
    Qt6::Qml
    Plasma::Plasma
    KF6::I18n
)

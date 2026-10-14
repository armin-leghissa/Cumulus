# SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
# SPDX-License-Identifier: BSD-2-Clause

add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_io.github.leghissa.cumulus\")

plasma_add_applet(io.github.leghissa.cumulus
    QML_SOURCES
        qml/main.qml
    CPP_SOURCES
        cumulus.cpp
    RESOURCES
        qml/images/pairs.svg
    GENERATE_APPLET_CLASS
)

target_link_libraries(io.github.leghissa.cumulus PRIVATE
    Qt6::Gui
    Plasma::Plasma
    KF6::I18n
)

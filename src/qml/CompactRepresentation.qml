// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid

// Panel: today's weather icon, optionally with today's temperature overlaid (docs/ui-guidelines.md).
MouseArea {
    id: compact

    required property WeatherService weather

    readonly property int panelTemperature: Plasmoid.configuration.panelTemperature

    signal toggleRequested()

    hoverEnabled: true
    onClicked: toggleRequested()

    Kirigami.Icon {
        id: icon
        anchors.fill: parent
        source: compact.weather.todayIconName
        active: compact.containsMouse
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        visible: compact.panelTemperature > 0 && compact.weather.hasForecast
        // max in the colour of the max line, min in the colour of the min line, slash in the text colour
        textFormat: Text.StyledText
        text: {
            const max = "<font color=\"" + Format.temperatureMaxColor + "\">" + Format.temperature(compact.weather.todayTemperatureMax) + "</font>";
            if (compact.panelTemperature !== 2) {
                return max;
            }
            const min = "<font color=\"" + Format.temperatureMinColor + "\">" + Format.temperature(compact.weather.todayTemperatureMin) + "</font>";
            return min + "/" + max;
        }
        font.pixelSize: Math.max(Kirigami.Theme.smallFont.pixelSize * 0.8, parent.height * 0.32)
        font.bold: true
        fontSizeMode: Text.HorizontalFit
        minimumPixelSize: 6
        color: Kirigami.Theme.textColor
        // outline keeps the number readable on any icon and panel background
        style: Text.Outline
        styleColor: Kirigami.Theme.backgroundColor
    }
}

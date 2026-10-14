// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC3
import org.kde.plasma.extras as PlasmaExtras

// Popup. Interim day list until the charts are implemented (docs/ui-guidelines.md).
ColumnLayout {
    id: full

    required property WeatherService weather
    required property string locationName

    Layout.preferredWidth: Kirigami.Units.gridUnit * 24
    Layout.preferredHeight: Kirigami.Units.gridUnit * 22
    spacing: Kirigami.Units.smallSpacing

    PlasmaExtras.Heading {
        Layout.fillWidth: true
        level: 3
        text: full.locationName
        elide: Text.ElideRight
    }

    PlasmaExtras.PlaceholderMessage {
        Layout.fillWidth: true
        Layout.fillHeight: true
        visible: !full.weather.hasForecast
        iconName: full.weather.status === WeatherService.Error ? "network-disconnect" : "weather-none-available"
        text: full.weather.status === WeatherService.Loading ? i18nc("@info", "Loading forecast…")
            : full.weather.status === WeatherService.Error ? i18nc("@info", "Forecast not available")
            : i18nc("@info", "No forecast")
        explanation: full.weather.status === WeatherService.Error ? full.weather.errorString : ""
    }

    ListView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        visible: full.weather.hasForecast
        clip: true
        model: full.weather.dailyModel

        delegate: RowLayout {
            required property int weekday
            required property string iconName
            required property var temperatureMin
            required property var temperatureMax
            required property var precipitationProbability
            required property bool precipitationProbabilityFallback
            required property var precipitationAmount
            required property var windSpeed

            width: ListView.view.width
            spacing: Kirigami.Units.largeSpacing

            PC3.Label {
                Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                text: Qt.locale().standaloneDayName(weekday, Locale.ShortFormat)
            }
            Kirigami.Icon {
                Layout.preferredWidth: Kirigami.Units.iconSizes.medium
                Layout.preferredHeight: Kirigami.Units.iconSizes.medium
                source: iconName
            }
            PC3.Label {
                Layout.preferredWidth: Kirigami.Units.gridUnit * 4
                text: Format.temperature(temperatureMin) + " / " + Format.temperature(temperatureMax)
            }
            PC3.Label {
                Layout.preferredWidth: Kirigami.Units.gridUnit * 3
                text: Format.percent(precipitationProbability)
                opacity: precipitationProbabilityFallback ? 0.6 : 1
            }
            PC3.Label {
                Layout.preferredWidth: Kirigami.Units.gridUnit * 3
                text: Format.precipitation(precipitationAmount, full.weather.precipitationSymbol)
            }
            PC3.Label {
                Layout.fillWidth: true
                text: Format.windSpeed(windSpeed, full.weather.windSpeedSymbol)
            }
        }
    }

    PC3.Label {
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignRight
        font: Kirigami.Theme.smallFont
        opacity: 0.7
        textFormat: Text.StyledText
        text: i18nc("@info attribution, %1 is a link", "Weather data by %1", "<a href=\"https://open-meteo.com/\">Open-Meteo.com</a>")
        onLinkActivated: link => Qt.openUrlExternally(link)
    }
}

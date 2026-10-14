// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC3
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.plasmoid

// Popup: 14-day chart and hourly chart, synced by the selected day (docs/ui-guidelines.md).
ColumnLayout {
    id: full

    required property WeatherService weather
    required property string locationName
    required property bool expanded

    // selected calendar day, 0 = today
    property int selectedDay: 0

    // Fixed column width: a wider popup shows more columns. The setting only sets the initial width.
    readonly property real columnWidth: Kirigami.Units.gridUnit * 4.5
    readonly property int visibleColumns: Math.max(3, Plasmoid.configuration.visibleColumns)
    readonly property bool hourlyAbove: Plasmoid.configuration.hourlyChartPosition === 1

    function selectDay(day) {
        selectedDay = day;
        const row = weather.hourlyModel.firstHourOfDay(day);
        if (row >= 0) {
            hourlyChart.scrollToIndex(row, true);
        }
    }

    // today is selected whenever the popup opens
    onExpandedChanged: {
        if (expanded) {
            selectedDay = 0;
            dailyChart.scrollToIndex(0, false);
            hourlyChart.scrollToIndex(0, false);
        }
    }
    onSelectedDayChanged: dailyChart.ensureVisible(selectedDay)

    Layout.preferredWidth: columnWidth * visibleColumns
    Layout.preferredHeight: Kirigami.Units.gridUnit * 30
    Layout.minimumWidth: columnWidth * 3
    Layout.minimumHeight: Kirigami.Units.gridUnit * 22
    spacing: Kirigami.Units.smallSpacing

    RowLayout {
        Layout.fillWidth: true

        PlasmaExtras.Heading {
            Layout.fillWidth: true
            level: 3
            text: full.locationName
            elide: Text.ElideRight
        }
        PC3.BusyIndicator {
            Layout.preferredWidth: Kirigami.Units.iconSizes.small
            Layout.preferredHeight: Kirigami.Units.iconSizes.small
            visible: full.weather.status === WeatherService.Loading
            running: visible
        }
        PC3.Label {
            visible: full.weather.hasForecast
            font: Kirigami.Theme.smallFont
            opacity: 0.7
            text: full.weather.status === WeatherService.Error
                  ? i18nc("@info:status", "Update failed, data from %1", Qt.formatTime(full.weather.lastUpdated, Qt.locale().timeFormat(Locale.ShortFormat)))
                  : i18nc("@info:status", "Updated %1", Qt.formatTime(full.weather.lastUpdated, Qt.locale().timeFormat(Locale.ShortFormat)))
        }
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

    // rows: chart block, separator, chart block; the order of the blocks follows the setting
    GridLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        visible: full.weather.hasForecast
        columns: 1
        rowSpacing: Kirigami.Units.smallSpacing

        PlasmaExtras.Heading {
            Layout.row: full.hourlyAbove ? 3 : 0
            Layout.fillWidth: true
            level: 5
            text: i18nc("@title", "14-day forecast")
        }

        ForecastChart {
            id: dailyChart
            Layout.row: full.hourlyAbove ? 4 : 1
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: full.weather.dailyModel
            hourly: false
            columnWidth: full.columnWidth
            selectedIndex: full.selectedDay
            windSpeedSymbol: full.weather.windSpeedSymbol
            precipitationSymbol: full.weather.precipitationSymbol
            onColumnClicked: index => full.selectDay(index)
        }

        Kirigami.Separator {
            Layout.row: 2
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.smallSpacing
            Layout.bottomMargin: Kirigami.Units.smallSpacing
        }

        PlasmaExtras.Heading {
            Layout.row: full.hourlyAbove ? 0 : 3
            Layout.fillWidth: true
            level: 5
            text: i18nc("@title", "Hourly forecast")
        }

        ForecastChart {
            id: hourlyChart
            Layout.row: full.hourlyAbove ? 1 : 4
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: full.weather.hourlyModel
            hourly: true
            columnWidth: full.columnWidth
            windSpeedSymbol: full.weather.windSpeedSymbol
            precipitationSymbol: full.weather.precipitationSymbol

            // selection follows scrolling: the day of the first visible hour
            function syncSelectedDay() {
                if (scrollAnimating) {
                    return; // a selected day is being scrolled to; don't select the days passed on the way
                }
                const day = full.weather.hourlyModel.dayIndexAt(firstVisibleIndex);
                if (day >= 0) {
                    full.selectedDay = day;
                }
            }
            onFirstVisibleIndexChanged: syncSelectedDay()
            onScrollAnimatingChanged: syncSelectedDay()
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

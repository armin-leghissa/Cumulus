// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC3
import org.kde.quickcharts as Charts

// One chart component for the 14-day and the hourly chart (docs/ui-guidelines.md, "Charts"):
// fixed column width, horizontal scrolling, header per column (label, icon, wind),
// temperature lines and semi-transparent precipitation probability bars with the amount on top.
Flickable {
    id: chart

    required property var model // DailyForecastModel or HourlyForecastModel
    required property real columnWidth
    required property bool hourly // hourly: one temperature line, hour labels, night shading
    property int selectedIndex: -1 // highlighted column (daily chart)
    property string windSpeedSymbol
    property string precipitationSymbol

    readonly property int count: columns.count
    // first column that is at least half visible
    readonly property int firstVisibleIndex: Math.max(0, Math.round(contentX / Math.max(1, columnWidth)))
    readonly property bool scrollAnimating: scrollAnimation.running

    signal columnClicked(int index)

    readonly property color maxColor: Format.temperatureMaxColor
    readonly property color minColor: Format.temperatureMinColor
    readonly property color barColor: Qt.rgba(0.49, 0.78, 0.94, 0.4)

    // Daily header: "Mo, 12.10." on one line if that fits the column, otherwise the date below the weekday.
    readonly property bool dateBelow: dateMetrics.width > columnWidth - Kirigami.Units.smallSpacing * 2
    readonly property real labelHeight: label.implicitHeight * (!hourly && dateBelow ? 2 : 1)
    readonly property real headerHeight: labelHeight + iconSize + wind.implicitHeight + Kirigami.Units.smallSpacing * 3
    readonly property real iconSize: Math.min(Kirigami.Units.iconSizes.medium, columnWidth * 0.7)
    readonly property real plotTop: headerHeight + valueLabelHeight
    readonly property real plotHeight: Math.max(0, height - plotTop - (hourly ? 0 : valueLabelHeight))
    readonly property real valueLabelHeight: valueMetrics.height + Kirigami.Units.smallSpacing

    // fixed axis over all data, with some headroom so lines don't touch the edges
    readonly property real yFrom: model.temperatureMinimum - Math.max(1, (model.temperatureMaximum - model.temperatureMinimum) * 0.1)
    readonly property real yTo: model.temperatureMaximum + Math.max(1, (model.temperatureMaximum - model.temperatureMinimum) * 0.1)

    function yForTemperature(value) {
        return plotTop + plotHeight * (1 - (value - yFrom) / Math.max(0.001, yTo - yFrom));
    }

    function scrollToIndex(index, animated) {
        const target = Math.max(0, Math.min(index * columnWidth, contentWidth - width));
        scrollAnimation.stop();
        cancelFlick();
        if (animated) {
            scrollAnimation.to = target;
            scrollAnimation.start();
        } else {
            contentX = target;
        }
    }

    function ensureVisible(index) {
        const left = index * columnWidth;
        if (left < contentX) {
            scrollToIndex(index, true);
        } else if (left + columnWidth > contentX + width) {
            scrollToIndex(index - Math.floor(width / columnWidth) + 1, true);
        }
    }

    contentWidth: count * columnWidth
    contentHeight: height
    flickableDirection: Flickable.HorizontalFlick
    boundsBehavior: Flickable.StopAtBounds
    clip: true

    NumberAnimation {
        id: scrollAnimation
        target: chart
        property: "contentX"
        duration: Kirigami.Units.longDuration
        easing.type: Easing.InOutQuad
    }

    // mouse wheel scrolls by one column per step
    WheelHandler {
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        onWheel: event => {
            const delta = event.angleDelta.y !== 0 ? event.angleDelta.y : event.angleDelta.x;
            scrollAnimation.stop();
            chart.contentX = Math.max(0, Math.min(chart.contentX - delta / 120 * chart.columnWidth,
                                                  chart.contentWidth - chart.width));
        }
    }

    // widest daily label, decides between one and two lines
    TextMetrics {
        id: dateMetrics
        font: label.font
        text: Qt.locale().standaloneDayName(3, Locale.ShortFormat) + ", " + Format.dayMonth(30, 12)
    }

    TextMetrics {
        id: valueMetrics
        font: Kirigami.Theme.smallFont
        text: "-88°"
    }

    // header metrics (invisible), so all columns share one header height
    PC3.Label {
        id: label
        visible: false
        text: "Mo"
    }
    PC3.Label {
        id: wind
        visible: false
        font: Kirigami.Theme.smallFont
        text: "0"
    }

    // columns: background (night, selection), header, precipitation bar, value labels
    Repeater {
        id: columns
        model: chart.model

        delegate: Item {
            id: column

            required property int index
            required property var model

            readonly property bool fallback: model.precipitationProbabilityFallback
            readonly property var probability: model.precipitationProbability
            readonly property real barHeight: Format.hasValue(probability) ? chart.plotHeight * probability / 100 : 0
            readonly property bool dayStart: chart.hourly && model.hour === 0

            x: index * chart.columnWidth
            width: chart.columnWidth
            height: chart.height

            Rectangle {
                anchors.fill: parent
                visible: chart.hourly && column.model.isNight
                color: Qt.darker(Kirigami.Theme.backgroundColor, 1.04)
            }

            Rectangle {
                anchors.fill: parent
                visible: column.index === chart.selectedIndex
                radius: Kirigami.Units.cornerRadius
                color: Qt.alpha(Kirigami.Theme.highlightColor, 0.2)
                border.color: Kirigami.Theme.highlightColor
            }

            // day boundary in the hourly chart
            Rectangle {
                visible: column.dayStart && column.index > 0
                width: 1
                height: parent.height
                color: Kirigami.Theme.disabledTextColor
            }

            Column {
                id: header
                width: parent.width
                topPadding: Kirigami.Units.smallSpacing
                spacing: Kirigami.Units.smallSpacing

                PC3.Label {
                    readonly property string weekday: Qt.locale().standaloneDayName(column.model.weekday, Locale.ShortFormat)
                    readonly property string date: chart.hourly ? "" : Format.dayMonth(column.model.day, column.model.month)
                    width: parent.width
                    height: chart.labelHeight
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                    font.bold: !chart.hourly && column.model.isToday || column.dayStart
                    text: !chart.hourly ? (chart.dateBelow ? weekday + "\n" + date : weekday + ", " + date)
                        : column.dayStart ? weekday
                        : Format.hour(column.model.hour)
                }
                Kirigami.Icon {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: chart.iconSize
                    height: chart.iconSize
                    source: column.model.iconName
                }
                PC3.Label {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    font: Kirigami.Theme.smallFont
                    opacity: 0.8
                    text: Format.windSpeed(column.model.windSpeed, chart.windSpeedSymbol)
                }
            }

            // precipitation probability bar, from the bottom of the plot area
            Rectangle {
                id: bar
                anchors.horizontalCenter: parent.horizontalCenter
                y: chart.plotTop + chart.plotHeight - height
                width: parent.width * 0.6
                height: column.barHeight
                radius: Kirigami.Units.cornerRadius / 2
                color: chart.barColor
                opacity: column.fallback ? 0.5 : 1
            }

            // amount at the inner top edge of the bar, or just above a bar that is too short
            PC3.Label {
                readonly property bool inside: bar.height >= implicitHeight + Kirigami.Units.smallSpacing
                anchors.horizontalCenter: parent.horizontalCenter
                y: inside ? bar.y + Kirigami.Units.smallSpacing / 2 : bar.y - implicitHeight
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                font: Kirigami.Theme.smallFont
                opacity: column.fallback ? 0.5 : 0.9
                text: Format.precipitation(column.model.precipitationAmount, chart.precipitationSymbol)
                visible: text !== "" && column.barHeight > 0
            }

            // temperature values next to the line points
            PC3.Label {
                readonly property var value: chart.hourly ? column.model.temperature : column.model.temperatureMax
                visible: Format.hasValue(value)
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                y: chart.yForTemperature(value) - implicitHeight - Kirigami.Units.smallSpacing / 2
                font: Kirigami.Theme.smallFont
                color: chart.maxColor
                text: Format.temperature(value)
            }
            PC3.Label {
                readonly property var value: column.model.temperatureMin
                visible: !chart.hourly && Format.hasValue(value)
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                y: chart.yForTemperature(value ?? 0) + Kirigami.Units.smallSpacing / 2
                font: Kirigami.Theme.smallFont
                color: chart.minColor
                text: Format.temperature(value)
            }

            TapHandler {
                onTapped: chart.columnClicked(column.index)
            }
        }
    }

    // temperature lines through the column centres
    Charts.LineChart {
        x: chart.columnWidth / 2
        y: chart.plotTop
        width: Math.max(0, (chart.count - 1) * chart.columnWidth)
        height: chart.plotHeight
        visible: chart.count > 1
        interpolate: true
        lineWidth: 2
        fillOpacity: 0
        yRange {
            from: chart.yFrom
            to: chart.yTo
            automatic: false
        }
        colorSource: Charts.ArraySource {
            array: chart.hourly ? [chart.maxColor] : [chart.maxColor, chart.minColor]
        }
        valueSources: chart.hourly
            ? [temperatureSource]
            : [maxSource, minSource]

        Charts.ModelSource {
            id: temperatureSource
            model: chart.model
            roleName: "temperature"
        }
        Charts.ModelSource {
            id: maxSource
            model: chart.model
            roleName: "temperatureMax"
        }
        Charts.ModelSource {
            id: minSource
            model: chart.model
            roleName: "temperatureMin"
        }
    }
}

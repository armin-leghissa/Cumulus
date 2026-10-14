// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

KCM.SimpleKCM {
    id: page

    // bound to config/main.xml by the configuration dialog
    property string cfg_locationName
    property real cfg_latitude
    property real cfg_longitude
    property int cfg_panelTemperature
    property int cfg_refreshInterval
    property int cfg_dayIconMode
    property int cfg_visibleColumns
    property int cfg_hourlyChartPosition

    // defaults, set by the dialog as well (unused here, declared to avoid warnings)
    property string cfg_locationNameDefault
    property real cfg_latitudeDefault
    property real cfg_longitudeDefault
    property int cfg_panelTemperatureDefault
    property int cfg_refreshIntervalDefault
    property int cfg_dayIconModeDefault
    property int cfg_visibleColumnsDefault
    property int cfg_hourlyChartPositionDefault

    Kirigami.FormLayout {
        // Location

        QQC2.Label {
            Kirigami.FormData.label: i18nc("@label", "Location:")
            text: page.cfg_locationName !== ""
                  ? page.cfg_locationName
                  : i18nc("@info no location chosen yet", "None")
            font.bold: page.cfg_locationName !== ""
        }

        Kirigami.SearchField {
            id: searchField
            Kirigami.FormData.label: i18nc("@label", "Search:")
            Layout.preferredWidth: Kirigami.Units.gridUnit * 18
            placeholderText: i18nc("@info:placeholder", "City or place name…")
            onTextChanged: locationSearch.query = text
        }

        LocationSearchModel {
            id: locationSearch
        }

        QQC2.ScrollView {
            implicitWidth: searchField.Layout.preferredWidth
            implicitHeight: Kirigami.Units.gridUnit * 8
            Layout.maximumHeight: implicitHeight
            clip: true
            visible: locationSearch.status !== LocationSearchModel.Idle

            ListView {
                id: results
                clip: true
                model: locationSearch

                delegate: QQC2.ItemDelegate {
                    required property string name
                    required property string displayName
                    required property real latitude
                    required property real longitude

                    width: ListView.view.width
                    text: displayName
                    onClicked: {
                        page.cfg_locationName = name;
                        page.cfg_latitude = latitude;
                        page.cfg_longitude = longitude;
                        searchField.text = "";
                    }
                }

                Kirigami.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: parent.width - Kirigami.Units.largeSpacing * 2
                    visible: results.count === 0
                    icon.name: locationSearch.status === LocationSearchModel.Error ? "network-disconnect" : ""
                    text: locationSearch.status === LocationSearchModel.Searching ? i18nc("@info", "Searching…")
                        : locationSearch.status === LocationSearchModel.NoResults ? i18nc("@info", "No places found")
                        : locationSearch.status === LocationSearchModel.Error ? i18nc("@info", "Search failed")
                        : ""
                    explanation: locationSearch.status === LocationSearchModel.Error ? locationSearch.errorString : ""
                }
            }
        }

        Item {
            Kirigami.FormData.isSection: true
        }

        // Panel

        QQC2.ComboBox {
            Kirigami.FormData.label: i18nc("@label:listbox", "Temperature on icon:")
            model: [
                i18nc("@item:inlistbox temperature on panel icon", "None"),
                i18nc("@item:inlistbox temperature on panel icon", "Maximum"),
                i18nc("@item:inlistbox temperature on panel icon", "Minimum and maximum"),
            ]
            currentIndex: page.cfg_panelTemperature
            onActivated: index => page.cfg_panelTemperature = index
        }

        QQC2.ComboBox {
            id: dayIconMode
            Kirigami.FormData.label: i18nc("@label:listbox", "Day icon:")
            model: [
                i18nc("@item:inlistbox how a day is condensed into one icon", "Optimistic"),
                i18nc("@item:inlistbox how a day is condensed into one icon", "Average"),
                i18nc("@item:inlistbox how a day is condensed into one icon", "Pessimistic"),
            ]
            currentIndex: page.cfg_dayIconMode
            onActivated: index => page.cfg_dayIconMode = index
        }

        QQC2.Label {
            Layout.preferredWidth: searchField.Layout.preferredWidth
            wrapMode: Text.Wrap
            font: Kirigami.Theme.smallFont
            opacity: 0.7
            text: [
                i18nc("@info", "Shows rain only if it rains for at least half of the daylight hours, otherwise the better part of the day."),
                i18nc("@info", "Shows rain if it rains for at least a quarter of the daylight hours, otherwise the typical weather of the day."),
                i18nc("@info", "Shows rain as soon as it rains during daylight, otherwise the worse part of the day."),
            ][dayIconMode.currentIndex] ?? ""
        }

        Item {
            Kirigami.FormData.isSection: true
        }

        // Charts

        QQC2.SpinBox {
            Kirigami.FormData.label: i18nc("@label:spinbox", "Visible columns:")
            from: 3
            to: 14
            value: page.cfg_visibleColumns
            onValueModified: page.cfg_visibleColumns = value
        }

        QQC2.ComboBox {
            Kirigami.FormData.label: i18nc("@label:listbox", "Hourly chart:")
            model: [
                i18nc("@item:inlistbox position of the hourly chart", "Below the 14-day chart"),
                i18nc("@item:inlistbox position of the hourly chart", "Above the 14-day chart"),
            ]
            currentIndex: page.cfg_hourlyChartPosition
            onActivated: index => page.cfg_hourlyChartPosition = index
        }

        Item {
            Kirigami.FormData.isSection: true
        }

        // Updates

        QQC2.SpinBox {
            Kirigami.FormData.label: i18nc("@label:spinbox", "Update every:")
            from: 15
            to: 24 * 60
            stepSize: 15
            value: page.cfg_refreshInterval
            textFromValue: (value, locale) => i18ncp("@item:valuesuffix", "%1 minute", "%1 minutes", value)
            valueFromText: (text, locale) => parseInt(text)
            onValueModified: page.cfg_refreshInterval = value
        }
    }
}

// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

import QtQuick
import org.kde.plasma.plasmoid

PlasmoidItem {
    id: root

    readonly property bool hasLocation: Plasmoid.configuration.locationName !== ""

    WeatherService {
        id: weatherService
        hasLocation: root.hasLocation
        latitude: Plasmoid.configuration.latitude
        longitude: Plasmoid.configuration.longitude
        refreshInterval: Plasmoid.configuration.refreshInterval
        dayIconMode: Plasmoid.configuration.dayIconMode
    }

    Plasmoid.icon: weatherService.todayIconName
    Plasmoid.configurationRequired: !hasLocation

    toolTipMainText: hasLocation ? Plasmoid.configuration.locationName : Plasmoid.title
    toolTipSubText: {
        if (!weatherService.hasForecast) {
            return "";
        }
        return i18nc("@info:tooltip today's minimum and maximum temperature", "Today: %1 – %2",
                     Format.temperature(weatherService.todayTemperatureMin),
                     Format.temperature(weatherService.todayTemperatureMax));
    }

    compactRepresentation: CompactRepresentation {
        weather: weatherService
        onToggleRequested: root.expanded = !root.expanded
    }

    fullRepresentation: FullRepresentation {
        weather: weatherService
        expanded: root.expanded
        locationName: Plasmoid.configuration.locationName
    }
}

// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

pragma Singleton

import QtQuick

// Display formatting only; values and units come from WeatherService.
QtObject {
    readonly property string noValue: "–"

    function hasValue(value) {
        return value !== undefined && value !== null;
    }

    function number(value, decimals) {
        return hasValue(value) ? Number(value).toLocaleString(Qt.locale(), 'f', decimals) : noValue;
    }

    // e.g. "18°"
    function temperature(value) {
        return hasValue(value) ? number(Math.round(value), 0) + "°" : noValue;
    }

    // e.g. "60 %"
    function percent(value) {
        return hasValue(value) ? i18nc("@label percentage", "%1 %", number(Math.round(value), 0)) : noValue;
    }

    // e.g. "12 km/h"
    function windSpeed(value, symbol) {
        return hasValue(value) ? number(Math.round(value), 0) + " " + symbol : noValue;
    }

    // e.g. "2.4 mm"; empty for zero, so no label is shown
    function precipitation(value, symbol) {
        if (!hasValue(value) || value <= 0) {
            return "";
        }
        return number(value, symbol === "in" ? 2 : 1) + " " + symbol;
    }
}

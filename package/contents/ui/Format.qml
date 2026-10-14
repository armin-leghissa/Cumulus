// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

pragma Singleton

import QtQuick

// Display formatting only; values and units come from WeatherService.
QtObject {
    readonly property string noValue: "–"

    // chart line colours, also used for the temperatures on the panel icon
    readonly property color temperatureMaxColor: "#e74c3c"
    readonly property color temperatureMinColor: "#3498db"

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

    // Day and month in the locale's order and separator, without year and leading zeros:
    // "12.10." (de), "10/12" (en_US), "12/10" (en_GB).
    function dayMonth(day, month) {
        const format = Qt.locale().dateFormat(Locale.ShortFormat);
        const dayFirst = format.indexOf("d") < format.indexOf("M");
        const separator = (format.match(/[dM]+([^dMy]+)[dM]/) || [null, "."])[1];
        const text = dayFirst ? day + separator + month : month + separator + day;
        return separator === "." ? text + "." : text;
    }

    readonly property bool twelveHourClock: /[aA]/.test(Qt.locale().timeFormat(Locale.ShortFormat))

    // hour of day (0–23) as "13:00", or "1 PM" for 12-hour locales
    function hour(value) {
        if (!hasValue(value)) {
            return noValue;
        }
        if (twelveHourClock) {
            return (value % 12 === 0 ? 12 : value % 12) + " " + (value < 12 ? Qt.locale().amText : Qt.locale().pmText);
        }
        return (value < 10 ? "0" : "") + value + ":00";
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

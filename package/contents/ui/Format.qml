// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

pragma Singleton

import QtQuick

// Display formatting only; values and units come from WeatherService, the unit symbols are translated here.
QtObject {
    readonly property string noValue: "–"

    // A singleton is created outside the applet's QML context, where Plasma sets the translation
    // domain, so plain i18n() would not translate here.
    readonly property string domain: "plasma_applet_io.github.leghissa.cumulus"

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
        return hasValue(value) ? i18ndc(domain, "@label percentage", "%1 %", number(Math.round(value), 0)) : noValue;
    }

    // e.g. "12 km/h"; unit is WeatherService.windSpeedUnit
    function windSpeed(value, unit) {
        if (!hasValue(value)) {
            return noValue;
        }
        const text = number(Math.round(value), 0);
        return unit === "mph" ? i18ndc(domain, "@label wind speed in miles per hour", "%1 mph", text)
                              : i18ndc(domain, "@label wind speed in kilometres per hour", "%1 km/h", text);
    }

    // e.g. "2.4 mm"; empty for zero, so no label is shown; unit is WeatherService.precipitationUnit
    function precipitation(value, unit) {
        if (!hasValue(value) || value <= 0) {
            return "";
        }
        return unit === "inch" ? i18ndc(domain, "@label precipitation amount in inches", "%1 in", number(value, 2))
                               : i18ndc(domain, "@label precipitation amount in millimetres", "%1 mm", number(value, 1));
    }
}

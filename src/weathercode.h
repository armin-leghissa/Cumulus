// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "forecast.h"

#include <QDateTime>
#include <QString>

#include <optional>

// WMO weather code helpers: condensing a day into one code, and mapping codes to theme icons.
namespace WeatherCode
{
// Values match the dayIconMode entry in main.xml.
enum class DayIconMode {
    Optimistic = 0,
    Average = 1,
    Pessimistic = 2,
};

// Condenses the hourly codes of `day` into one code (docs/decisions.md D8):
// only daylight hours (for today: the remaining ones), precipitation first if enough hours are wet,
// otherwise a quantile of the dry hours' codes by severity. Falls back to the API's daily code.
std::optional<int> condenseDay(const DailyForecast &day, const QList<HourlyForecast> &hours, const QDateTime &now, DayIconMode mode);

// Freedesktop weather icon name for a WMO code, e.g. "weather-showers-scattered".
QString iconName(std::optional<int> code, bool night = false);
}

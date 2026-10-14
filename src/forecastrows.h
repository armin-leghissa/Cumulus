// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "forecast.h"
#include "weathercode.h"

#include <QList>
#include <QString>

#include <optional>
#include <utility>

// Display rows for the daily and hourly models, derived from a Forecast at a given time.
// Kept free of Qt Quick so it can be unit tested.

struct DayRow {
    DailyForecast data;
    int weekday = 0; // 0 = Sunday … 6 = Saturday, as expected by QML's Locale.standaloneDayName()
    QString iconName; // condensed day icon (docs/decisions.md D8)
};

struct HourRow {
    HourlyForecast data;
    int dayIndex = 0; // calendar day in the location's time zone, 0 = today
    int weekday = 0; // 0 = Sunday … 6 = Saturday
    bool isNight = false;
    QString iconName;
};

namespace ForecastRows
{
// One row per forecast day, today first. Days before today (in the location's time zone) are dropped.
QList<DayRow> days(const Forecast &forecast, const QDateTime &now, WeatherCode::DayIconMode mode);

// One row per hour, from the current hour to the end of the forecast. Past hours are dropped.
QList<HourRow> hours(const Forecast &forecast, const QDateTime &now);

// Lowest and highest temperature over all rows (min/max for days), for fixed chart axes.
std::optional<std::pair<double, double>> temperatureRange(const QList<DayRow> &rows);
std::optional<std::pair<double, double>> temperatureRange(const QList<HourRow> &rows);

// Row to scroll to when a day is selected: 00:00 of that day, or the first row for today. -1 if none.
int firstHourOfDay(const QList<HourRow> &rows, int dayIndex);
}

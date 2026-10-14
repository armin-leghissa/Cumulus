// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QDate>
#include <QDateTime>
#include <QList>
#include <QTimeZone>

#include <optional>

// Plain value types for one parsed forecast. Times are in the location's time zone.

struct DailyForecast {
    QDate date;
    std::optional<double> temperatureMin;
    std::optional<double> temperatureMax;
    std::optional<double> precipitationProbability; // percent
    bool precipitationProbabilityFallback = false; // value comes from the fallback model (GFS)
    std::optional<double> precipitationSum;
    std::optional<double> windSpeedMean;
    std::optional<int> weatherCode; // API daily code = most severe condition of the day
    QDateTime sunrise;
    QDateTime sunset;
};

struct HourlyForecast {
    QDateTime time; // start of the hour
    std::optional<double> temperature;
    std::optional<double> precipitationProbability;
    bool precipitationProbabilityFallback = false;
    std::optional<double> precipitation;
    std::optional<double> windSpeed;
    std::optional<int> weatherCode;
};

struct Forecast {
    QTimeZone timeZone;
    QList<DailyForecast> days;
    QList<HourlyForecast> hours;
};

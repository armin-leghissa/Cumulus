// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "forecast.h"

#include <QList>

// Builds synthetic forecasts: `days` days from `firstDay`, sunrise 07:00, sunset 18:00,
// hourly weather codes given per day (24 entries each, missing days filled with 0).
inline Forecast makeForecast(QDate firstDay, int days, const QList<QList<int>> &hourlyCodes = {})
{
    Forecast forecast;
    forecast.timeZone = QTimeZone("Europe/Vienna");
    for (int d = 0; d < days; ++d) {
        const QDate date = firstDay.addDays(d);
        DailyForecast day;
        day.date = date;
        day.temperatureMin = 5 + d;
        day.temperatureMax = 15 + d;
        day.weatherCode = 99; // "most severe" from the API, must not leak into condensed icons
        day.sunrise = QDateTime(date, QTime(7, 0), forecast.timeZone);
        day.sunset = QDateTime(date, QTime(18, 0), forecast.timeZone);
        forecast.days.append(day);
        for (int h = 0; h < 24; ++h) {
            HourlyForecast hour;
            hour.time = QDateTime(date, QTime(h, 0), forecast.timeZone);
            hour.temperature = 10;
            hour.weatherCode = d < hourlyCodes.size() ? hourlyCodes.at(d).value(h, 0) : 0;
            forecast.hours.append(hour);
        }
    }
    return forecast;
}

// 24 hourly codes: `night` outside 07:00–18:00, the given daylight codes (11 entries, 07–17) in between.
inline QList<int> dayCodes(const QList<int> &daylight, int night = 0)
{
    QList<int> codes(24, night);
    for (int i = 0; i < daylight.size() && 7 + i < 18; ++i) {
        codes[7 + i] = daylight.at(i);
    }
    return codes;
}

// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "forecastrows.h"

namespace
{
QDate today(const Forecast &forecast, const QDateTime &now)
{
    return now.toTimeZone(forecast.timeZone).date();
}

const DailyForecast *dayOf(const Forecast &forecast, QDate date)
{
    for (const DailyForecast &day : forecast.days) {
        if (day.date == date) {
            return &day;
        }
    }
    return nullptr;
}

bool isNight(const HourlyForecast &hour, const DailyForecast *day)
{
    if (!day || !day->sunrise.isValid() || !day->sunset.isValid()) {
        return false;
    }
    // An hour counts as night if its middle lies outside sunrise..sunset.
    const QDateTime middle = hour.time.addSecs(30 * 60);
    return middle < day->sunrise || middle >= day->sunset;
}
}

namespace ForecastRows
{
QList<DayRow> days(const Forecast &forecast, const QDateTime &now, WeatherCode::DayIconMode mode)
{
    const QDate first = today(forecast, now);
    QList<DayRow> rows;
    for (const DailyForecast &day : forecast.days) {
        if (day.date < first) {
            continue;
        }
        rows.append({
            .data = day,
            .weekday = day.date.dayOfWeek() % 7,
            .iconName = WeatherCode::iconName(WeatherCode::condenseDay(day, forecast.hours, now, mode)),
        });
    }
    return rows;
}

QList<HourRow> hours(const Forecast &forecast, const QDateTime &now)
{
    const QDate first = today(forecast, now);
    QList<HourRow> rows;
    for (const HourlyForecast &hour : forecast.hours) {
        if (hour.time.addSecs(3600) <= now) {
            continue;
        }
        const DailyForecast *day = dayOf(forecast, hour.time.date());
        const bool night = isNight(hour, day);
        rows.append({
            .data = hour,
            .dayIndex = int(first.daysTo(hour.time.date())),
            .isNight = night,
            .iconName = WeatherCode::iconName(hour.weatherCode, night),
        });
    }
    return rows;
}

int firstHourOfDay(const QList<HourRow> &rows, int dayIndex)
{
    for (qsizetype i = 0; i < rows.size(); ++i) {
        if (rows.at(i).dayIndex >= dayIndex) {
            return rows.at(i).dayIndex == dayIndex ? int(i) : -1;
        }
    }
    return -1;
}
}

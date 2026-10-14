// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "weathercode.h"

#include <QHash>

#include <algorithm>
#include <array>
#include <cmath>

using namespace Qt::StringLiterals;

namespace
{
// WMO codes used by Open-Meteo, from mild to severe.
constexpr std::array SeverityOrder = {0, 1, 2, 3, 45, 48, 51, 53, 55, 56, 57, 61, 80, 63, 81, 65, 82, 66, 67, 71, 85, 73, 75, 86, 77, 95, 96, 99};

int severity(int code)
{
    const auto it = std::find(SeverityOrder.begin(), SeverityOrder.end(), code);
    return it == SeverityOrder.end() ? 0 : int(std::distance(SeverityOrder.begin(), it));
}

bool isWet(int code)
{
    return code >= 51; // drizzle, rain, snow, showers, thunderstorm
}

// Nearest-rank quantile on severity, so the result is always a code that occurred.
int quantile(QList<int> codes, double q)
{
    std::sort(codes.begin(), codes.end(), [](int a, int b) {
        return severity(a) < severity(b);
    });
    const auto rank = qsizetype(std::ceil(q * codes.size()));
    return codes.at(std::clamp<qsizetype>(rank - 1, 0, codes.size() - 1));
}

int mostFrequent(const QList<int> &codes)
{
    QHash<int, int> counts;
    for (int code : codes) {
        ++counts[code];
    }
    int best = codes.first();
    for (auto it = counts.cbegin(); it != counts.cend(); ++it) {
        const int bestCount = counts.value(best);
        // ties go to the more severe code
        if (it.value() > bestCount || (it.value() == bestCount && severity(it.key()) > severity(best))) {
            best = it.key();
        }
    }
    return best;
}

int mostSevere(const QList<int> &codes)
{
    return *std::max_element(codes.begin(), codes.end(), [](int a, int b) {
        return severity(a) < severity(b);
    });
}

QList<int> codesInWindow(const DailyForecast &day, const QList<HourlyForecast> &hours, const QDateTime &now, bool daylightOnly)
{
    // An invalid `now` means "no current time": the whole day counts.
    const bool isToday = now.isValid() && !hours.isEmpty() && now.toTimeZone(hours.first().time.timeZone()).date() == day.date;
    QList<int> codes;
    for (const HourlyForecast &hour : hours) {
        if (!hour.weatherCode || hour.time.date() != day.date) {
            continue;
        }
        const QDateTime end = hour.time.addSecs(3600);
        if (isToday && end <= now) {
            continue; // past hour
        }
        if (daylightOnly && day.sunrise.isValid() && day.sunset.isValid() && (end <= day.sunrise || hour.time >= day.sunset)) {
            continue;
        }
        codes.append(*hour.weatherCode);
    }
    return codes;
}
}

namespace WeatherCode
{
std::optional<int> condenseDay(const DailyForecast &day, const QList<HourlyForecast> &hours, const QDateTime &now, DayIconMode mode)
{
    QList<int> codes = codesInWindow(day, hours, now, true);
    if (codes.isEmpty()) {
        codes = codesInWindow(day, hours, now, false); // today after sunset: the rest of the day
    }
    if (codes.isEmpty()) {
        codes = codesInWindow(day, hours, QDateTime(), false); // whole day
    }
    if (codes.isEmpty()) {
        return day.weatherCode;
    }

    QList<int> wet;
    QList<int> dry;
    for (int code : codes) {
        (isWet(code) ? wet : dry).append(code);
    }
    const double wetShare = double(wet.size()) / codes.size();

    switch (mode) {
    case DayIconMode::Optimistic:
        if (wetShare >= 0.5) {
            return mostFrequent(wet);
        }
        return quantile(dry, 0.25);
    case DayIconMode::Average:
        if (wetShare >= 0.25) {
            return mostFrequent(wet);
        }
        return quantile(dry, 0.5);
    case DayIconMode::Pessimistic:
        if (!wet.isEmpty()) {
            return mostSevere(wet);
        }
        return quantile(dry, 0.75);
    }
    return day.weatherCode;
}

QString iconName(std::optional<int> code, bool night)
{
    if (!code) {
        return u"weather-none-available"_s;
    }
    const auto dayNight = [night](const QString &base) {
        return night ? base + u"-night"_s : base;
    };
    switch (*code) {
    case 0:
        return dayNight(u"weather-clear"_s);
    case 1:
        return dayNight(u"weather-few-clouds"_s);
    case 2:
        return dayNight(u"weather-clouds"_s);
    case 3:
        return u"weather-overcast"_s;
    case 45:
    case 48:
        return u"weather-fog"_s;
    case 51:
    case 53:
    case 55:
    case 61:
    case 80:
        return dayNight(u"weather-showers-scattered"_s);
    case 63:
    case 65:
    case 81:
    case 82:
        return dayNight(u"weather-showers"_s);
    case 56:
    case 57:
    case 66:
    case 67:
        return u"weather-freezing-rain"_s;
    case 71:
    case 77:
    case 85:
        return dayNight(u"weather-snow-scattered"_s);
    case 73:
    case 75:
    case 86:
        return dayNight(u"weather-snow"_s);
    case 95:
        return dayNight(u"weather-storm"_s);
    case 96:
    case 99:
        return u"weather-hail"_s;
    }
    return u"weather-none-available"_s;
}
}

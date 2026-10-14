// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "forecast.h"
#include "units.h"

#include <QByteArray>
#include <QString>
#include <QUrl>

#include <optional>

// Request building and response parsing for the Open-Meteo API (see docs/decisions.md D1, D3, D8).
// No networking here, so everything is testable with recorded responses.
namespace OpenMeteo
{
inline constexpr int ForecastDays = 14;

QUrl forecastUrl(double latitude, double longitude, const Units &units);
QUrl geocodingUrl(const QString &name, const QString &language, int count = 10);

// Parses a forecast response requested with models=best_match,gfs_seamless.
// Unsuffixed (single model) responses are accepted as well.
// Returns std::nullopt and sets errorString on API errors or malformed data.
std::optional<Forecast> parseForecast(const QByteArray &json, QString *errorString = nullptr);

struct Location {
    QString name;
    QString region; // admin1, e.g. state
    QString country;
    double latitude = 0;
    double longitude = 0;

    // "Name, Region, Country", skipping empty parts and a region equal to the name
    QString displayName() const;
};

QList<Location> parseGeocoding(const QByteArray &json, QString *errorString = nullptr);
}

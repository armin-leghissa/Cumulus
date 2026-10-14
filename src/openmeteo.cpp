// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "openmeteo.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

using namespace Qt::StringLiterals;

namespace
{
const QString PrimaryModel = u"best_match"_s;
const QString FallbackModel = u"gfs_seamless"_s;

const QStringList DailyVariables = {
    u"temperature_2m_max"_s,
    u"temperature_2m_min"_s,
    u"precipitation_probability_max"_s,
    u"precipitation_sum"_s,
    u"wind_speed_10m_mean"_s,
    u"weather_code"_s,
    u"sunrise"_s,
    u"sunset"_s,
};

const QStringList HourlyVariables = {
    u"temperature_2m"_s,
    u"precipitation_probability"_s,
    u"precipitation"_s,
    u"wind_speed_10m"_s,
    u"weather_code"_s,
};

// One variable of a multi-model response: the primary model's series plus the fallback model's.
// A plain, unsuffixed series (single-model response) counts as primary.
class Series
{
public:
    Series(const QJsonObject &section, const QString &variable)
    {
        if (section.contains(variable)) {
            m_primary = section.value(variable).toArray();
        } else {
            m_primary = section.value(variable + u'_' + PrimaryModel).toArray();
        }
        m_fallback = section.value(variable + u'_' + FallbackModel).toArray();
    }

    // Primary value, or the fallback value where the primary one is null.
    std::optional<double> number(qsizetype i, bool *fromFallback = nullptr) const
    {
        if (fromFallback) {
            *fromFallback = false;
        }
        if (const auto v = m_primary.at(i); v.isDouble()) {
            return v.toDouble();
        }
        if (const auto v = m_fallback.at(i); v.isDouble()) {
            if (fromFallback) {
                *fromFallback = true;
            }
            return v.toDouble();
        }
        return std::nullopt;
    }

    std::optional<int> code(qsizetype i) const
    {
        const auto v = number(i);
        return v ? std::optional<int>(qRound(*v)) : std::nullopt;
    }

    QString string(qsizetype i) const
    {
        const QString primary = m_primary.at(i).toString();
        return primary.isEmpty() ? m_fallback.at(i).toString() : primary;
    }

private:
    QJsonArray m_primary;
    QJsonArray m_fallback;
};

QDateTime localDateTime(const QString &isoLocal, const QTimeZone &zone)
{
    // Open-Meteo returns local times without offset, e.g. "2026-10-14T13:00".
    QDateTime dt = QDateTime::fromString(isoLocal, Qt::ISODate);
    if (!dt.isValid()) {
        return {};
    }
    return QDateTime(dt.date(), dt.time(), zone);
}

void setError(QString *errorString, const QString &message)
{
    if (errorString) {
        *errorString = message;
    }
}

std::optional<QJsonObject> parseObject(const QByteArray &json, QString *errorString)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (!doc.isObject()) {
        setError(errorString, u"Invalid response: "_s + parseError.errorString());
        return std::nullopt;
    }
    const QJsonObject root = doc.object();
    if (root.value(u"error"_s).toBool()) {
        setError(errorString, root.value(u"reason"_s).toString(u"Unknown error"_s));
        return std::nullopt;
    }
    return root;
}
}

namespace OpenMeteo
{
QUrl forecastUrl(double latitude, double longitude, const Units &units)
{
    QUrl url(u"https://api.open-meteo.com/v1/forecast"_s);
    QUrlQuery query;
    query.addQueryItem(u"latitude"_s, QString::number(latitude, 'f', 4));
    query.addQueryItem(u"longitude"_s, QString::number(longitude, 'f', 4));
    query.addQueryItem(u"daily"_s, DailyVariables.join(u','));
    query.addQueryItem(u"hourly"_s, HourlyVariables.join(u','));
    query.addQueryItem(u"models"_s, PrimaryModel + u',' + FallbackModel);
    query.addQueryItem(u"timezone"_s, u"auto"_s);
    query.addQueryItem(u"forecast_days"_s, QString::number(ForecastDays));
    query.addQueryItem(u"temperature_unit"_s, units.temperature);
    query.addQueryItem(u"wind_speed_unit"_s, units.windSpeed);
    query.addQueryItem(u"precipitation_unit"_s, units.precipitation);
    url.setQuery(query);
    return url;
}

QUrl geocodingUrl(const QString &name, const QString &language, int count)
{
    QUrl url(u"https://geocoding-api.open-meteo.com/v1/search"_s);
    QUrlQuery query;
    query.addQueryItem(u"name"_s, name);
    query.addQueryItem(u"count"_s, QString::number(count));
    query.addQueryItem(u"language"_s, language);
    query.addQueryItem(u"format"_s, u"json"_s);
    url.setQuery(query);
    return url;
}

std::optional<Forecast> parseForecast(const QByteArray &json, QString *errorString)
{
    const auto root = parseObject(json, errorString);
    if (!root) {
        return std::nullopt;
    }

    Forecast forecast;
    forecast.timeZone = QTimeZone(root->value(u"timezone"_s).toString().toUtf8());
    if (!forecast.timeZone.isValid()) {
        forecast.timeZone = QTimeZone::fromSecondsAheadOfUtc(root->value(u"utc_offset_seconds"_s).toInt());
    }

    const QJsonObject daily = root->value(u"daily"_s).toObject();
    const QJsonArray dailyTime = daily.value(u"time"_s).toArray();
    const QJsonObject hourly = root->value(u"hourly"_s).toObject();
    const QJsonArray hourlyTime = hourly.value(u"time"_s).toArray();
    if (dailyTime.isEmpty() || hourlyTime.isEmpty()) {
        setError(errorString, u"Invalid response: no daily or hourly data"_s);
        return std::nullopt;
    }

    const Series tMax(daily, u"temperature_2m_max"_s);
    const Series tMin(daily, u"temperature_2m_min"_s);
    const Series dProbability(daily, u"precipitation_probability_max"_s);
    const Series dPrecipitation(daily, u"precipitation_sum"_s);
    const Series dWind(daily, u"wind_speed_10m_mean"_s);
    const Series dCode(daily, u"weather_code"_s);
    const Series sunrise(daily, u"sunrise"_s);
    const Series sunset(daily, u"sunset"_s);

    forecast.days.reserve(dailyTime.size());
    for (qsizetype i = 0; i < dailyTime.size(); ++i) {
        DailyForecast day;
        day.date = QDate::fromString(dailyTime.at(i).toString(), Qt::ISODate);
        day.temperatureMax = tMax.number(i);
        day.temperatureMin = tMin.number(i);
        day.precipitationProbability = dProbability.number(i, &day.precipitationProbabilityFallback);
        day.precipitationSum = dPrecipitation.number(i);
        day.windSpeedMean = dWind.number(i);
        day.weatherCode = dCode.code(i);
        day.sunrise = localDateTime(sunrise.string(i), forecast.timeZone);
        day.sunset = localDateTime(sunset.string(i), forecast.timeZone);
        forecast.days.append(day);
    }

    const Series temperature(hourly, u"temperature_2m"_s);
    const Series hProbability(hourly, u"precipitation_probability"_s);
    const Series hPrecipitation(hourly, u"precipitation"_s);
    const Series hWind(hourly, u"wind_speed_10m"_s);
    const Series hCode(hourly, u"weather_code"_s);

    forecast.hours.reserve(hourlyTime.size());
    for (qsizetype i = 0; i < hourlyTime.size(); ++i) {
        HourlyForecast hour;
        hour.time = localDateTime(hourlyTime.at(i).toString(), forecast.timeZone);
        hour.temperature = temperature.number(i);
        hour.precipitationProbability = hProbability.number(i, &hour.precipitationProbabilityFallback);
        hour.precipitation = hPrecipitation.number(i);
        hour.windSpeed = hWind.number(i);
        hour.weatherCode = hCode.code(i);
        forecast.hours.append(hour);
    }

    return forecast;
}

QString Location::displayName() const
{
    QStringList parts{name};
    if (!region.isEmpty() && region != name) {
        parts.append(region);
    }
    if (!country.isEmpty()) {
        parts.append(country);
    }
    return parts.join(u", "_s);
}

QList<Location> parseGeocoding(const QByteArray &json, QString *errorString)
{
    QList<Location> locations;
    const auto root = parseObject(json, errorString);
    if (!root) {
        return locations;
    }
    // "results" is missing entirely when nothing matches.
    const QJsonArray results = root->value(u"results"_s).toArray();
    for (const QJsonValue &value : results) {
        const QJsonObject o = value.toObject();
        locations.append({
            .name = o.value(u"name"_s).toString(),
            .region = o.value(u"admin1"_s).toString(),
            .country = o.value(u"country"_s).toString(),
            .latitude = o.value(u"latitude"_s).toDouble(),
            .longitude = o.value(u"longitude"_s).toDouble(),
        });
    }
    return locations;
}
}

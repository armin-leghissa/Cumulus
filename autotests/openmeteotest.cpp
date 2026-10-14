// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "openmeteo.h"

#include <QFile>
#include <QTest>
#include <QUrlQuery>

using namespace Qt::StringLiterals;

class OpenMeteoTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parsesRecordedResponse()
    {
        QFile file(QFINDTESTDATA("data/forecast-vienna.json"));
        QVERIFY(file.open(QIODevice::ReadOnly));

        QString error;
        const auto forecast = OpenMeteo::parseForecast(file.readAll(), &error);
        QVERIFY2(forecast, qPrintable(error));

        QCOMPARE(forecast->timeZone.id(), "Europe/Vienna");
        QCOMPARE(forecast->days.size(), 14);
        QCOMPARE(forecast->hours.size(), 14 * 24);

        const DailyForecast &first = forecast->days.first();
        QCOMPARE(first.date, QDate(2026, 10, 14));
        QCOMPARE(first.sunrise, QDateTime(QDate(2026, 10, 14), QTime(7, 12), forecast->timeZone));
        QVERIFY(first.temperatureMin && first.temperatureMax && first.windSpeedMean && first.precipitationSum && first.weatherCode);
        QVERIFY(*first.temperatureMin <= *first.temperatureMax);

        // best_match has no probability for days 12–14 in this recording; GFS fills in.
        for (int i = 0; i < 14; ++i) {
            const DailyForecast &day = forecast->days.at(i);
            QVERIFY2(day.precipitationProbability, qPrintable(day.date.toString()));
            QCOMPARE(day.precipitationProbabilityFallback, i >= 11);
        }

        QCOMPARE(forecast->hours.first().time, QDateTime(QDate(2026, 10, 14), QTime(0, 0), forecast->timeZone));
        QVERIFY(forecast->hours.last().precipitationProbability);
        QVERIFY(forecast->hours.last().precipitationProbabilityFallback);
    }

    void fallsBackPerValue()
    {
        const QByteArray json = R"({
            "timezone": "UTC", "utc_offset_seconds": 0,
            "daily": {
                "time": ["2026-10-14", "2026-10-15", "2026-10-16"],
                "precipitation_probability_max_best_match": [40, null, null],
                "precipitation_probability_max_gfs_seamless": [10, 30, null],
                "temperature_2m_max_best_match": [null, 12.5, 13],
                "temperature_2m_max_gfs_seamless": [11, 99, 99]
            },
            "hourly": { "time": ["2026-10-14T00:00"] }
        })";
        const auto forecast = OpenMeteo::parseForecast(json);
        QVERIFY(forecast);
        const auto &days = forecast->days;

        QCOMPARE(days[0].precipitationProbability, 40.0);
        QVERIFY(!days[0].precipitationProbabilityFallback);
        QCOMPARE(days[1].precipitationProbability, 30.0);
        QVERIFY(days[1].precipitationProbabilityFallback);
        QVERIFY(!days[2].precipitationProbability);

        QCOMPARE(days[0].temperatureMax, 11.0);
        QCOMPARE(days[1].temperatureMax, 12.5);
        QVERIFY(!days[0].temperatureMin); // not in the response at all
    }

    void acceptsSingleModelResponse()
    {
        const QByteArray json = R"({
            "timezone": "Europe/Vienna",
            "daily": { "time": ["2026-10-14"], "temperature_2m_max": [18.2], "weather_code": [3] },
            "hourly": { "time": ["2026-10-14T00:00"], "weather_code": [2] }
        })";
        const auto forecast = OpenMeteo::parseForecast(json);
        QVERIFY(forecast);
        QCOMPARE(forecast->days[0].temperatureMax, 18.2);
        QCOMPARE(forecast->days[0].weatherCode, 3);
        QCOMPARE(forecast->hours[0].weatherCode, 2);
    }

    void reportsErrors_data()
    {
        QTest::addColumn<QByteArray>("json");
        QTest::addColumn<QString>("error");
        QTest::newRow("api error") << QByteArray(R"({"error": true, "reason": "Latitude must be in range of -90 to 90°."})")
                                   << u"Latitude must be in range of -90 to 90°."_s;
        QTest::newRow("not json") << QByteArray("The service is overloaded") << QString();
        QTest::newRow("no data") << QByteArray(R"({"timezone": "UTC"})") << QString();
    }

    void reportsErrors()
    {
        QFETCH(QByteArray, json);
        QFETCH(QString, error);
        QString errorString;
        QVERIFY(!OpenMeteo::parseForecast(json, &errorString));
        QVERIFY(!errorString.isEmpty());
        if (!error.isEmpty()) {
            QCOMPARE(errorString, error);
        }
    }

    void buildsForecastUrl()
    {
        const Units units = Units::forMeasurementSystem(QLocale::ImperialUSSystem);
        const QUrl url = OpenMeteo::forecastUrl(48.20849, 16.37208, units);
        const QUrlQuery query(url);
        QCOMPARE(url.host(), u"api.open-meteo.com"_s);
        QCOMPARE(query.queryItemValue(u"latitude"_s), u"48.2085"_s);
        QCOMPARE(query.queryItemValue(u"models"_s), u"best_match,gfs_seamless"_s);
        QCOMPARE(query.queryItemValue(u"forecast_days"_s), u"14"_s);
        QCOMPARE(query.queryItemValue(u"timezone"_s), u"auto"_s);
        QCOMPARE(query.queryItemValue(u"temperature_unit"_s), u"fahrenheit"_s);
        QCOMPARE(query.queryItemValue(u"wind_speed_unit"_s), u"mph"_s);
        QCOMPARE(query.queryItemValue(u"precipitation_unit"_s), u"inch"_s);
        QVERIFY(query.queryItemValue(u"daily"_s).split(u',').contains(u"sunrise"_s));
    }

    void unitsFollowMeasurementSystem()
    {
        QCOMPARE(Units::forMeasurementSystem(QLocale::MetricSystem).windSpeedSymbol, u"km/h"_s);
        QCOMPARE(Units::forMeasurementSystem(QLocale::ImperialUKSystem).temperature, u"celsius"_s);
        QCOMPARE(Units::forMeasurementSystem(QLocale::ImperialUKSystem).windSpeed, u"mph"_s);
        QCOMPARE(Units::forMeasurementSystem(QLocale::ImperialUSSystem).precipitationSymbol, u"in"_s);
    }

    void parsesGeocoding()
    {
        const QByteArray json = R"({"results": [
            {"name": "Wien", "latitude": 48.20849, "longitude": 16.37208, "country": "Österreich", "admin1": "Wien"},
            {"name": "Vienna", "latitude": 38.90122, "longitude": -77.26526, "country": "Vereinigte Staaten", "admin1": "Virginia"}
        ]})";
        const auto locations = OpenMeteo::parseGeocoding(json);
        QCOMPARE(locations.size(), 2);
        QCOMPARE(locations[1].region, u"Virginia"_s);
        QCOMPARE(locations[1].longitude, -77.26526);

        QVERIFY(OpenMeteo::parseGeocoding(R"({"generationtime_ms": 0.1})").isEmpty()); // no match
    }
};

QTEST_GUILESS_MAIN(OpenMeteoTest)
#include "openmeteotest.moc"

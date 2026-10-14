// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "testforecast.h"
#include "weathercode.h"

#include <QTest>

using namespace Qt::StringLiterals;
using WeatherCode::DayIconMode;

Q_DECLARE_METATYPE(DayIconMode)

namespace
{
const QDate Day(2026, 10, 20);

std::optional<int> condense(const QList<int> &codes, DayIconMode mode, const QDateTime &now = {})
{
    const Forecast forecast = makeForecast(Day, 1, {codes});
    return WeatherCode::condenseDay(forecast.days.first(), forecast.hours, now, mode);
}
}

class WeatherCodeTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void condensesDaylightHours_data()
    {
        QTest::addColumn<QList<int>>("codes");
        QTest::addColumn<DayIconMode>("mode");
        QTest::addColumn<int>("expected");

        // 11 daylight hours (07–17); night hours are rain and must be ignored.
        const auto day = [](const QList<int> &daylight) {
            return dayCodes(daylight, 61);
        };

        QTest::newRow("clear day, rainy night") << day({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}) << DayIconMode::Average << 0;
        QTest::newRow("one foggy hour is no fog day") << day({45, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0}) << DayIconMode::Average << 0;
        QTest::newRow("median of clouds") << day({0, 0, 1, 1, 2, 2, 2, 3, 3, 3, 3}) << DayIconMode::Average << 2;
        QTest::newRow("optimistic 25th percentile") << day({0, 0, 1, 1, 2, 2, 2, 3, 3, 3, 3}) << DayIconMode::Optimistic << 1;
        QTest::newRow("pessimistic 75th percentile") << day({0, 0, 1, 1, 2, 2, 2, 3, 3, 3, 3}) << DayIconMode::Pessimistic << 3;
        QTest::newRow("fog most of the day") << day({45, 45, 45, 45, 45, 45, 3, 3, 2, 2, 2}) << DayIconMode::Average << 45;

        // 3 of 11 wet hours = 27 %: rain for average, not for optimistic.
        const QList<int> rainyAfternoon = day({1, 1, 1, 1, 2, 2, 2, 2, 61, 63, 61});
        QTest::newRow("rainy afternoon, average") << rainyAfternoon << DayIconMode::Average << 61;
        QTest::newRow("rainy afternoon, optimistic") << rainyAfternoon << DayIconMode::Optimistic << 1;
        QTest::newRow("rainy afternoon, pessimistic") << rainyAfternoon << DayIconMode::Pessimistic << 63;

        // 2 of 11 wet hours: below the average threshold.
        QTest::newRow("short shower, average") << day({1, 1, 1, 1, 1, 1, 1, 1, 1, 80, 80}) << DayIconMode::Average << 1;
        QTest::newRow("short shower, pessimistic") << day({1, 1, 1, 1, 1, 1, 1, 1, 1, 80, 95}) << DayIconMode::Pessimistic << 95;
    }

    void condensesDaylightHours()
    {
        QFETCH(QList<int>, codes);
        QFETCH(DayIconMode, mode);
        QFETCH(int, expected);
        QCOMPARE(condense(codes, mode), expected);
    }

    void todayUsesRemainingHours()
    {
        // Rain in the morning, clear from noon on.
        const QList<int> codes = dayCodes({61, 61, 61, 61, 61, 0, 0, 0, 0, 0, 0});
        const QTimeZone zone("Europe/Vienna");
        QCOMPARE(condense(codes, DayIconMode::Average, QDateTime(Day, QTime(8, 30), zone)), 61);
        QCOMPARE(condense(codes, DayIconMode::Average, QDateTime(Day, QTime(12, 30), zone)), 0);
        // other days are not affected by the current time
        QCOMPARE(condense(codes, DayIconMode::Average, QDateTime(Day.addDays(-1), QTime(12, 30), zone)), 61);
    }

    void todayAfterSunsetUsesRestOfDay()
    {
        QList<int> codes = dayCodes({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 3);
        codes[21] = 45;
        codes[22] = 45;
        codes[23] = 45;
        const QDateTime evening(Day, QTime(20, 15), QTimeZone("Europe/Vienna"));
        QCOMPARE(condense(codes, DayIconMode::Average, evening), 45); // 20–23: 3, 45, 45, 45
    }

    void fallsBackToApiCode()
    {
        Forecast forecast = makeForecast(Day, 1);
        for (HourlyForecast &hour : forecast.hours) {
            hour.weatherCode.reset();
        }
        QCOMPARE(WeatherCode::condenseDay(forecast.days.first(), forecast.hours, {}, DayIconMode::Average), 99);
    }

    void mapsIcons()
    {
        QCOMPARE(WeatherCode::iconName(0), u"weather-clear"_s);
        QCOMPARE(WeatherCode::iconName(0, true), u"weather-clear-night"_s);
        QCOMPARE(WeatherCode::iconName(3, true), u"weather-overcast"_s);
        QCOMPARE(WeatherCode::iconName(61), u"weather-showers-scattered"_s);
        QCOMPARE(WeatherCode::iconName(65), u"weather-showers"_s);
        QCOMPARE(WeatherCode::iconName(75), u"weather-snow"_s);
        QCOMPARE(WeatherCode::iconName(95, true), u"weather-storm-night"_s);
        QCOMPARE(WeatherCode::iconName(std::nullopt), u"weather-none-available"_s);
        QCOMPARE(WeatherCode::iconName(42), u"weather-none-available"_s);
    }
};

QTEST_GUILESS_MAIN(WeatherCodeTest)
#include "weathercodetest.moc"

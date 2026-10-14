// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "forecastrows.h"
#include "testforecast.h"

#include <QTest>

using namespace Qt::StringLiterals;
using WeatherCode::DayIconMode;

namespace
{
const QDate FirstDay(2026, 10, 19); // a Monday
const QTimeZone Zone("Europe/Vienna");
}

class ForecastRowsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void dayRowsStartToday()
    {
        const Forecast forecast = makeForecast(FirstDay, 4, {dayCodes({}), dayCodes({61, 61, 61, 61, 61, 61, 61, 61, 61, 61, 61})});

        // second day of the forecast is "today" (data from yesterday's cache)
        const auto rows = ForecastRows::days(forecast, QDateTime(FirstDay.addDays(1), QTime(6, 0), Zone), DayIconMode::Average);
        QCOMPARE(rows.size(), 3);
        QCOMPARE(rows[0].data.date, FirstDay.addDays(1));
        QCOMPARE(rows[0].weekday, 2); // Tuesday, 0 = Sunday
        QCOMPARE(rows[0].iconName, u"weather-showers-scattered"_s);
        QCOMPARE(rows[1].iconName, u"weather-clear"_s);
    }

    void weekdayIsSundayBased()
    {
        const Forecast forecast = makeForecast(QDate(2026, 10, 25), 1); // Sunday
        QCOMPARE(ForecastRows::days(forecast, QDateTime(QDate(2026, 10, 25), QTime(9, 0), Zone), DayIconMode::Average)[0].weekday, 0);
    }

    void hourRowsStartAtCurrentHour()
    {
        const Forecast forecast = makeForecast(FirstDay, 3);
        const auto rows = ForecastRows::hours(forecast, QDateTime(FirstDay, QTime(13, 40), Zone));

        QCOMPARE(rows.size(), 3 * 24 - 13);
        QCOMPARE(rows[0].data.time.time(), QTime(13, 0));
        QCOMPARE(rows[0].dayIndex, 0);
        QCOMPARE(rows[11].data.time, QDateTime(FirstDay.addDays(1), QTime(0, 0), Zone));
        QCOMPARE(rows[11].dayIndex, 1);
        QCOMPARE(rows[0].weekday, 1); // Monday
        QCOMPARE(rows[11].weekday, 2);
        QCOMPARE(rows.last().dayIndex, 2);
    }

    void marksNightHours()
    {
        const Forecast forecast = makeForecast(FirstDay, 1);
        const auto rows = ForecastRows::hours(forecast, QDateTime(FirstDay, QTime(0, 0), Zone));
        // sunrise 07:00, sunset 18:00
        QVERIFY(rows[6].isNight);
        QVERIFY(!rows[7].isNight);
        QVERIFY(!rows[17].isNight);
        QVERIFY(rows[18].isNight);
        QCOMPARE(rows[22].iconName, u"weather-clear-night"_s);
        QCOMPARE(rows[12].iconName, u"weather-clear"_s);
    }

    void computesTemperatureRange()
    {
        Forecast forecast = makeForecast(FirstDay, 3); // min 5 + d, max 15 + d
        forecast.days[1].temperatureMax.reset();
        const auto now = QDateTime(FirstDay, QTime(10, 0), Zone);
        QCOMPARE(ForecastRows::temperatureRange(ForecastRows::days(forecast, now, DayIconMode::Average)), std::pair(5.0, 17.0));

        forecast.hours[12].temperature = -3.5;
        forecast.hours[30].temperature = 21;
        QCOMPARE(ForecastRows::temperatureRange(ForecastRows::hours(forecast, now)), std::pair(-3.5, 21.0));

        QVERIFY(!ForecastRows::temperatureRange(QList<DayRow>()));
    }

    void findsFirstHourOfDay()
    {
        const Forecast forecast = makeForecast(FirstDay, 3);
        const auto rows = ForecastRows::hours(forecast, QDateTime(FirstDay, QTime(20, 5), Zone));
        QCOMPARE(ForecastRows::firstHourOfDay(rows, 0), 0); // today: current hour
        QCOMPARE(ForecastRows::firstHourOfDay(rows, 1), 4); // 20, 21, 22, 23, then 00:00
        QCOMPARE(rows[4].data.time.time(), QTime(0, 0));
        QCOMPARE(ForecastRows::firstHourOfDay(rows, 3), -1);
    }
};

QTEST_GUILESS_MAIN(ForecastRowsTest)
#include "forecastrowstest.moc"

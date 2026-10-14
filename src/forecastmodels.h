// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "forecastrows.h"

#include <QAbstractListModel>
#include <QQmlEngine>

// One row per day, today first. Owned by WeatherService.
class DailyForecastModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by WeatherService")

    // lowest/highest temperature over all rows, for a fixed chart axis; 0 when empty
    Q_PROPERTY(double temperatureMinimum READ temperatureMinimum NOTIFY rowsChanged)
    Q_PROPERTY(double temperatureMaximum READ temperatureMaximum NOTIFY rowsChanged)

public:
    enum Role {
        DateRole = Qt::UserRole + 1,
        DayRole, // day of month, 1–31 (plain numbers avoid time zone shifts of QDate → JS Date)
        MonthRole, // 1–12
        WeekdayRole, // 0 = Sunday, for Locale.standaloneDayName()
        IsTodayRole,
        IconNameRole,
        TemperatureMinRole,
        TemperatureMaxRole,
        PrecipitationProbabilityRole, // undefined if unknown
        PrecipitationProbabilityFallbackRole,
        PrecipitationAmountRole,
        WindSpeedRole,
    };
    Q_ENUM(Role)

    using QAbstractListModel::QAbstractListModel;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRows(const QList<DayRow> &rows);
    const QList<DayRow> &rows() const;

    double temperatureMinimum() const;
    double temperatureMaximum() const;

Q_SIGNALS:
    void rowsChanged();

private:
    QList<DayRow> m_rows;
    std::pair<double, double> m_range;
};

// One row per hour, from the current hour to the end of the forecast. Owned by WeatherService.
class HourlyForecastModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by WeatherService")

    // lowest/highest temperature over all rows, for a fixed chart axis; 0 when empty
    Q_PROPERTY(double temperatureMinimum READ temperatureMinimum NOTIFY rowsChanged)
    Q_PROPERTY(double temperatureMaximum READ temperatureMaximum NOTIFY rowsChanged)

public:
    enum Role {
        TimeRole = Qt::UserRole + 1,
        HourRole,
        DayIndexRole,
        WeekdayRole, // 0 = Sunday
        IsNightRole,
        IconNameRole,
        TemperatureRole,
        PrecipitationProbabilityRole,
        PrecipitationProbabilityFallbackRole,
        PrecipitationAmountRole,
        WindSpeedRole,
    };
    Q_ENUM(Role)

    using QAbstractListModel::QAbstractListModel;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRows(const QList<HourRow> &rows);

    double temperatureMinimum() const;
    double temperatureMaximum() const;

    // Row of 00:00 of the given day, or the first row for today; -1 if the day is not in the model.
    Q_INVOKABLE int firstHourOfDay(int dayIndex) const;
    Q_INVOKABLE int dayIndexAt(int row) const;

Q_SIGNALS:
    void rowsChanged();

private:
    QList<HourRow> m_rows;
    std::pair<double, double> m_range;
};

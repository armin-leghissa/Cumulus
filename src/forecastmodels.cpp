// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "forecastmodels.h"

namespace
{
QVariant optional(const std::optional<double> &value)
{
    return value ? QVariant(*value) : QVariant();
}
}

int DailyForecastModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

QVariant DailyForecastModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return {};
    }
    const DayRow &row = m_rows.at(index.row());
    switch (role) {
    case DateRole:
        return row.data.date;
    case DayRole:
        return row.data.date.day();
    case MonthRole:
        return row.data.date.month();
    case WeekdayRole:
        return row.weekday;
    case IsTodayRole:
        return index.row() == 0;
    case IconNameRole:
        return row.iconName;
    case TemperatureMinRole:
        return optional(row.data.temperatureMin);
    case TemperatureMaxRole:
        return optional(row.data.temperatureMax);
    case PrecipitationProbabilityRole:
        return optional(row.data.precipitationProbability);
    case PrecipitationProbabilityFallbackRole:
        return row.data.precipitationProbabilityFallback;
    case PrecipitationAmountRole:
        return optional(row.data.precipitationSum);
    case WindSpeedRole:
        return optional(row.data.windSpeedMean);
    }
    return {};
}

QHash<int, QByteArray> DailyForecastModel::roleNames() const
{
    return {
        {DateRole, "date"},
        {DayRole, "day"},
        {MonthRole, "month"},
        {WeekdayRole, "weekday"},
        {IsTodayRole, "isToday"},
        {IconNameRole, "iconName"},
        {TemperatureMinRole, "temperatureMin"},
        {TemperatureMaxRole, "temperatureMax"},
        {PrecipitationProbabilityRole, "precipitationProbability"},
        {PrecipitationProbabilityFallbackRole, "precipitationProbabilityFallback"},
        {PrecipitationAmountRole, "precipitationAmount"},
        {WindSpeedRole, "windSpeed"},
    };
}

void DailyForecastModel::setRows(const QList<DayRow> &rows)
{
    beginResetModel();
    m_rows = rows;
    m_range = ForecastRows::temperatureRange(m_rows).value_or(std::pair(0.0, 0.0));
    endResetModel();
    Q_EMIT rowsChanged();
}

double DailyForecastModel::temperatureMinimum() const
{
    return m_range.first;
}

double DailyForecastModel::temperatureMaximum() const
{
    return m_range.second;
}

const QList<DayRow> &DailyForecastModel::rows() const
{
    return m_rows;
}

int HourlyForecastModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

QVariant HourlyForecastModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return {};
    }
    const HourRow &row = m_rows.at(index.row());
    switch (role) {
    case TimeRole:
        return row.data.time;
    case HourRole:
        return row.data.time.time().hour();
    case DayIndexRole:
        return row.dayIndex;
    case WeekdayRole:
        return row.weekday;
    case IsNightRole:
        return row.isNight;
    case IconNameRole:
        return row.iconName;
    case TemperatureRole:
        return optional(row.data.temperature);
    case PrecipitationProbabilityRole:
        return optional(row.data.precipitationProbability);
    case PrecipitationProbabilityFallbackRole:
        return row.data.precipitationProbabilityFallback;
    case PrecipitationAmountRole:
        return optional(row.data.precipitation);
    case WindSpeedRole:
        return optional(row.data.windSpeed);
    }
    return {};
}

QHash<int, QByteArray> HourlyForecastModel::roleNames() const
{
    return {
        {TimeRole, "time"},
        {HourRole, "hour"},
        {DayIndexRole, "dayIndex"},
        {WeekdayRole, "weekday"},
        {IsNightRole, "isNight"},
        {IconNameRole, "iconName"},
        {TemperatureRole, "temperature"},
        {PrecipitationProbabilityRole, "precipitationProbability"},
        {PrecipitationProbabilityFallbackRole, "precipitationProbabilityFallback"},
        {PrecipitationAmountRole, "precipitationAmount"},
        {WindSpeedRole, "windSpeed"},
    };
}

void HourlyForecastModel::setRows(const QList<HourRow> &rows)
{
    beginResetModel();
    m_rows = rows;
    m_range = ForecastRows::temperatureRange(m_rows).value_or(std::pair(0.0, 0.0));
    endResetModel();
    Q_EMIT rowsChanged();
}

double HourlyForecastModel::temperatureMinimum() const
{
    return m_range.first;
}

double HourlyForecastModel::temperatureMaximum() const
{
    return m_range.second;
}

int HourlyForecastModel::firstHourOfDay(int dayIndex) const
{
    return ForecastRows::firstHourOfDay(m_rows, dayIndex);
}

int HourlyForecastModel::dayIndexAt(int row) const
{
    return row >= 0 && row < m_rows.size() ? m_rows.at(row).dayIndex : -1;
}

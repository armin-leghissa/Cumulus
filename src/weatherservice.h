// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "forecast.h"
#include "forecastmodels.h"
#include "units.h"

#include <QDateTime>
#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QTimer>

#include <optional>

class QNetworkAccessManager;
class QNetworkReply;

// Fetches, caches and refreshes the forecast for one location and exposes it to QML.
// QML sets the inputs from the applet configuration and binds to the outputs.
class WeatherService : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    // inputs
    Q_PROPERTY(double latitude READ latitude WRITE setLatitude NOTIFY locationChanged)
    Q_PROPERTY(double longitude READ longitude WRITE setLongitude NOTIFY locationChanged)
    Q_PROPERTY(bool hasLocation READ hasLocation WRITE setHasLocation NOTIFY locationChanged)
    Q_PROPERTY(int refreshInterval READ refreshInterval WRITE setRefreshInterval NOTIFY refreshIntervalChanged) // minutes
    Q_PROPERTY(int dayIconMode READ dayIconMode WRITE setDayIconMode NOTIFY dayIconModeChanged)

    // outputs
    Q_PROPERTY(DailyForecastModel *dailyModel READ dailyModel CONSTANT)
    Q_PROPERTY(HourlyForecastModel *hourlyModel READ hourlyModel CONSTANT)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY statusChanged)
    Q_PROPERTY(QDateTime lastUpdated READ lastUpdated NOTIFY forecastChanged)
    Q_PROPERTY(bool hasForecast READ hasForecast NOTIFY forecastChanged)
    Q_PROPERTY(QString todayIconName READ todayIconName NOTIFY forecastChanged)
    Q_PROPERTY(QVariant todayTemperatureMin READ todayTemperatureMin NOTIFY forecastChanged)
    Q_PROPERTY(QVariant todayTemperatureMax READ todayTemperatureMax NOTIFY forecastChanged)
    Q_PROPERTY(QString temperatureSymbol READ temperatureSymbol CONSTANT)
    Q_PROPERTY(QString windSpeedSymbol READ windSpeedSymbol CONSTANT)
    Q_PROPERTY(QString precipitationSymbol READ precipitationSymbol CONSTANT)

public:
    enum Status {
        Idle,
        Loading,
        Ready,
        Error,
    };
    Q_ENUM(Status)

    explicit WeatherService(QObject *parent = nullptr);
    ~WeatherService() override;

    double latitude() const;
    void setLatitude(double latitude);
    double longitude() const;
    void setLongitude(double longitude);
    bool hasLocation() const;
    void setHasLocation(bool hasLocation);
    int refreshInterval() const;
    void setRefreshInterval(int minutes);
    int dayIconMode() const;
    void setDayIconMode(int mode);

    DailyForecastModel *dailyModel() const;
    HourlyForecastModel *hourlyModel() const;
    Status status() const;
    QString errorString() const;
    QDateTime lastUpdated() const;
    bool hasForecast() const;
    QString todayIconName() const;
    QVariant todayTemperatureMin() const;
    QVariant todayTemperatureMax() const;
    QString temperatureSymbol() const;
    QString windSpeedSymbol() const;
    QString precipitationSymbol() const;

    // Fetches the forecast now, regardless of its age.
    Q_INVOKABLE void refresh();

Q_SIGNALS:
    void locationChanged();
    void refreshIntervalChanged();
    void dayIconModeChanged();
    void statusChanged();
    void forecastChanged();

private:
    void abortRequest();
    void scheduleReload();
    void reload();
    void refreshIfStale();
    void onReplyFinished(QNetworkReply *reply);
    void handleFailure(const QString &message, bool transient);
    bool applyResponse(const QByteArray &json, const QDateTime &fetched, QString *errorString);
    void rebuildRows();
    void scheduleHourTick();
    void setStatus(Status status, const QString &errorString = {});
    QString cacheFilePath() const;

    double m_latitude = 0;
    double m_longitude = 0;
    bool m_hasLocation = false;
    int m_refreshInterval = 60;
    int m_dayIconMode = 1;
    Units m_units;

    QNetworkAccessManager *m_network;
    QPointer<QNetworkReply> m_reply;
    QTimer m_reloadTimer; // coalesces input changes
    QTimer m_refreshTimer;
    QTimer m_retryTimer; // after failures, see retry.h
    int m_failedAttempts = 0;
    QTimer m_hourTimer; // drops past hours and re-evaluates "today" at every full hour

    std::optional<Forecast> m_forecast;
    QDateTime m_lastUpdated;
    Status m_status = Idle;
    QString m_errorString;

    DailyForecastModel *m_dailyModel;
    HourlyForecastModel *m_hourlyModel;
};

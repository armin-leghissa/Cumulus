// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "weatherservice.h"
#include "cumulus_debug.h"
#include "openmeteo.h"
#include "retry.h"
#include "useragent.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkInformation>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QStandardPaths>

#include <algorithm>
#include <chrono>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

namespace
{
constexpr int MinimumRefreshInterval = 15; // minutes
constexpr auto RequestTimeout = 15s; // answers normally take well under a second
}

WeatherService::WeatherService(QObject *parent)
    : QObject(parent)
    , m_units(Units::fromSystem())
    , m_network(new QNetworkAccessManager(this))
    , m_dailyModel(new DailyForecastModel(this))
    , m_hourlyModel(new HourlyForecastModel(this))
{
    m_reloadTimer.setSingleShot(true);
    m_reloadTimer.setInterval(0);
    connect(&m_reloadTimer, &QTimer::timeout, this, &WeatherService::reload);

    m_refreshTimer.setSingleShot(true);
    connect(&m_refreshTimer, &QTimer::timeout, this, &WeatherService::refreshIfStale);

    m_retryTimer.setSingleShot(true);
    connect(&m_retryTimer, &QTimer::timeout, this, &WeatherService::refresh);

    m_hourTimer.setSingleShot(true);
    m_hourTimer.setTimerType(Qt::VeryCoarseTimer);
    connect(&m_hourTimer, &QTimer::timeout, this, [this] {
        rebuildRows();
        refreshIfStale(); // also catches up after suspend
    });

    connect(m_network, &QNetworkAccessManager::finished, this, &WeatherService::onReplyFinished);

    if (QNetworkInformation::loadDefaultBackend()) {
        connect(QNetworkInformation::instance(), &QNetworkInformation::reachabilityChanged, this, [this](QNetworkInformation::Reachability reachability) {
            if (reachability == QNetworkInformation::Reachability::Online) {
                refreshIfStale();
            }
        });
    }
}

WeatherService::~WeatherService() = default;

double WeatherService::latitude() const
{
    return m_latitude;
}

void WeatherService::setLatitude(double latitude)
{
    if (qFuzzyCompare(m_latitude, latitude)) {
        return;
    }
    m_latitude = latitude;
    Q_EMIT locationChanged();
    scheduleReload();
}

double WeatherService::longitude() const
{
    return m_longitude;
}

void WeatherService::setLongitude(double longitude)
{
    if (qFuzzyCompare(m_longitude, longitude)) {
        return;
    }
    m_longitude = longitude;
    Q_EMIT locationChanged();
    scheduleReload();
}

bool WeatherService::hasLocation() const
{
    return m_hasLocation;
}

void WeatherService::setHasLocation(bool hasLocation)
{
    if (m_hasLocation == hasLocation) {
        return;
    }
    m_hasLocation = hasLocation;
    Q_EMIT locationChanged();
    scheduleReload();
}

int WeatherService::refreshInterval() const
{
    return m_refreshInterval;
}

void WeatherService::setRefreshInterval(int minutes)
{
    minutes = std::max(minutes, MinimumRefreshInterval);
    if (m_refreshInterval == minutes) {
        return;
    }
    m_refreshInterval = minutes;
    Q_EMIT refreshIntervalChanged();
    refreshIfStale();
}

int WeatherService::dayIconMode() const
{
    return m_dayIconMode;
}

void WeatherService::setDayIconMode(int mode)
{
    mode = std::clamp(mode, int(WeatherCode::DayIconMode::Optimistic), int(WeatherCode::DayIconMode::Pessimistic));
    if (m_dayIconMode == mode) {
        return;
    }
    m_dayIconMode = mode;
    Q_EMIT dayIconModeChanged();
    rebuildRows();
}

DailyForecastModel *WeatherService::dailyModel() const
{
    return m_dailyModel;
}

HourlyForecastModel *WeatherService::hourlyModel() const
{
    return m_hourlyModel;
}

WeatherService::Status WeatherService::status() const
{
    return m_status;
}

QString WeatherService::errorString() const
{
    return m_errorString;
}

QDateTime WeatherService::lastUpdated() const
{
    return m_lastUpdated;
}

bool WeatherService::hasForecast() const
{
    return !m_dailyModel->rows().isEmpty();
}

QString WeatherService::todayIconName() const
{
    return hasForecast() ? m_dailyModel->rows().constFirst().iconName : u"weather-none-available"_s;
}

QVariant WeatherService::todayTemperatureMin() const
{
    return hasForecast() ? m_dailyModel->data(m_dailyModel->index(0), DailyForecastModel::TemperatureMinRole) : QVariant();
}

QVariant WeatherService::todayTemperatureMax() const
{
    return hasForecast() ? m_dailyModel->data(m_dailyModel->index(0), DailyForecastModel::TemperatureMaxRole) : QVariant();
}

QString WeatherService::temperatureSymbol() const
{
    return m_units.temperatureSymbol;
}

QString WeatherService::windSpeedSymbol() const
{
    return m_units.windSpeedSymbol;
}

QString WeatherService::precipitationSymbol() const
{
    return m_units.precipitationSymbol;
}

void WeatherService::refresh()
{
    if (!m_hasLocation) {
        return;
    }
    abortRequest();
    m_retryTimer.stop();
    QNetworkRequest request(OpenMeteo::forecastUrl(m_latitude, m_longitude, m_units));
    request.setHeader(QNetworkRequest::UserAgentHeader, Cumulus::userAgent());
    request.setTransferTimeout(RequestTimeout);
    m_reply = m_network->get(request);
    setStatus(Loading);
}

void WeatherService::abortRequest()
{
    // abort() emits finished() synchronously; clear m_reply first so it is not treated as a failure.
    if (QNetworkReply *reply = m_reply) {
        m_reply = nullptr;
        reply->abort();
    }
}

void WeatherService::scheduleReload()
{
    // latitude, longitude and hasLocation usually change together; reload once.
    m_reloadTimer.start();
}

void WeatherService::reload()
{
    abortRequest();
    m_refreshTimer.stop();
    m_retryTimer.stop();
    m_failedAttempts = 0;
    m_forecast.reset();
    m_lastUpdated = {};
    rebuildRows();

    if (!m_hasLocation) {
        setStatus(Idle);
        return;
    }

    // Show the cached forecast immediately, so the applet is not empty after login or offline.
    QFile cache(cacheFilePath());
    if (cache.open(QIODevice::ReadOnly)) {
        applyResponse(cache.readAll(), QFileInfo(cache).lastModified(), nullptr);
    }
    refreshIfStale();
}

void WeatherService::refreshIfStale()
{
    if (!m_hasLocation || m_reply) {
        return;
    }
    const QDateTime now = QDateTime::currentDateTime();
    const QDateTime due = m_lastUpdated.isValid() ? m_lastUpdated.addSecs(m_refreshInterval * 60) : now;
    if (due <= now) {
        refresh();
    } else {
        m_refreshTimer.start(std::chrono::milliseconds(now.msecsTo(due)));
    }
}

void WeatherService::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply != m_reply) {
        return; // aborted or superseded
    }
    m_reply = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        // Keep showing the last forecast; try again soon.
        handleFailure(reply->errorString(), Retry::isTransient(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()));
        return;
    }

    const QByteArray body = reply->readAll();
    QString error;
    if (!applyResponse(body, QDateTime::currentDateTime(), &error)) {
        handleFailure(error, false);
        return;
    }
    if (m_failedAttempts > 0) {
        qCInfo(CUMULUS) << "Forecast request succeeded after" << m_failedAttempts << "failed attempts";
    }
    m_failedAttempts = 0;

    QDir().mkpath(QFileInfo(cacheFilePath()).absolutePath());
    QSaveFile cache(cacheFilePath());
    if (cache.open(QIODevice::WriteOnly)) {
        cache.write(body);
        cache.commit();
    }

    setStatus(Ready);
    refreshIfStale();
}

void WeatherService::handleFailure(const QString &message, bool transient)
{
    ++m_failedAttempts;
    const auto delay = Retry::delay(m_failedAttempts, transient, std::chrono::minutes(m_refreshInterval));
    qCWarning(CUMULUS).nospace() << "Forecast request failed (attempt " << m_failedAttempts << "): " << message
                                 << " - retrying in " << std::chrono::duration_cast<std::chrono::seconds>(delay).count() << " s";
    // During the quick retries the failure is not shown yet; it is usually gone with the next attempt.
    if (Retry::isFast(m_failedAttempts, transient)) {
        setStatus(Loading);
    } else {
        setStatus(Error, message);
    }
    m_retryTimer.start(delay);
}

bool WeatherService::applyResponse(const QByteArray &json, const QDateTime &fetched, QString *errorString)
{
    auto forecast = OpenMeteo::parseForecast(json, errorString);
    if (!forecast) {
        return false;
    }
    m_forecast = std::move(forecast);
    m_lastUpdated = fetched;
    rebuildRows();
    return true;
}

void WeatherService::rebuildRows()
{
    if (m_forecast) {
        const QDateTime now = QDateTime::currentDateTime();
        m_dailyModel->setRows(ForecastRows::days(*m_forecast, now, WeatherCode::DayIconMode(m_dayIconMode)));
        m_hourlyModel->setRows(ForecastRows::hours(*m_forecast, now));
    } else {
        m_dailyModel->setRows({});
        m_hourlyModel->setRows({});
    }
    Q_EMIT forecastChanged();
    scheduleHourTick();
}

void WeatherService::scheduleHourTick()
{
    const QDateTime now = QDateTime::currentDateTime();
    const QTime t = now.time();
    const QDateTime nextHour = QDateTime(now.date(), QTime(t.hour(), 0)).addSecs(3600 + 5);
    m_hourTimer.start(std::chrono::milliseconds(now.msecsTo(nextHour)));
}

void WeatherService::setStatus(Status status, const QString &errorString)
{
    if (m_status == status && m_errorString == errorString) {
        return;
    }
    m_status = status;
    m_errorString = errorString;
    Q_EMIT statusChanged();
}

QString WeatherService::cacheFilePath() const
{
    const QString key = u"%1,%2,%3,%4,%5"_s.arg(m_latitude, 0, 'f', 4)
                            .arg(m_longitude, 0, 'f', 4)
                            .arg(m_units.temperature, m_units.windSpeed, m_units.precipitation);
    const QString hash = QString::fromLatin1(QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha1).toHex());
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + u"/cumulus/forecast-"_s + hash + u".json"_s;
}

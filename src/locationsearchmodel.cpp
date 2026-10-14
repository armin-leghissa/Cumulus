// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "locationsearchmodel.h"
#include "useragent.h"

#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <chrono>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

namespace
{
constexpr int MinimumQueryLength = 2;
}

LocationSearchModel::LocationSearchModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_network(new QNetworkAccessManager(this))
{
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(400ms);
    connect(&m_debounce, &QTimer::timeout, this, &LocationSearchModel::search);
    connect(m_network, &QNetworkAccessManager::finished, this, &LocationSearchModel::onReplyFinished);
}

int LocationSearchModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_results.size());
}

QVariant LocationSearchModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return {};
    }
    const OpenMeteo::Location &location = m_results.at(index.row());
    switch (role) {
    case NameRole:
        return location.name;
    case DisplayNameRole:
        return location.displayName();
    case LatitudeRole:
        return location.latitude;
    case LongitudeRole:
        return location.longitude;
    }
    return {};
}

QHash<int, QByteArray> LocationSearchModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {DisplayNameRole, "displayName"},
        {LatitudeRole, "latitude"},
        {LongitudeRole, "longitude"},
    };
}

QString LocationSearchModel::query() const
{
    return m_query;
}

void LocationSearchModel::setQuery(const QString &query)
{
    if (m_query == query) {
        return;
    }
    m_query = query;
    Q_EMIT queryChanged();
    m_debounce.start();
}

LocationSearchModel::Status LocationSearchModel::status() const
{
    return m_status;
}

QString LocationSearchModel::errorString() const
{
    return m_errorString;
}

void LocationSearchModel::search()
{
    if (QNetworkReply *reply = m_reply) {
        m_reply = nullptr; // abort() emits finished() synchronously
        reply->abort();
    }
    const QString query = m_query.trimmed();
    if (query.size() < MinimumQueryLength) {
        setResults({}, Idle);
        return;
    }
    const QString language = QLocale().name().section(u'_', 0, 0);
    QNetworkRequest request(OpenMeteo::geocodingUrl(query, language));
    request.setHeader(QNetworkRequest::UserAgentHeader, Cumulus::userAgent());
    request.setTransferTimeout(15s);
    m_reply = m_network->get(request);
    m_status = Searching;
    Q_EMIT statusChanged();
}

void LocationSearchModel::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply != m_reply) {
        return;
    }
    m_reply = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        setResults({}, Error, reply->errorString());
        return;
    }
    QString error;
    const QList<OpenMeteo::Location> results = OpenMeteo::parseGeocoding(reply->readAll(), &error);
    if (!error.isEmpty()) {
        setResults({}, Error, error);
        return;
    }
    setResults(results, results.isEmpty() ? NoResults : Ready);
}

void LocationSearchModel::setResults(const QList<OpenMeteo::Location> &results, Status status, const QString &errorString)
{
    beginResetModel();
    m_results = results;
    endResetModel();
    m_status = status;
    m_errorString = errorString;
    Q_EMIT statusChanged();
}

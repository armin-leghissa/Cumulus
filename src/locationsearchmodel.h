// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "openmeteo.h"

#include <QAbstractListModel>
#include <QPointer>
#include <QQmlEngine>
#include <QTimer>

class QNetworkAccessManager;
class QNetworkReply;

// Place search for the settings page (Open-Meteo geocoding). Set `query`; results follow after a short debounce.
class LocationSearchModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY statusChanged)

public:
    enum Status {
        Idle, // query too short
        Searching,
        Ready,
        NoResults,
        Error,
    };
    Q_ENUM(Status)

    enum Role {
        NameRole = Qt::UserRole + 1,
        DisplayNameRole,
        LatitudeRole,
        LongitudeRole,
    };
    Q_ENUM(Role)

    explicit LocationSearchModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString query() const;
    void setQuery(const QString &query);
    Status status() const;
    QString errorString() const;

Q_SIGNALS:
    void queryChanged();
    void statusChanged();

private:
    void search();
    void onReplyFinished(QNetworkReply *reply);
    void setResults(const QList<OpenMeteo::Location> &results, Status status, const QString &errorString = {});

    QString m_query;
    Status m_status = Idle;
    QString m_errorString;
    QList<OpenMeteo::Location> m_results;

    QNetworkAccessManager *m_network;
    QPointer<QNetworkReply> m_reply;
    QTimer m_debounce;
};

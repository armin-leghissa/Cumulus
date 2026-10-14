// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QObject>
#include <QString>
#include <QQmlEngine>

class Cumulus : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString message READ message CONSTANT)

public:
    static Cumulus *create(QQmlEngine *, QJSEngine *);

    QString message() const;

private:
    Cumulus();
};

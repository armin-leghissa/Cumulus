// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "cumulus.h"

#include <KLocalizedString>

QString Cumulus::message() const
{
    return i18nc("@info", "Hello World");
}

Cumulus *Cumulus::create(QQmlEngine *, QJSEngine *)
{
    static Cumulus instance;
    QQmlEngine::setObjectOwnership(&instance, QQmlEngine::CppOwnership);
    return &instance;
}

Cumulus::Cumulus()
    : QObject(nullptr)
{}

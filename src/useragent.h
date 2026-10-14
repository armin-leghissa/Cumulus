// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QByteArray>

namespace Cumulus
{
// Descriptive User-Agent for all requests to Open-Meteo.
inline QByteArray userAgent()
{
    return QByteArrayLiteral("Cumulus/" CUMULUS_VERSION_STRING " (https://github.com/armin-leghissa/Cumulus)");
}
}

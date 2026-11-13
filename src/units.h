// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QLocale>
#include <QString>

// Units follow the system measurement setting (LC_MEASUREMENT), see docs/ui-guidelines.md.
struct Units {
    // API parameter values, also passed to QML, which shows the translated unit symbols
    QString temperature; // celsius | fahrenheit
    QString windSpeed; // kmh | mph
    QString precipitation; // mm | inch

    static Units forMeasurementSystem(QLocale::MeasurementSystem system);
    static Units fromSystem();

    bool operator==(const Units &) const = default;
};

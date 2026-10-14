// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "units.h"

using namespace Qt::StringLiterals;

Units Units::forMeasurementSystem(QLocale::MeasurementSystem system)
{
    switch (system) {
    case QLocale::ImperialUSSystem:
        return {u"fahrenheit"_s, u"mph"_s, u"inch"_s, u"°F"_s, u"mph"_s, u"in"_s};
    case QLocale::ImperialUKSystem:
        return {u"celsius"_s, u"mph"_s, u"mm"_s, u"°C"_s, u"mph"_s, u"mm"_s};
    case QLocale::MetricSystem:
        break;
    }
    return {u"celsius"_s, u"kmh"_s, u"mm"_s, u"°C"_s, u"km/h"_s, u"mm"_s};
}

Units Units::fromSystem()
{
    return forMeasurementSystem(QLocale::system().measurementSystem());
}

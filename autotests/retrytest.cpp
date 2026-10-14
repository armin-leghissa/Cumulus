// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "retry.h"

#include <QTest>

using namespace std::chrono_literals;

class RetryTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void classifiesErrors()
    {
        QVERIFY(Retry::isTransient(0)); // no HTTP response: connect failure, timeout
        QVERIFY(Retry::isTransient(502));
        QVERIFY(Retry::isTransient(503));
        QVERIFY(!Retry::isTransient(429));
        QVERIFY(!Retry::isTransient(400));
    }

    void retriesTransientFailuresQuicklyFirst()
    {
        QCOMPARE(Retry::delay(1, true), 2s);
        QCOMPARE(Retry::delay(2, true), 10s);
        QCOMPARE(Retry::delay(3, true), 30s);
        QCOMPARE(Retry::delay(4, true), 5min);
        QCOMPARE(Retry::delay(40, true), 5min);
        QVERIFY(Retry::isFast(3, true));
        QVERIFY(!Retry::isFast(4, true));
    }

    void retriesOtherFailuresSlowly()
    {
        QCOMPARE(Retry::delay(1, false), 5min);
        QVERIFY(!Retry::isFast(1, false));
    }

    void capsSteadyDelay()
    {
        QCOMPARE(Retry::delay(5, true, 3min), 3min);
        QCOMPARE(Retry::delay(1, true, 1s), 2s); // fast delays are not capped
    }
};

QTEST_GUILESS_MAIN(RetryTest)
#include "retrytest.moc"

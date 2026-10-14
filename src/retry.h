// SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <chrono>

// When to try a failed request again.
// Open-Meteo sometimes does not answer a connection attempt at all while the next one succeeds
// (measured 2026-10-14: about one in five), so transient failures are retried quickly a few times
// before falling back to a slow, steady retry.
namespace Retry
{
using namespace std::chrono_literals;

inline constexpr int FastAttempts = 3;
inline constexpr std::chrono::milliseconds FastDelays[FastAttempts] = {2s, 10s, 30s};
inline constexpr std::chrono::milliseconds SteadyDelay = 5min;

// Connection problems, timeouts (no HTTP status) and server errors (5xx) are transient.
// Other HTTP errors, e.g. 429 "too many requests" or 400, are not retried quickly.
constexpr bool isTransient(int httpStatus)
{
    return httpStatus == 0 || httpStatus >= 500;
}

// True while the next attempt after `failedAttempts` consecutive failures (1 = first failure) is a fast one.
constexpr bool isFast(int failedAttempts, bool transient)
{
    return transient && failedAttempts >= 1 && failedAttempts <= FastAttempts;
}

// Delay before the next attempt; the steady delay is capped at `maximum` (e.g. the refresh interval).
constexpr std::chrono::milliseconds delay(int failedAttempts, bool transient, std::chrono::milliseconds maximum = SteadyDelay)
{
    if (isFast(failedAttempts, transient)) {
        return FastDelays[failedAttempts - 1];
    }
    return SteadyDelay < maximum ? SteadyDelay : maximum;
}
}

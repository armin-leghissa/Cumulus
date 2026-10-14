# Design decisions

Research and decisions for Cumulus. Status as of 2026-10-14; checked against
Plasma 6.7.4, Qt 6.11, KF 6.29 (Manjaro).

Goal: show a 14-day daily forecast per location with minimum/maximum
temperature and probability of rain.

## D1 – Weather data provider: Open-Meteo

**Decision:** Use the [Open-Meteo Forecast API](https://open-meteo.com/en/docs)
and the [Open-Meteo Geocoding API](https://open-meteo.com/en/docs/geocoding-api).

Why:
- Up to 16 forecast days (default 7), so 14 days fit.
- The required values exist directly as daily aggregates:
  `temperature_2m_min`, `temperature_2m_max`, `precipitation_probability_max`,
  plus `weather_code` (WMO code) for an icon.
- No API key, no account. Free for non-commercial use (private projects,
  open-source apps without ads/subscriptions). Limits of 600/min, 5,000/h,
  10,000/day and 300,000/month are far above what one applet needs.
- `timezone=auto` returns days aligned to the location's local time.
- The same provider offers geocoding (place name → coordinates, timezone,
  country, admin regions), so the location picker needs no second service.

Conditions to honour:
- Data is licensed under **CC BY 4.0**: the applet must show an attribution
  ("Weather data by Open-Meteo.com" with a link), e.g. in the full
  representation or the config dialog.
- Stay non-commercial (no ads/subscriptions), otherwise a paid plan is
  required.
- Send a descriptive `User-Agent` (applet name/version and repository URL).

Alternatives considered:

| Provider | Days | Key | Why not |
|---|---|---|---|
| MET Norway Locationforecast | ~9 | no (strict User-Agent) | too short, no daily min/max/probability aggregates |
| Visual Crossing | 15 | yes | free tier needs an account/key per user, 1,000 records/day |
| WeatherAPI.com | 3 free, 14 paid | yes | 14 days only on paid plans |
| OpenWeatherMap | 5 (3-hourly) free | yes | 16-day daily forecast not in the free tier |
| Plasma weather ions (BBC, DWD, EnvCan, NOAA, wetter.com) | provider-dependent, mostly ≤ 10 | – | see D2 |

## D2 – Not building on the Plasma weather library (for now)

kdeplasma-addons 6.7 ships `PlasmaWeather` (`libplasmaweatherion`,
`libplasmaweatherdata`, headers in `/usr/include/plasma/weather/`) with
the ion plugins listed above. An ion implements `findPlaces()` and
`fetchForecast()` and delivers a `Forecast` with a `FutureDays` table model.

**Decision:** Do not depend on it. Talk to Open-Meteo directly.

Why: none of the shipped ions delivers 14 days. Using the library would mean
writing an Open-Meteo ion anyway, which ties the applet to kdeplasma-addons'
build and data model and to its release cycle. I did not find a documented
API stability promise. Its generic `FutureDays` model also adds a layer we
would have to adapt for "fallback probability" (see D3).

Possible later: package the Open-Meteo client additionally as a weather ion
so Plasma's own weather applet could use it.

## D3 – Rain probability for days 12–14: per-day fallback to GFS

Finding (live request, 2026-10-14, Vienna, `forecast_days=16`):

| `models=` | temperature days | probability days |
|---|---|---|
| `best_match` (default) | 16 | **11** (days 12+ are `null`) |
| `ecmwf_ifs025` | 15 | 11 |
| `icon_seamless` | 7 | 7 |
| `gfs_seamless` | 16 | **16** |

The probability is derived from ensemble runs, which end earlier than the
deterministic forecast. The ECMWF ensemble (Ensemble API, 51 members) also
ended after day 11 in this test.

**Decision:** Request `models=best_match,gfs_seamless` in one call (the
response suffixes every variable with `_best_match` / `_gfs_seamless`). Per
day, use `best_match` values; where a `best_match` value is `null`, take
the GFS value and mark that day's value as a fallback. If both are `null`,
show "no data". Do not compute a probability ourselves.

The model exposes the fallback flag so the UI can show it subtly (for example
dimmed or with a tooltip "lower confidence, GFS"). Re-check this behaviour
from time to time, because model horizons change.

Alternatives rejected (2026-10-14):
- **GFS only** (`models=gfs_seamless`): simplest, because it has every value
  for all 14 days, so no merging and no fallback flag. But it is a coarser
  global model. In a test for Vienna, its minimum temperatures for days 1–5
  were 2–4 °C above `best_match`, which uses high-resolution regional
  models in Europe. The near days matter most, so accuracy wins over the
  few lines of merge code.
- **`best_match` only**: no precipitation probability for days 12–14, which
  violates the 14-day requirement.

## D4 – Architecture: C++ backend, QML frontend

Follows the "QML for UI, C++ for logic" convention in CLAUDE.md. A pure QML
applet with `XMLHttpRequest` is therefore ruled out.

Planned C++ types, exposed via `QML_ELEMENT`:

- **`OpenMeteoClient`** (internal, not a QML type): builds request URLs,
  owns the `QNetworkAccessManager`, sets the User-Agent and parses the JSON
  into plain value structs (`DailyForecast { QDate date; double tMin, tMax;
  std::optional<int> precipProbability; bool probabilityIsFallback;
  int weatherCode; }`). Parsing is a free function or static method on raw
  `QByteArray`, so tests run without the network.
- **`ForecastModel`** (`QAbstractListModel`): one row per day. Roles: date,
  min, max, probability, probability fallback flag, weather code, icon name.
- **`WeatherService`** (QML type the UI instantiates): properties for the
  location (latitude, longitude, name), units and refresh interval as input;
  `model`, `status` (idle/loading/ready/error), `errorString`, `lastUpdated`
  as output; `Q_INVOKABLE refresh()`. It owns the refresh timer and the
  cache.
- **`LocationSearchModel`** (`QAbstractListModel`): wraps geocoding for the
  location picker in the config dialog (search text in, result rows out),
  with a short debounce.
- **Weather code → icon**: map the WMO codes to freedesktop/Breeze weather
  icon names (`weather-clear`, `weather-few-clouds`, `weather-overcast`,
  `weather-fog`, `weather-showers`, `weather-snow`, `weather-storm`, …) in
  C++, as a role of `ForecastModel`.

QML: `main.qml` (`PlasmoidItem`), a compact representation for panels
(today's icon and temperature) and a full representation (14-row or
14-column list bound to `WeatherService.model`), and the config page with the
location search.

Configuration (location, units, refresh interval) is stored through Plasma's
applet configuration (`Plasmoid.configuration`). QML binds those values to
`WeatherService` properties. **Open:** how `plasma_add_applet` packages the
`config/main.xml` and config QML files for a compiled applet has to be
verified when implementing it.

## D5 – Refresh, caching, units

- **Refresh interval:** default 60 min, configurable (minimum 15 min). Daily
  values change at most with model runs (every few hours), so more frequent
  requests add nothing. Also refresh on network reconnect and on resume, if
  the data is older than the interval.
- **Retries:** Open-Meteo sometimes does not answer a connection attempt at
  all, while the next one succeeds (measured 2026-10-14: about one in five
  connections, on both API hosts, other servers unaffected). Transient
  failures (no HTTP response, 5xx) are retried after 2 s, 10 s and 30 s,
  then every 5 minutes; other HTTP errors (e.g. 429) only every 5 minutes.
  The failure is shown only after the quick retries. Request timeout 15 s
  (answers normally take well under a second). Logic in `src/retry.h`,
  failures are logged in the category `io.github.leghissa.cumulus`.
- **Cache:** store the last successful raw response per location in
  `QStandardPaths::CacheLocation`. On startup or when offline, show the
  cached data with its age, so the applet is never empty after login.
- **Units:** derived from `QLocale::system().measurementSystem()` (follows
  `LC_MEASUREMENT`, verified), no setting; mapping in ui-guidelines.md.
  Pass `temperature_unit`, `wind_speed_unit` and `precipitation_unit` to the
  API and include them in the cache key.
- **Days:** always request 14 (`forecast_days=14`), `timezone=auto`.

## D6 – Testing

Add QtTest unit tests via ECM (`ecm_add_test`) for the parts that contain
logic: JSON parsing incl. `null` handling and the GFS fallback, the WMO code →
icon mapping, `ForecastModel` roles. Use recorded API responses as fixture
files. No network in tests. UI checks stay manual via
`tools/run-plasmoidviewer.sh`.

## D7 – Charts: KQuickCharts, composed in QML

**Decision:** Draw the temperature lines with [KQuickCharts](https://api.kde.org/kquickcharts-index.html)
(KF6, `org.kde.quickcharts`) `LineChart`, with a fixed value range from the
model (`temperatureMinimum`/`temperatureMaximum`). Precipitation bars,
value labels and column backgrounds are plain QML items per column
(implemented 2026-10-14): they must line up exactly with the fixed columns
and carry labels at the bar's top edge, which `BarChart` does not offer.
The column headers (weekday/hour, icon, wind) are a QML `Repeater` aligned to
the chart columns. All of it is one reusable QML component
(`ForecastChart.qml`) that is fed by a C++ model, used for both the daily and
the hourly chart.

Why:
- It is a KDE framework, already installed with Plasma (it is used by
  Plasma System Monitor), and it follows the Plasma theme and colours.
- It reads data straight from a `QAbstractItemModel` via `ModelSource`
  (one source per role), which fits `ForecastModel` without glue code.
- Lightweight, made for small embedded charts, which suits an applet popup.

Alternatives:
- **Qt Graphs** (`QtGraphs`, Qt ≥ 6.8, `GraphsView` with line and bar
  series): capable, but not installed by default (`qt6-graphs` on Arch, an
  extra dependency), has its own theming, and is heavier than needed.
- **Qt Charts**: deprecated in favour of Qt Graphs.
- **Own `QQuickPaintedItem`/`Canvas`**: full control, but re-implements
  scaling, theming and HiDPI. Only if KQuickCharts turns out not to be enough.

Both charts use a fixed column width (4.5 grid units; the visible-columns
setting only sizes the popup initially) and scroll horizontally (see ui-guidelines.md). The chart component
sits in a horizontal `Flickable` with content width = rows × column width;
the column headers are part of the same content, so they scroll with it.
No fitting to daylight windows, so no variable zoom.

- `ForecastModel` (daily): 14 rows, today first.
- `HourlyModel`: one row per hour from the current hour to the end of day
  14 (past hours are not in the model). Roles include `isNight` (from
  sunrise/sunset) and `dayIndex` (calendar day in the location's time
  zone). `Q_INVOKABLE firstHourOfDay(dayIndex)` gives the row to scroll to
  when a day is selected: the row of 00:00 of that day, or row 0 for today.
- QML maps the first visible row to the selected day via `dayIndex` and
  keeps the daily chart's selection in view.

A C++ `QQuickItem` "custom element" is therefore not needed for the chart
itself. The custom parts are the C++ models behind it, in line with the QML/C++
convention.

## D8 – Daily and hourly data in one request

The hourly chart needs hourly `temperature_2m`, `precipitation_probability`,
`precipitation`, `wind_speed_10m`, `weather_code` for all 14 days. Test
(2026-10-14): hourly values exist for all 14 days, but
`precipitation_probability` with `best_match` ends at day 11 like the daily
value, so the GFS fallback from D3 applies per hour, too.

**Decision:** One request per refresh with both `daily` and `hourly`
variables and `models=best_match,gfs_seamless`. The response is cached as a
whole. Clicking a day needs no further network request.

Daily values come from the API's daily aggregates: `temperature_2m_max/min`,
`precipitation_probability_max`, `precipitation_sum`, `wind_speed_10m_mean`,
`sunrise`, `sunset`
(km/h by default, `wind_speed_unit` selects km/h, m/s, mph, kn).

### Day icon: computed from hourly codes

The daily `weather_code` of the API is the *most severe* condition of the
day (one foggy hour turns the whole day into "fog"). In a test for Vienna on
2026-10-14 it differed from a computed value on 10 of 14 days. So the day
icon is derived in C++ (pure function, unit tested) from the hourly
`weather_code` values:

1. **Window:** only hours between `sunrise` and `sunset` of that day (both
   requested as daily variables). For today, only the remaining hours, from
   the current hour on. If no hours remain (after sunset), use the hours from
   now until midnight. If that is empty as well, use all hours of the day.
2. **Precipitation first:** an hour is wet if its code is drizzle, rain,
   snow, showers or thunderstorm (WMO ≥ 51). If the share of wet hours in
   the window reaches the mode's threshold, the day shows precipitation:
   the most frequent wet code (pessimistic mode: the most severe one).
3. **Otherwise clouds/fog:** take a quantile of the dry hours' codes, ranked
   by severity `0 < 1 < 2 < 3 < 45 < 48` (clear → overcast → fog).

| Mode (setting) | Wet threshold | Quantile of dry hours |
|---|---|---|
| Optimistic | ≥ 50 % of the hours | 25th percentile |
| Average (default) | ≥ 25 % (≈ 3 h of a winter day) | median |
| Pessimistic | at least 1 wet hour | 75th percentile |

**Median, not mean:** weather codes are categories with an order, not
measurements. A mean of the ranks produces "in-between" values that no hour
had and depends on the arbitrary spacing of the ranks (e.g. two foggy hours
pulled a partly cloudy day to "overcast" in the test). The median always
returns a code that really occurred and ignores single outliers. Quantiles
use the nearest-rank method, so the result is always an existing code.

The thresholds are a starting point and are fixed by unit tests, so they can
be tuned later. The hourly chart shows each hour's own code, without any
aggregation.

## D9 – Packaging: Plasma package with a C++ QML module inside

The applet started as a compiled applet (`plasma_add_applet`, one `.so`
with embedded QML). Plasma loads compiled applets only from Qt plugin paths,
and there is no per-user plugin directory, so a non-root install needed
`QT_PLUGIN_PATH` set for the Plasma session through a login script.

**Decision (2026-10-14):** Ship a regular Plasma package (`package/`:
`metadata.json`, `contents/ui`, `contents/config`) with the C++ QML module
built into `contents/lib/io/github/leghissa/cumulus/`. The QML files import
it by relative path (`import "../lib/io/github/leghissa/cumulus"`).

Why:
- Qt loads the C++ plugins listed in the `qmldir` of an imported local
  directory (documented for directory imports). Verified: a test package
  installed with `kpackagetool6` into `~/.local/share/plasma/plasmoids/`
  and loaded by ID used its package-local C++ plugin without any
  environment variable.
- The same package works in `~/.local/share/plasma/plasmoids/` and
  `/usr/share/plasma/plasmoids/`, so user and system installs differ only
  in the prefix (targets `install-user`, `install-system`).
- No login script, no `QT_PLUGIN_PATH`, no re-login after the first
  install. A plasmashell restart is enough, and the install targets do it.
- The C++/QML split (CLAUDE.md conventions) is kept.

Trade-offs:
- KDE documents C++ widgets as a QML package plus a private QML plugin in
  the system QML import path. A plugin inside the package is not described
  there (neither recommended nor ruled out); it relies on the Qt mechanism
  above.
- QML files are no longer compiled into the plugin, so QML errors show up at
  runtime instead of build time.
- Widgets with compiled code cannot be distributed through the KDE Store
  (KDE documentation); distribution would be via distro packages (e.g. AUR).
- A user install of the same ID takes precedence over a system install.

## Sources

- Open-Meteo Forecast API: https://open-meteo.com/en/docs
- Open-Meteo Geocoding API: https://open-meteo.com/en/docs/geocoding-api
- Open-Meteo terms (limits, CC BY 4.0): https://open-meteo.com/en/terms
- MET Norway Locationforecast: https://api.met.no/weatherapi/locationforecast/2.0/documentation
- Provider comparison (free tiers): https://apibouncer.com/guides/best-free-weather-apis
- KQuickCharts: https://api.kde.org/kquickcharts-index.html
- Qt directory imports: https://doc.qt.io/qt-6/qtqml-syntax-directoryimports.html
- KDE widget C++ API: https://develop.kde.org/docs/plasma/widget/c-api/
- Plasma weather library: kdeplasma-addons 6.7.4, `/usr/include/plasma/weather/`

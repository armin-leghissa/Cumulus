# UI guidelines

How Cumulus looks and behaves. Technical decisions are in
[decisions.md](decisions.md).

## Icons

- Use icons from the system icon theme only (freedesktop weather icon
  names such as `weather-clear`, `weather-few-clouds`, `weather-showers`,
  looked up via the theme, e.g. `Kirigami.Icon { source: "weather-clear" }`).
- Do not bundle own weather icons. They would not follow the user's theme,
  colour scheme or light/dark mode.

## Panel (compact representation)

- Default: only the weather icon of today.
- Optional, by setting: today's temperature overlaid on the icon, either
  - max only (e.g. `18°`), or
  - min and max (e.g. `7°/18°`).
- The text sits on top of the icon and must stay readable on any icon and
  panel background (e.g. text outline/shadow or a subtle backdrop) and scale
  with the panel size.

## Popup (full representation)

- Clicking the panel icon opens a popup with the 14-day chart.
- The popup always shows both charts: the 14-day chart and the hourly
  chart, the hourly one above or below it (setting, default below).
- Today is selected when the popup opens. Clicking a day selects it and
  scrolls the hourly chart to it (see below).

## Charts

Both charts (14 days and hourly) share one look and are built from the same
QML component; only the data model and the column labels differ.

Layout and scrolling, for both charts:
- Fixed column width. The number of visible columns is a setting (default
  5); column width = chart width / that number. No fitting to day or
  daylight lengths.
- Horizontally scrollable (swipe/drag, mouse wheel). Never into the past:
  the daily chart starts at today, the hourly chart at the current hour.
- The value axes are fixed over all data of the chart, so the scale does
  not jump while scrolling.

Daily chart, one column per day:
- Column header: localized short weekday name, without trailing dot
  (`Qt.locale().standaloneDayName(day, Locale.ShortFormat)`, gives
  "Mo, Di, …" in German, "Mon, Tue, …" in English).
- Weather icon representing the day's overall weather (system icon theme),
  condensed according to the day icon mode setting.
- Average wind speed of the day as a number.
- Red line: maximum temperature.
- Blue line: minimum temperature.
- Semi-transparent light blue bars: precipitation probability (bar height,
  0–100 %). The precipitation amount is shown as a label at the inner top
  edge of the bar (e.g. `2.4 mm`); no label when the amount is 0.
- Days whose probability comes from the GFS fallback (see decisions.md D3)
  are shown slightly dimmed.

Hourly chart, always visible above or below the daily chart:
- One continuous timeline from the current hour to the end of the forecast
  (day 14), across midnights, one column per full hour. Day and night hours
  appear in sequence; nothing is skipped.
- Column header: the hour, plus icon, wind and precipitation bars as in the
  daily chart.
- A single red line for the hourly temperature (there is no min/max per
  hour).
- Night hours (between sunset and the next sunrise) get the theme's
  background colour, slightly darkened (e.g. `Qt.darker(Kirigami.Theme.backgroundColor, 1.1)`),
  so the day/night boundary is visible in light and dark themes.
- Selection follows scrolling: the calendar day (local midnight to
  midnight, not daylight) of the first visible hour is the selected day in
  the 14-day chart. If it is scrolled out of view there, the
  daily chart scrolls along so the selection stays visible.
- Selecting a day in the daily chart scrolls the timeline (animated) to
  00:00 of that day, or to the current hour for today. Day boundaries are
  calendar days; sunrise/sunset only affect the night shading.

## Units

- No unit settings. Units follow the system's measurement setting
  (System Settings → Region & Language → Measurement Units, i.e.
  `LC_MEASUREMENT`), read via `QLocale::system().measurementSystem()`:

  | Measurement system | Temperature | Wind | Precipitation |
  |---|---|---|---|
  | Metric | °C | km/h | mm |
  | Imperial (US) | °F | mph | inch |
  | Imperial (UK) | °C | mph | mm |

- A manual override in the settings can be added later if it is ever needed.

## Settings

- Location, chosen via a place search (Open-Meteo geocoding).
- Temperature overlay on the panel icon: off (default), max only, or min
  and max.
- Refresh interval, default 60 min, minimum 15 min.
- Number of visible chart columns, default 5 (applies to both charts).
- Position of the hourly chart: below (default) or above the 14-day chart.
- Day icon mode: optimistic, average (default) or pessimistic. Decides how
  the hours of a day are condensed into one icon (see decisions.md D8).

Nothing else. Units are automatic (see above).

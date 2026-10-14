# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Cumulus is a KDE Plasma 6 applet (plasmoid) with a mixed C++/QML implementation, generated from KDevelop's "Plasma Applet mixed C++/QML" template. It is still at the skeleton stage: the C++ and QML are template code that will be replaced.

Stack: Qt ≥ 6.7 (Quick, Core, Gui), KF6 (I18n, Config), Plasma 6, ECM/CMake ≥ 3.16.

## Build & run

The KDevelop project builds out of source into `../Cumulus_build` (Debug):

```bash
cmake -S . -B ../Cumulus_build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON   # add -DCMAKE_INSTALL_PREFIX=<prefix> to install elsewhere
cmake --build ../Cumulus_build
tools/run-plasmoidviewer.sh   # run from the build dir without installing (CUMULUS_BUILD_DIR overrides the build dir)
```

This is a compiled C++ applet: QML and assets are embedded in the plugin `.so` (`bin/plasma/applets/`). Load it **by plugin ID** with `QT_PLUGIN_PATH` pointing at the build's `bin/`, which is what the script does, never by path. `plasmoidviewer -a src/` treats the directory as a pure-QML KPackage, finds no `contents/ui/main.qml` and fails with "File name empty! … Invalid empty URL". The KDevelop launch config (`.kdev4/Cumulus.kdev4`, not versioned) is a native launcher that runs this script after building the target. KDevelop rejects shell metacharacters, including `=`, in launcher arguments, so environment variables can't be set there inline.

To use it in the real shell, `cmake --install ../Cumulus_build` (prefix `/usr`, needs root), then `kquitapp6 plasmashell && plasmashell`. A stale build dir can still contain outputs under an old plugin ID in `bin/plasma/`.

Tests (QtTest via `ecm_add_tests`, in `autotests/`, no network; `autotests/data/` holds a recorded API response):

```bash
ctest --test-dir ../Cumulus_build --output-on-failure
ctest --test-dir ../Cumulus_build -R weathercodetest   # single test; or run ../Cumulus_build/bin/weathercodetest <testfunction>
```

plasmoidviewer resets its applet configuration on every start, so it always shows the "not configured" state. To test with a location, use `QT_PLUGIN_PATH=../Cumulus_build/bin plasmawindowed io.github.leghissa.cumulus`, which keeps its config in `~/.config/plasmawindowedrc` (`[Applets][<id>][Configuration][General]`: `locationName`, `latitude`, `longitude`). The forecast cache is in `~/.cache/cumulus/`.

## Architecture

- `plasma_add_applet(io.github.leghissa.cumulus ... GENERATE_APPLET_CLASS)` in `src/CMakeLists.txt` builds a single plugin that bundles the QML (`QML_SOURCES`), the C++ (`CPP_SOURCES`), and resources such as images into a Qt resource. Any new `.qml`, `.cpp`, or asset file must be added to the matching list there. `GENERATE_APPLET_CLASS` means there is no hand-written `Plasma::Applet` subclass.
- C++ is split in two: `cumulus_core` (static library: `openmeteo` request/parsing, `weathercode` day icon rule and icon names, `forecastrows` display rows, `units`) has no Qt Quick or network dependency and is what the unit tests link against. The applet target adds the QML-facing classes: `WeatherService` (network, cache, refresh timers) and the list models in `forecastmodels`.
- C++ types reach QML through declarative registration (`QML_ELEMENT`, `QML_UNCREATABLE`). Types compiled into the applet target are visible in its QML files without an import. `qml/Format.qml` is a QML singleton for display formatting (`QT_QML_SINGLETON_TYPE` in `src/CMakeLists.txt`).
- `src/qml/main.qml` is the root item. It must be a `PlasmoidItem`, which defines `fullRepresentation` (and optionally `compactRepresentation`). Files in `QML_SOURCES`/`RESOURCES` end up flat in the applet's QML module (`qrc:/qt/qml/plasma/applet/io/github/leghissa/cumulus/`), so `config/main.xml` is found as `main.xml`. Representations live in their own files and get the `WeatherService` passed in as a required property.
- `src/config/main.xml` defines the applet settings (`Plasmoid.configuration.*`).
- `src/metadata.json` is the KPackage metadata: the plugin ID comes from the CMake target name, and the file also holds the display name, icon, category, and `X-Plasma-API-Minimum-Version`.
- Translations use the domain `plasma_applet_io.github.leghissa.cumulus`, set with `TRANSLATION_DOMAIN` in `src/CMakeLists.txt`. Use `i18n*()` in both C++ and QML.

## Plugin ID

The plugin ID is `io.github.leghissa.cumulus` (third-party reverse-domain naming; `org.kde.plasma.*` is reserved for Plasma itself). Renaming it means changing the CMake target, `TRANSLATION_DOMAIN`, and the `.pot` name in `Messages.sh` together. `Messages.sh` is the xgettext extraction script for KDE's i18n infrastructure.

## Design

Details and reasoning live in `docs/decisions.md` (technical) and `docs/ui-guidelines.md` (UI). Read them before implementing a feature. Short version:
- Data comes from Open-Meteo (forecast and geocoding, no API key, CC BY 4.0 attribution required). One request per refresh fetches daily and hourly data with `models=best_match,gfs_seamless`. GFS fills in precipitation probability where `best_match` has none (days 12–14). The last response is cached.
- C++ provides the API client, the data models and a `WeatherService` QML type. Charts use KQuickCharts in a reusable QML component for both the daily and the hourly chart.
- Panel: today's weather icon, optionally with a max or min/max temperature overlay. Popup: the 14-day chart and an hourly chart, which is always visible below or above it (setting). The two are synced by the selected day, today by default.
- The day icon is computed in C++ from the hourly codes between sunrise and sunset, not taken from the API's daily code (which shows the worst condition of the day). A setting selects optimistic, average (median) or pessimistic.
- Icons come only from the system icon theme. Units follow the system measurement setting (`QLocale::system().measurementSystem()`), with no unit setting.

## Conventions

**Language:** Code, identifiers, comments, commit messages and repository documentation (README, CLAUDE.md, docs/) are in English. Talk to the user in German. Never use gendering, only correct and precise grammar.

**QML for UI, C++ for logic:**
- QML holds the UI structure: layout, visuals, Plasma/Kirigami components, bindings, animations, and purely presentational state (e.g. which view is expanded).
- C++ holds the program logic: networking, parsing, data models (`QAbstractListModel` subclasses), caching, unit conversion, time and date calculations, configuration handling, and anything else that is non-trivial or worth unit testing.
- C++ is exposed to QML as custom QML types or helper classes (`QML_ELEMENT` / `QML_SINGLETON`) via properties, signals, and `Q_INVOKABLE`. QML only binds and reacts to them.
- Keep JavaScript in QML to small expressions and display formatting. No `XMLHttpRequest`, data processing or business rules in QML/JS.

License: LGPL-2.1-or-later (SPDX headers on source files).

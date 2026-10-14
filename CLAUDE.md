# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Cumulus is a KDE Plasma 6 applet (plasmoid) with a mixed C++/QML implementation, generated from KDevelop's "Plasma Applet mixed C++/QML" template. It is still at the skeleton stage: the C++ and QML are template code that will be replaced.

Stack: Qt ≥ 6.7 (Quick, Core, Gui), KF6 (I18n, Config), Plasma 6, ECM/CMake ≥ 3.16.

## Build & run

The KDevelop project builds out of source into `../Cumulus_build` (Debug):

```bash
cmake -S . -B ../Cumulus_build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build ../Cumulus_build
tools/run-plasmoidviewer.sh   # plasmoidviewer -a ../Cumulus_build/package (CUMULUS_BUILD_DIR overrides the build dir)
```

The applet is a Plasma package, assembled by the build in `../Cumulus_build/package` (docs/decisions.md D9): `package/` from the source tree is copied there (target `cumulus_package`), and the C++ QML module is built into `contents/lib/io/github/leghissa/cumulus/`. The QML files import it by relative path, so neither `QT_PLUGIN_PATH` nor `QML_IMPORT_PATH` is needed. The KDevelop launch config (`.kdev4/Cumulus.kdev4`, not versioned) is a native launcher that runs the script after building the project (KDevelop rejects shell metacharacters such as `=` in launcher arguments).

Install targets (top-level `CMakeLists.txt`): `install-user` (`~/.local`), `install-system` (`/usr` via `pkexec`), `uninstall-user`, `uninstall-system`; plain `install`/`uninstall` follow the cache variable `CUMULUS_INSTALL_MODE` (USER by default). Everything installs into `<prefix>/share/plasma/plasmoids/io.github.leghissa.cumulus/`, and all targets restart plasmashell via `tools/restart-plasmashell.sh` afterwards. **When installing yourself to test, set `CUMULUS_NO_RESTART=1`** so the user's running shell is not restarted.

Tests (QtTest via `ecm_add_tests`, in `autotests/`, no network; `autotests/data/` holds a recorded API response):

```bash
ctest --test-dir ../Cumulus_build --output-on-failure
ctest --test-dir ../Cumulus_build -R weathercodetest   # single test; or run ../Cumulus_build/bin/weathercodetest <testfunction>
```

plasmoidviewer resets its applet configuration on every start and on every form factor switch, so it then shows the "not configured" state (Plasma's green placeholder icon with a "Configure…" button). To test with a location, install with `CUMULUS_NO_RESTART=1 cmake --build ../Cumulus_build --target install-user` and run `plasmawindowed io.github.leghissa.cumulus`, which keeps its config in `~/.config/plasmawindowedrc` (`[Applets][<id>][Configuration][General]`: `locationName`, `latitude`, `longitude`). The forecast cache is in `~/.cache/cumulus/`. Network failures and retries are logged in the category `io.github.leghissa.cumulus` (warnings by default), e.g. `journalctl --user -u plasma-plasmashell.service | grep leghissa`.

## Architecture

- `package/` is the applet package: `metadata.json` (with `KPlugin.Id`), `contents/ui/*.qml`, `contents/config/main.xml` and `config.qml`. QML files are plain files, not compiled into a resource; new ones need no CMake change, but QML components used by name must be listed in `contents/ui/qmldir` (it also declares the `Format` singleton).
- C++ is split in two: `cumulus_core` (static library: `openmeteo` request/parsing, `weathercode` day icon rule and icon names, `forecastrows` display rows, `units`) has no Qt Quick or network dependency and is what the unit tests link against. The QML module `cumulusplugin` (URI `io.github.leghissa.cumulus`, `src/CMakeLists.txt`) adds the QML-facing classes: `WeatherService` (network, cache, refresh timers), the list models in `forecastmodels` and `LocationSearchModel`. Its library must be named `libcumulusplugin.so` (`PREFIX "lib"`), or QML reports "plugin not found".
- C++ types reach QML through declarative registration (`QML_ELEMENT`, `QML_UNCREATABLE`). Every QML file that uses them has `import "../lib/io/github/leghissa/cumulus"`.
- `contents/ui/main.qml` is the root item. It must be a `PlasmoidItem`, which defines `fullRepresentation` and `compactRepresentation`. Representations live in their own files and get the `WeatherService` passed in as a required property.
- `contents/config/main.xml` defines the applet settings (`Plasmoid.configuration.*`). `contents/config/config.qml` lists the settings pages (in `contents/ui/`); a page binds each entry as `cfg_<name>` and must also declare `cfg_<name>Default`, otherwise Plasma 6 logs "Setting initial properties failed".
- Translations use the domain `plasma_applet_io.github.leghissa.cumulus` (derived from the plugin ID for packages). Use `i18n*()` in QML.

## Plugin ID

The plugin ID is `io.github.leghissa.cumulus` (third-party reverse-domain naming; `org.kde.plasma.*` is reserved for Plasma itself). Renaming it means changing `KPlugin.Id` in `package/metadata.json`, `CUMULUS_PLUGIN_ID` in `CMakeLists.txt`, and the `.pot` name in `Messages.sh` together. `Messages.sh` is the xgettext extraction script for KDE's i18n infrastructure.

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

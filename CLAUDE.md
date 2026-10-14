# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Cumulus is a KDE Plasma 6 applet (plasmoid) with a mixed C++/QML implementation, generated from KDevelop's "Plasma Applet mixed C++/QML" template. It is still at the skeleton stage: the C++ and QML are template code that will be replaced.

Stack: Qt ≥ 6.7 (Quick, Core, Gui), KF6 (I18n, Config), Plasma 6, ECM/CMake ≥ 3.16.

## Build & run

The KDevelop project builds out of source into `../Cumulus_build` (Debug):

```bash
cmake -S . -B ../Cumulus_build -DCMAKE_BUILD_TYPE=Debug   # add -DCMAKE_INSTALL_PREFIX=<prefix> to install elsewhere
cmake --build ../Cumulus_build
tools/run-plasmoidviewer.sh   # run from the build dir without installing (CUMULUS_BUILD_DIR overrides the build dir)
```

This is a compiled C++ applet: QML and assets are embedded in the plugin `.so` (`bin/plasma/applets/`). Load it **by plugin ID** with `QT_PLUGIN_PATH` pointing at the build's `bin/`, which is what the script does, never by path. `plasmoidviewer -a src/` treats the directory as a pure-QML KPackage, finds no `contents/ui/main.qml` and fails with "File name empty! … Invalid empty URL". The KDevelop launch config (`.kdev4/Cumulus.kdev4`, not versioned) is a native launcher that runs this script after building the target. KDevelop rejects shell metacharacters, including `=`, in launcher arguments, so environment variables can't be set there inline.

To use it in the real shell, `cmake --install ../Cumulus_build` (prefix `/usr`, needs root), then `kquitapp6 plasmashell && plasmashell`. A stale build dir can still contain outputs under an old plugin ID in `bin/plasma/`.

The repository has no tests or lint setup.

## Architecture

- `plasma_add_applet(io.github.leghissa.cumulus ... GENERATE_APPLET_CLASS)` in `src/CMakeLists.txt` builds a single plugin that bundles the QML (`QML_SOURCES`), the C++ (`CPP_SOURCES`), and resources such as images into a Qt resource. Any new `.qml`, `.cpp`, or asset file must be added to the matching list there. `GENERATE_APPLET_CLASS` means there is no hand-written `Plasma::Applet` subclass.
- C++ types reach QML through declarative registration (`QML_ELEMENT` / `QML_SINGLETON` plus a static `create()` factory). See `Cumulus` in `src/cumulus.h`. QML uses these types directly, with no import or `qmlRegisterType` call.
- `src/qml/main.qml` is the root item. It must be a `PlasmoidItem`, which defines `fullRepresentation` (and optionally `compactRepresentation`). Resources resolve relative to the QML file, for example `Qt.resolvedUrl("./pairs.svg")`.
- `src/metadata.json` is the KPackage metadata: the plugin ID comes from the CMake target name, and the file also holds the display name, icon, category, and `X-Plasma-API-Minimum-Version`.
- Translations use the domain `plasma_applet_io.github.leghissa.cumulus`, set with `TRANSLATION_DOMAIN` in `src/CMakeLists.txt`. Use `i18n*()` in both C++ and QML.

## Plugin ID

The plugin ID is `io.github.leghissa.cumulus` (third-party reverse-domain naming; `org.kde.plasma.*` is reserved for Plasma itself). Renaming it means changing the CMake target, `TRANSLATION_DOMAIN`, and the `.pot` name in `Messages.sh` together. `Messages.sh` is the xgettext extraction script for KDE's i18n infrastructure.

## Conventions

**Language:** Code, identifiers, comments, commit messages and repository documentation (README, CLAUDE.md, docs/) are in English. Talk to the user in German.

**QML for UI, C++ for logic:**
- QML holds the UI structure: layout, visuals, Plasma/Kirigami components, bindings, animations, and purely presentational state (e.g. which view is expanded).
- C++ holds the program logic: networking, parsing, data models (`QAbstractListModel` subclasses), caching, unit conversion, time and date calculations, configuration handling, and anything else that is non-trivial or worth unit testing.
- C++ is exposed to QML as custom QML types or helper classes (`QML_ELEMENT` / `QML_SINGLETON`) via properties, signals, and `Q_INVOKABLE`. QML only binds and reacts to them.
- Keep JavaScript in QML to small expressions and display formatting. No `XMLHttpRequest`, data processing or business rules in QML/JS.

License: LGPL-2.1-or-later (SPDX headers on source files).

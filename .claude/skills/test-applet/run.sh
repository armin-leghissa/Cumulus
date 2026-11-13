#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Shows the installed applet in plasmawindowed once per locale, screenshots its window and collects the log.
#   run.sh <output dir> [locale ...]     e.g. run.sh /tmp/shots ru_RU en_US (no locale: the user's own)
# CUMULUS_WAIT (seconds, default 15) is how long to wait for the window and the forecast.

set -uo pipefail

out="${1:?usage: run.sh <output dir> [locale ...]}"
shift
locales=("$@")
[[ ${#locales[@]} -eq 0 ]] && locales=(system)
wait="${CUMULUS_WAIT:-15}"
mkdir -p "$out"

if pgrep -x plasmawindowed > /dev/null; then
    echo "plasmawindowed is already running, stop it first" >&2
    exit 1
fi

# Activates the window of process $1 through a KWin script and succeeds only if it is then the active
# window. Without this check spectacle would capture whatever window has the focus, e.g. the user's mail.
activate_window() {
    local pid=$1 name="cumulus-test-$$-$1" script since id
    script="$(mktemp --suffix=.js)"
    printf '%s\n' \
        "for (const w of workspace.windowList()) {" \
        "    if (w.pid === $pid && w.normalWindow) {" \
        "        workspace.activeWindow = w;" \
        "    }" \
        "}" \
        "const active = workspace.activeWindow;" \
        "print(\"$name active pid=\" + (active ? active.pid : -1));" > "$script"
    since="$(date '+%Y-%m-%d %H:%M:%S')"
    id="$(qdbus6 org.kde.KWin /Scripting org.kde.kwin.Scripting.loadScript "$script" "$name")"
    qdbus6 org.kde.KWin "/Scripting/Script$id" org.kde.kwin.Script.run > /dev/null
    sleep 1
    qdbus6 org.kde.KWin /Scripting org.kde.kwin.Scripting.unloadScript "$name" > /dev/null
    rm -f "$script"
    journalctl --user --since "$since" -o cat | grep -q "$name active pid=$pid\$"
}

for locale in "${locales[@]}"; do
    if [[ $locale != system ]] && ! locale -a | grep -qix "$locale\.utf-\?8"; then
        echo "$locale: locale is not generated on this system (locale -a), skipped" >&2
        continue
    fi
    log="$out/$locale.log"
    shot="$out/$locale.png"
    if [[ $locale == system ]]; then
        QT_FORCE_STDERR_LOGGING=1 plasmawindowed io.github.leghissa.cumulus > "$log" 2>&1 &
    else
        LC_ALL="$locale.UTF-8" LANG="$locale.UTF-8" LANGUAGE="${locale%%_*}" QT_FORCE_STDERR_LOGGING=1 \
            plasmawindowed io.github.leghissa.cumulus > "$log" 2>&1 &
    fi
    pid=$!
    sleep "$wait"
    # only the applet's window, never the whole screen or another window
    if activate_window "$pid"; then
        spectacle --activewindow --background --nonotify --output "$shot"
    else
        shot="none"
        echo "$locale: the applet window could not be activated, no screenshot taken" >&2
    fi
    kill -TERM "$pid" 2> /dev/null
    for _ in 1 2 3 4 5; do
        kill -0 "$pid" 2> /dev/null || break
        sleep 1
    done
    if kill -0 "$pid" 2> /dev/null; then
        kill -KILL "$pid"
        echo "$locale: plasmawindowed did not stop on SIGTERM, killed" >&2
    fi
    echo "$locale: screenshot $shot, log $log"
    grep -iE 'cumulus|leghissa|\.qml|warning|error' "$log" | grep -v 'isScreenUiReady' | sed "s/^/  /" | head -40
done

if pgrep -x plasmawindowed > /dev/null; then
    echo "a plasmawindowed instance is still running" >&2
    exit 1
fi

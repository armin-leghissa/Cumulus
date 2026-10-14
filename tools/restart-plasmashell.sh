#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Restarts plasmashell so it picks up an (un)installed applet.
# Does nothing for DESTDIR (packaging) installs, when run as root, outside a running Plasma session,
# or when CUMULUS_NO_RESTART is set.

if [[ -n "${DESTDIR:-}" || -n "${CUMULUS_NO_RESTART:-}" || "$(id -u)" -eq 0 ]]; then
    exit 0
fi

if systemctl --user --quiet is-active plasma-plasmashell.service 2>/dev/null; then
    echo "Restarting plasmashell (systemd user service)"
    systemctl --user restart plasma-plasmashell.service
elif pgrep -u "$(id -u)" -x plasmashell > /dev/null; then
    echo "Restarting plasmashell"
    kquitapp6 plasmashell && kstart plasmashell > /dev/null 2>&1
else
    echo "plasmashell is not running, no restart needed"
fi
exit 0

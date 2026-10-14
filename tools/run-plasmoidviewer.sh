#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Runs the applet in plasmoidviewer straight from the build directory, without installing.
# Build dir defaults to ../Cumulus_build next to the repository; override with CUMULUS_BUILD_DIR.
# Extra arguments are passed on to plasmoidviewer (default: --formfactor planar).

set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${CUMULUS_BUILD_DIR:-$repo_dir/../Cumulus_build}"

if [[ ! -d "$build_dir/bin/plasma/applets" ]]; then
    echo "No built applet found in $build_dir/bin/plasma/applets - build first." >&2
    exit 1
fi

if [[ $# -eq 0 ]]; then
    set -- --formfactor planar
fi

export QT_PLUGIN_PATH="$build_dir/bin${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}"
exec plasmoidviewer -a io.github.leghissa.cumulus "$@"

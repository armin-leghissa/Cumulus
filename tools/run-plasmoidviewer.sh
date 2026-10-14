#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Runs the applet in plasmoidviewer straight from the build directory, without installing.
# The build assembles the complete applet package (QML files plus C++ module) in <build dir>/package.
# Build dir defaults to ../Cumulus_build next to the repository; override with CUMULUS_BUILD_DIR.
# Extra arguments are passed on to plasmoidviewer (default: --formfactor planar).

set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${CUMULUS_BUILD_DIR:-$repo_dir/../Cumulus_build}"
package_dir="$build_dir/package"

if [[ ! -f "$package_dir/metadata.json" ]]; then
    echo "No applet package found in $package_dir - build first." >&2
    exit 1
fi

if [[ $# -eq 0 ]]; then
    set -- --formfactor planar
fi

exec plasmoidviewer -a "$package_dir" "$@"

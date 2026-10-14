#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Extracts the translatable strings and merges them into the translations in po/<lang>/.
#   tools/update-translations.sh            update all existing translations
#   tools/update-translations.sh de fr ...  additionally create translations for new languages
# New and changed strings end up untranslated or marked "fuzzy" and need to be translated.

set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
domain="plasma_applet_io.github.leghissa.cumulus"
cd "$repo_dir"

pot="$(mktemp --suffix=.pot)"
trap 'rm -f "$pot"' EXIT

# Same keywords as KDE's extraction (Messages.sh); QML is parsed like C++.
mapfile -t sources < <(find package src -name '*.qml' -o -name '*.cpp' | sort)
xgettext --from-code=UTF-8 -C --kde \
    -ci18n -ki18n:1 -ki18nc:1c,2 -ki18np:1,2 -ki18ncp:1c,2,3 \
    --package-name=cumulus --msgid-bugs-address=leghissa@zoho.eu \
    -o "$pot" "${sources[@]}"

for lang in "$@"; do
    if [[ ! -f "po/$lang/$domain.po" ]]; then
        mkdir -p "po/$lang"
        msginit --no-translator --locale="$lang" --input="$pot" --output-file="po/$lang/$domain.po"
    fi
done

for po in po/*/"$domain.po"; do
    msgmerge --quiet --update --backup=none --previous "$po" "$pot"
    printf '%s: ' "$po"
    msgfmt --check --statistics -o /dev/null "$po"
done

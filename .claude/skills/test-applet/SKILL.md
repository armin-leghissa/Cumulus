---
name: test-applet
description: Install the applet, restart plasmashell and check the popup visually (screenshots, optionally per locale) and in the log. Use after UI, translation or formatting changes, or when the user asks to test the applet.
argument-hint: "[locale ...]  e.g. ru_RU en_US"
---

# Test the applet

1. Build and run the unit tests:
   `cmake --build ../Cumulus_build && ctest --test-dir ../Cumulus_build --output-on-failure`
2. Install for the user **with** the plasmashell restart, so the user can try the change by hand right away:
   `cmake --build ../Cumulus_build --target install-user`
3. Show the applet and take screenshots, one run per locale from the arguments (none: the user's own locale):
   `.claude/skills/test-applet/run.sh <scratchpad>/test-applet $ARGUMENTS`
   The script starts `plasmawindowed io.github.leghissa.cumulus` (location from `~/.config/plasmawindowedrc`), activates its window through a KWin script and takes the screenshot only if that window is then the active one, stops the instance with SIGTERM, and prints the relevant log lines. Locales that are not generated are skipped. Raise `CUMULUS_WAIT` if the forecast was not loaded yet.
   Never take screenshots any other way (no full screen, no unchecked `spectacle -a`): after the plasmashell restart the focus is often on another window, e.g. the user's mail.
4. Before reading a screenshot, check its size (about 690×760 with the window shadow). If it is much larger, delete it unread and tell the user. Then read every screenshot and check it: texts translated, dates, numbers, units and the clock in the locale's format, nothing clipped or overlapping. Report new warnings from the log.
5. Show the user the screenshots that matter and summarise what you checked.

Notes:
- plasmawindowed is used instead of plasmoidviewer because it keeps the applet configuration and loads the applet by ID, which is what makes the package translations work.
- A locale needs to be generated on the system (`locale -a`); `en_US` also covers the imperial units and the 12-hour clock.
- The popup only, not the panel icon: for the panel, ask the user to look at their own panel after the restart.

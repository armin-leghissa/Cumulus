# SPDX-FileCopyrightText: 2026 Armin Leghissa <leghissa@zoho.eu>
# SPDX-License-Identifier: BSD-2-Clause

include(ECMAddTests)

ecm_add_tests(
    openmeteotest.cpp
    weathercodetest.cpp
    forecastrowstest.cpp
    LINK_LIBRARIES cumulus_core Qt6::Test
)

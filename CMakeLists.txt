cmake_minimum_required(VERSION 3.16)

project(plasma-cumulus VERSION 1.0)

set(QT_MIN_VERSION "6.7.0")
set(KF6_MIN_VERSION "6.0.0")

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FeatureSummary)

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Quick
    Core
    Network
)

find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS
    I18n
    Config
)

find_package(Plasma REQUIRED)

# QML-only runtime dependencies
find_package(KF6Kirigami ${KF6_MIN_VERSION} CONFIG)
set_package_properties(KF6Kirigami PROPERTIES TYPE RUNTIME)
find_package(KF6QuickCharts ${KF6_MIN_VERSION} CONFIG)
set_package_properties(KF6QuickCharts PROPERTIES TYPE RUNTIME)

add_subdirectory(src)

if(BUILD_TESTING)
    find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Test)
    add_subdirectory(autotests)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

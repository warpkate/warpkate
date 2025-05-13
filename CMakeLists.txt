cmake_minimum_required(VERSION 3.16)

# Project name and version
project(WarpKate VERSION 0.1.0)

# KDE Applications version, required for KDE applications
set(KF6_MIN_VERSION "6.0.0")
set(QT_MIN_VERSION "6.5.0")

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# ECM setup
find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)
include(ECMInstallIcons)
include(ECMQtDeclareLoggingCategory)
include(ECMSetupVersion)
include(ECMAddTests)
include(GenerateExportHeader)

# Required Qt components
find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS
    Core
    Widgets
    Network
)

# Required KDE Frameworks components
find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS
    Config
    I18n
    KIO
    Parts
    TextEditor
    CoreAddons
)

# Find Kate
find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS
    TextEditor
)

## Find qtermwidget for terminal functionality
#find_package(qtermwidget6 REQUIRED)

# Add C++17 support
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate proper version header
ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX WARPKATE
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/warpkate_version.h"
)

# Add subdirectories
add_subdirectory(src)
if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()

# Add icons if available
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/icons" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/icons/CMakeLists.txt")
    add_subdirectory(icons)
endif()

# Add resources if available
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/CMakeLists.txt")
    add_subdirectory(resources)
endif()

# Feature summary
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)


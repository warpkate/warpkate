cmake_minimum_required(VERSION 3.16)

# Project name and version
project(WarpKate VERSION 0.1.0)

# KDE Applications version, required for KDE applications
set(KF5_MIN_VERSION "5.80.0")
set(QT_MIN_VERSION "5.15.0")

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# ECM setup
find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
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
find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS
    Core
    Widgets
    Network
)

# Required KDE Frameworks components
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    Config
    I18n
    KIO
    Parts
    TextEditor
)

# Find Kate
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    TextEditor
)

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
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Add icons if available
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/icons")
    add_subdirectory(icons)
endif()

# Add resources if available
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources")
    add_subdirectory(resources)
endif()

# Feature summary
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)


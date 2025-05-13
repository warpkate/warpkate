# Define sources by component
# Core plugin components
set(CORE_SRCS
    core/warpkateplugin.cpp
    core/warpkateplugin.h
    core/warpkateview.cpp
    core/warpkateview.h
)

# Terminal components
set(TERMINAL_SRCS
    terminal/terminalemulator.cpp
    terminal/terminalemulator.h
    terminal/blockmodel.cpp
    terminal/blockmodel.h
    terminal/terminalblockview.cpp
    terminal/terminalblockview.h
    terminal/terminaloutputprocessor.cpp
    terminal/terminaloutputprocessor.h
    terminal/filelisting.cpp
    terminal/filelisting.h
#    terminal/qtermwidgetemulator.cpp
#    terminal/qtermwidgetemulator.h
)

# AI components
set(AI_SRCS
    ai/aiservice.cpp
    ai/aiservice.h
    ai/aiprovider.cpp
    ai/aiprovider.h
    ai/openai_provider.cpp
    ai/openai_provider.h
    ai/apikeymanager.cpp
    ai/apikeymanager.h
)

# UI components
set(UI_SRCS
    ui/conversationview.cpp
    ui/conversationview.h
    ui/commandinput.cpp
    ui/commandinput.h
)

# Configuration components
set(CONFIG_SRCS
    config/warpkateconfigpage.cpp
    config/warpkateconfigpage.h
    config/warpkatepreferencesdialog.cpp
    config/warpkatepreferencesdialog.h
)

# Utility components
set(UTIL_SRCS
    util/interactive_elements.cpp
    util/interactive_elements.h
)

# Combine all source groups
set(warpkate_SRCS
    ${CORE_SRCS}
    ${TERMINAL_SRCS}
    ${AI_SRCS}
    ${UI_SRCS}
    ${CONFIG_SRCS}
    ${UTIL_SRCS}
)

# UI files
ki18n_wrap_ui(warpkate_SRCS
    ui/warpkatewidget.ui
    ui/configwidget.ui
)

# Resources
qt6_add_resources(warpkate_SRCS core/warpkate.qrc)

# Add the plugin library
add_library(warpkateplugin MODULE ${warpkate_SRCS})

# Generate proper export macros for the plugin
generate_export_header(warpkateplugin
    BASE_NAME warpkate
    EXPORT_MACRO_NAME WARPKATE_EXPORT
    EXPORT_FILE_NAME warpkate_export.h
)

# Target properties
set_target_properties(warpkateplugin PROPERTIES
    OUTPUT_NAME "warpkateplugin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/kf6/ktexteditor"
)

# Link against required libraries
target_link_libraries(warpkateplugin
    PUBLIC
        KF6TextEditor
        KF6Parts
        KF6I18n
        KF6ConfigCore
        KF6ConfigGui
        KF6::KIOCore
        KF6::KIOWidgets
        KF6::KIOFileWidgets
        KF6CoreAddons
        KF6XmlGui
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
#                Qt6::Core5Compat
#        qtermwidget6
)

# Include directories
target_include_directories(warpkateplugin
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        /usr/include
        /usr/include/KF6
        /usr/include/KF6/KTextEditor
        /usr/include/KF6/KParts
        /usr/include/KF6/KXmlGui
        /usr/include/KF6/KIO
        /usr/include/KF6/KService
        /usr/include/KF6/KConfigWidgets
        /usr/include/KF6/KCoreAddons
        /usr/include/KF6/KSyntaxHighlighting
        /usr/include/KF6/KI18n
        /usr/include/KF6/KConfigGui
        /usr/include/KF6/KWidgetsAddons
        /usr/include/KF6/KConfigCore
        /usr/include/KF6/KConfig
        ${KF6TextEditor_INCLUDE_DIRS}
        ${Qt6Core_INCLUDE_DIRS}
)

# Install the plugin
install(TARGETS warpkateplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/ktexteditor)

# Install JSON metadata for plugin
install(FILES warpkateplugin.json DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/ktexteditor)

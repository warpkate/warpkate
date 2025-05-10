# Define sources
set(warpkate_SRCS
    warpkateplugin.cpp
    warpkateplugin.h
    warpkateview.cpp
    warpkateview.h
    terminalemulator.cpp
    terminalemulator.h
    blockmodel.cpp
    blockmodel.h
    warpprotocol.cpp
    warpprotocol.h
)

# UI files
ki18n_wrap_ui(warpkate_SRCS
    ui/warpkatewidget.ui
    ui/configwidget.ui
)

# Resources
qt5_add_resources(warpkate_SRCS warpkate.qrc)

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
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/kf5/ktexteditor"
)

# Link against required libraries
target_link_libraries(warpkateplugin
    PUBLIC
        KF5::TextEditor
        KF5::Parts
        KF5::I18n
        KF5::Config
        KF5::KIOWidgets
    PRIVATE
        Qt5::Core
        Qt5::Widgets
        Qt5::Network
)

# Include directories
target_include_directories(warpkateplugin
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
)

# Install the plugin
install(TARGETS warpkateplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/ktexteditor)

# Install JSON metadata for plugin
install(FILES warpkateplugin.json DESTINATION ${KDE_INSTALL_PLUGINDIR}/ktexteditor)


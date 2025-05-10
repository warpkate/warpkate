# WarpKate Build and Installation Instructions

This guide outlines the steps to build, install, and test the WarpKate plugin for Kate editor.

## Building from Source

1. Clone the repository:
   ```bash
   git clone https://github.com/warpkate/warpkate.git
   cd warpkate
   ```

2. Create a build directory and run CMake:
   ```bash
   mkdir -p build
   cd build
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   ```

3. Build the plugin:
   ```bash
   make -j$(nproc)
   ```

4. The built plugin is located at:
   ```
   build/bin/kf6/ktexteditor/warpkateplugin.so
   ```

## Installation

### System-wide Installation (Recommended)

1. Install the plugin using CMake:
   ```bash
   sudo cmake --build . --target install
   ```

   This will install both the plugin binary and its metadata file to the correct locations.

2. Alternative manual installation (if the above doesn't work):
   ```bash
   # For older KDE/KF5
   sudo mkdir -p /usr/lib64/qt6/plugins/ktexteditor/
   sudo cp bin/kf6/ktexteditor/warpkateplugin.so /usr/lib64/qt6/plugins/ktexteditor/
   sudo cp ../src/warpkateplugin.json /usr/lib64/qt6/plugins/ktexteditor/

   # For KF6
   sudo mkdir -p /usr/lib64/qt6/plugins/kf6/ktexteditor/
   sudo cp bin/kf6/ktexteditor/warpkateplugin.so /usr/lib64/qt6/plugins/kf6/ktexteditor/
   sudo cp ../src/warpkateplugin.json /usr/lib64/qt6/plugins/kf6/ktexteditor/
   ```

3. After installation, rebuild the KDE services cache:
   ```bash
   kbuildsycoca6 --noincremental
   ```

### User-level Installation (for testing)

1. Create the local plugins directory:
   ```bash
   mkdir -p ~/.local/lib64/qt6/plugins/kf6/ktexteditor/
   ```

2. Copy the plugin and its metadata:
   ```bash
   cp build/bin/kf6/ktexteditor/warpkateplugin.so ~/.local/lib64/qt6/plugins/kf6/ktexteditor/
   cp src/warpkateplugin.json ~/.local/lib64/qt6/plugins/kf6/ktexteditor/
   ```

3. Make sure to update the plugin metadata to match your KDE version:
   ```json
   {
       "KTextEditor": {
           "Version": "6.0.0"
       },
       "X-Kate-PluginInterface-Version": "6.0"
   }
   ```

4. Set the QT_PLUGIN_PATH environment variable when running Kate:
   ```bash
   QT_PLUGIN_PATH=~/.local/lib64/qt6/plugins/:$QT_PLUGIN_PATH kate
   ```

## Troubleshooting

- **Plugin not detected**:
  - Ensure the metadata file (warpkateplugin.json) is in the same directory as the .so file
  - Check that the KTextEditor and X-Kate-PluginInterface versions match your Kate version
  - Try rebuilding the KDE services cache: `kbuildsycoca6 --noincremental`

- **Plugin loads but doesn't work**:
  - Run Kate with debug output: `QT_LOGGING_RULES="kf.texteditor.debug=true" kate`
  - Check if there are any error messages related to the plugin

- **Build errors**:
  - Ensure you have all the necessary development packages installed
  - Make sure Qt6 and KF6 development packages are installed

## Development Workflow

1. Make code changes
2. Rebuild with `make -j$(nproc)` in the build directory
3. Reinstall the plugin:
   ```bash
   sudo cmake --build . --target install
   ```
4. Rebuild KDE services cache:
   ```bash
   kbuildsycoca6 --noincremental
   ```
5. Restart Kate to load the updated plugin

## Key Files

- `src/warpkateplugin.cpp` and `src/warpkateplugin.h`: Main plugin class
- `src/warpkateview.cpp` and `src/warpkateview.h`: Plugin UI and view handling
- `src/terminalemulator.cpp` and `src/terminalemulator.h`: Terminal emulation code
- `src/blockmodel.cpp` and `src/blockmodel.h`: Block model for command history
- `src/terminalblockview.cpp` and `src/terminalblockview.h`: Block view UI
- `src/warpkateplugin.json`: Plugin metadata


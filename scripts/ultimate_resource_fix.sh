#!/bin/bash

# Ultimate Resource Fix Script for WarpKate
# This script takes extreme measures to ensure resources are properly included
# Created: $(date)

# Set color codes for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
BOLD='\033[1m'
NC='\033[0m' # No Color

echo -e "${MAGENTA}${BOLD}====== WarpKate Ultimate Resource Fix ======${NC}"
echo -e "${BLUE}This script will take extreme measures to fix resource issues${NC}"

# Backup original files
BACKUP_DIR="resource_fix_backup_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$BACKUP_DIR"
echo -e "\n${YELLOW}${BOLD}STEP 1: Backing up original files to $BACKUP_DIR${NC}"
cp -v src/warpkate.qrc "$BACKUP_DIR/"
cp -rv src/icons "$BACKUP_DIR/"
cp -v src/warpkateplugin.cpp "$BACKUP_DIR/"

# Get the build directory path
if [ -d "build-test" ]; then
  BUILD_DIR="build-test"
elif [ -d "build" ]; then
  BUILD_DIR="build"
else
  echo -e "${YELLOW}No build directory found. Creating build-test directory...${NC}"
  mkdir -p build-test
  BUILD_DIR="build-test"
fi

# Source resource file location
QRC_FILE="src/warpkate.qrc"
if [ ! -f "$QRC_FILE" ]; then
  echo -e "${RED}ERROR: Resource file not found at $QRC_FILE${NC}"
  echo -e "${RED}Please run this script from the WarpKate project root directory.${NC}"
  exit 1
fi

echo -e "\n${YELLOW}${BOLD}STEP 2: Clearing Kate caches${NC}"
echo -e "${BLUE}Removing Kate's cache files...${NC}"
rm -rf ~/.cache/kate
rm -rf ~/.cache/katepart
rm -rf ~/.config/kate/sessions

echo -e "${BLUE}Backing up and removing Kate's configuration...${NC}"
if [ -f ~/.config/katerc ]; then
    cp ~/.config/katerc "$BACKUP_DIR/"
    # Just remove the [WarpKate] section instead of the whole file
    sed -i '/\[WarpKate\]/,/^\[/d' ~/.config/katerc
fi

echo -e "\n${YELLOW}${BOLD}STEP 3: Removing symlinks and copying actual icon files${NC}"
# Create an icons directory if it doesn't exist
mkdir -p src/icons

# Copy actual icon files to src/icons
echo -e "${BLUE}Copying icon files directly to src/icons...${NC}"
cp -v icons/aibutton.svg src/icons/
cp -v icons/robbie50.svg src/icons/ 
cp -v icons/org.kde.konsole.svg src/icons/

# Verify copies
echo -e "${BLUE}Verifying icon files:${NC}"
for icon in src/icons/*.svg; do
    if [ -f "$icon" ]; then
        echo -e "${GREEN}✓ $icon exists${NC}"
        ls -la "$icon"
    else
        echo -e "${RED}✗ $icon missing${NC}"
    fi
done

echo -e "\n${YELLOW}${BOLD}STEP 4: Simplifying the QRC file structure${NC}"
# Backup existing QRC file
echo -e "${BLUE}Original QRC file:${NC}"
cat "$QRC_FILE"

# Create a simplified QRC file
echo -e "${BLUE}Creating simplified QRC file...${NC}"
cat > "$QRC_FILE" << EOF
<!DOCTYPE RCC>
<RCC version="1.0">
    <!-- Use simple, explicit resource paths -->
    <qresource>
        <file>icons/aibutton.svg</file>
        <file>icons/robbie50.svg</file>
        <file>icons/org.kde.konsole.svg</file>
        <file>ui/warpkatewidget.ui</file>
        <file>ui/configwidget.ui</file>
    </qresource>
</RCC>
EOF

echo -e "${GREEN}New QRC file created:${NC}"
cat "$QRC_FILE"

echo -e "\n${YELLOW}${BOLD}STEP 5: Adding debugging code to track resource loading${NC}"
# Create a patch for warpkateplugin.cpp to add debugging code
DEBUG_PATCH_FILE="$BACKUP_DIR/debug_patch.txt"
cat > "$DEBUG_PATCH_FILE" << 'EOF'
// Add at top of file
#include <QFile>
#include <QDir>
#include <QStandardPaths>

// Add to WarpKatePlugin::WarpKatePlugin constructor
// Resource debugging
qDebug() << "WarpKate plugin constructor - Resource paths:";
qDebug() << "Current directory:" << QDir::currentPath();
qDebug() << "Application directory:" << QCoreApplication::applicationDirPath();

// Check if icons exist in resources
QStringList iconPaths = {
    ":/icons/aibutton.svg", 
    ":/icons/robbie50.svg",
    ":/icons/org.kde.konsole.svg",
    ":/aibutton.svg", 
    ":/robbie50.svg",
    ":/org.kde.konsole.svg",
    "/icons/aibutton.svg", 
    "/icons/robbie50.svg",
    "/icons/org.kde.konsole.svg"
};

qDebug() << "Checking icon existence:";
for (const QString &path : iconPaths) {
    bool exists = QFile::exists(path);
    qDebug() << "  " << path << ":" << (exists ? "EXISTS" : "NOT FOUND");
}

// List all resources
qDebug() << "All available resources:";
QDir resourceDir(":/");
QStringList resources = resourceDir.entryList(QDir::AllEntries);
for (const QString &resource : resources) {
    qDebug() << "  " << resource;
    
    // If it's a directory, list contents
    if (QFileInfo(":/"+resource).isDir()) {
        QDir subDir(":/"+resource);
        QStringList subResources = subDir.entryList(QDir::AllEntries);
        for (const QString &subResource : subResources) {
            qDebug() << "    " << subResource;
        }
    }
}
EOF

echo -e "${BLUE}Adding debug code to warpkateplugin.cpp...${NC}"
# Find a good place to inject debug code
PLUGIN_FILE="src/warpkateplugin.cpp"
if [ -f "$PLUGIN_FILE" ]; then
    TEMP_FILE=$(mktemp)
    cat "$PLUGIN_FILE" > "$TEMP_FILE"
    
    # Add the include statements
    sed -i '1i#include <QFile>' "$TEMP_FILE"
    sed -i '2i#include <QDir>' "$TEMP_FILE"
    sed -i '3i#include <QStandardPaths>' "$TEMP_FILE"
    
    # Find the constructor and add debugging after it
    CONSTRUCTOR_LINE=$(grep -n "WarpKatePlugin::WarpKatePlugin" "$TEMP_FILE" | head -1 | cut -d: -f1)
    if [ -n "$CONSTRUCTOR_LINE" ]; then
        OPEN_BRACE_LINE=$((CONSTRUCTOR_LINE + 1))
        DEBUG_CODE=$(cat << 'EOF'

    // Resource debugging
    qDebug() << "WarpKate plugin constructor - Resource paths:";
    qDebug() << "Current directory:" << QDir::currentPath();
    qDebug() << "Application directory:" << QCoreApplication::applicationDirPath();

    // Check if icons exist in resources
    QStringList iconPaths = {
        ":/icons/aibutton.svg", 
        ":/icons/robbie50.svg",
        ":/icons/org.kde.konsole.svg",
        ":/aibutton.svg", 
        ":/robbie50.svg",
        ":/org.kde.konsole.svg",
        "/icons/aibutton.svg", 
        "/icons/robbie50.svg",
        "/icons/org.kde.konsole.svg"
    };

    qDebug() << "Checking icon existence:";
    for (const QString &path : iconPaths) {
        bool exists = QFile::exists(path);
        qDebug() << "  " << path << ":" << (exists ? "EXISTS" : "NOT FOUND");
    }

    // List all resources
    qDebug() << "All available resources:";
    QDir resourceDir(":/");
    QStringList resources = resourceDir.entryList(QDir::AllEntries);
    for (const QString &resource : resources) {
        qDebug() << "  " << resource;
        
        // If it's a directory, list contents
        if (QFileInfo(":/"+resource).isDir()) {
            QDir subDir(":/"+resource);
            QStringList subResources = subDir.entryList(QDir::AllEntries);
            for (const QString &subResource : subResources) {
                qDebug() << "    " << subResource;
            }
        }
    }
EOF
)
        sed -i "${OPEN_BRACE_LINE}a\\${DEBUG_CODE}" "$TEMP_FILE"
        
        # Replace the original file
        mv "$TEMP_FILE" "$PLUGIN_FILE"
        echo -e "${GREEN}✓ Debug code added to $PLUGIN_FILE${NC}"
    else
        echo -e "${RED}✗ Could not find constructor in $PLUGIN_FILE${NC}"
        rm "$TEMP_FILE"
    fi
else
    echo -e "${RED}✗ Plugin file not found at $PLUGIN_FILE${NC}"
fi

echo -e "\n${YELLOW}${BOLD}STEP 6: Extreme cleaning of build directory${NC}"
echo -e "${BLUE}Completely removing all build artifacts...${NC}"
cd "$BUILD_DIR"
echo -e "${BLUE}Running make clean...${NC}"
make clean || true

echo -e "${BLUE}Removing ALL generated files...${NC}"
find . -name "moc_*" -delete
find . -name "qrc_*" -delete
find . -name "ui_*" -delete
find . -name "*.o" -delete
find . -name "*.so" -delete

# Remove CMake cache files
echo -e "${BLUE}Removing CMake cache files...${NC}"
rm -f CMakeCache.txt
rm -rf CMakeFiles
rm -rf */CMakeFiles
rm -f */Makefile
rm -f Makefile
rm -f cmake_install.cmake
rm -f */cmake_install.cmake

# Return to project root
cd ..

echo -e "\n${YELLOW}${BOLD}STEP 7: Reconfiguring and rebuilding with verbose output${NC}"
echo -e "${BLUE}Running CMake with debug flags...${NC}"
cd "$BUILD_DIR"
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-DDEBUG_RESOURCES" -DCMAKE_VERBOSE_MAKEFILE=ON
if [ $? -ne 0 ]; then
  echo -e "${RED}CMake configuration failed!${NC}"
  cd ..
  exit 1
fi

echo -e "${BLUE}Running build with VERBOSE=1...${NC}"
VERBOSE=1 make
if [ $? -ne 0 ]; then
  echo -e "${RED}Build failed!${NC}"
  cd ..
  exit 1
fi

echo -e "${GREEN}✓ Build completed successfully${NC}"

# Return to project root
cd ..

echo -e "\n${YELLOW}${BOLD}STEP 8: Installing the plugin to all possible locations${NC}"
PLUGIN_FILE="$BUILD_DIR/bin/kf6/ktexteditor/warpkateplugin.so"
if [ ! -f "$PLUGIN_FILE" ]; then
  echo -e "${RED}✗ Plugin file not found after build!${NC}"
  exit 1
fi

echo -e "${GREEN}✓ Plugin file exists:${NC} $PLUGIN_FILE"
echo -e "  Last modified: $(ls -l --time-style=long-iso "$PLUGIN_FILE" | awk '{print $6, $7}')"

# Install to various locations
USER_PLUGIN_DIR_1="$HOME/.local/lib/qt6/plugins/kf6/ktexteditor"
USER_PLUGIN_DIR_2="$HOME/.local/lib64/qt6/plugins/kf6/ktexteditor"
SYSTEM_PLUGIN_DIR="/usr/lib64/qt6/plugins/kf6/ktexteditor"

mkdir -p "$USER_PLUGIN_DIR_1"
mkdir -p "$USER_PLUGIN_DIR_2"

echo -e "${BLUE}Installing to user directories...${NC}"
cp -v "$PLUGIN_FILE" "$USER_PLUGIN_DIR_1/"
cp -v "$PLUGIN_FILE" "$USER_PLUGIN_DIR_2/"

echo -e "${BLUE}Installing to system directory (requires sudo)...${NC}"
sudo cp -v "$PLUGIN_FILE" "$SYSTEM_PLUGIN_DIR/"

echo -e "${BLUE}Installing to project kate/addons directory...${NC}"
if [ -d "./kate/addons" ]; then
    cp -v "$PLUGIN_FILE" "./kate/addons/"
    echo -e "${GREEN}✓ Plugin installed to project kate/addons directory${NC}"
else
    echo -e "${YELLOW}Project kate/addons directory not found, skipping this location${NC}"
fi

# Also copy JSON file
JSON_FILE="src/warpkateplugin.json"
if [ -f "$JSON_FILE" ]; then
    echo -e "${BLUE}Installing JSON file to plugin directories...${NC}"
    cp -v "$JSON_FILE" "$USER_PLUGIN_DIR_1/"
    cp -v "$JSON_FILE" "$USER_PLUGIN_DIR_2/"
    sudo cp -v "$JSON_FILE" "$SYSTEM_PLUGIN_DIR/"
    
    if [ -d "./kate/addons" ]; then
        cp -v "$JSON_FILE" "./kate/addons/"
    fi
fi

echo -e "\n${YELLOW}${BOLD}STEP 9: Creating debugging helper for Kate${NC}"
echo -e "${BLUE}Creating script to run Kate with debugging enabled...${NC}"
cat > run_kate_debug.sh << 'EOF'
#!/bin/bash
export QT_DEBUG_PLUGINS=1
export QT_LOGGING_RULES="*.debug=true"
kate --new-window > kate_debug.log 2>&1
echo "Debug log written to kate_debug.log"
EOF
chmod +x run_kate_debug.sh

echo -e "\n${MAGENTA}${BOLD}====== Ultimate Resource Fix Complete ======${NC}"
echo -e "${YELLOW}The WarpKate plugin has been rebuilt with maximum debugging and fixed resources.${NC}"
echo -e "${YELLOW}Next steps:${NC}"


#!/bin/bash

# Script to aggressively rebuild WarpKate resources and plugin
# Created: $(date)

# Set color codes for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m' # No Color

echo -e "${YELLOW}${BOLD}====== WarpKate Force Resource Rebuild ======${NC}"
echo -e "${BLUE}This script aggressively rebuilds resources to ensure icon/resource changes are detected${NC}"

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

echo -e "\n${YELLOW}${BOLD}STEP 1: Resource files verification${NC}"
echo -e "${BLUE}Checking resource file: $QRC_FILE${NC}"

# Display the current resource file content
echo -e "${BLUE}Current resource content:${NC}"
cat "$QRC_FILE"

# Check the icon files referenced in the QRC file
echo -e "\n${BLUE}Checking referenced icon files:${NC}"
ICON_PATHS=$(grep -o '<file>.*</file>' "$QRC_FILE" | sed 's/<file>\(.*\)<\/file>/\1/')

for ICON in $ICON_PATHS; do
  ICON_PATH="src/$ICON"
  if [ -f "$ICON_PATH" ]; then
    echo -e "${GREEN}✓ Icon file exists:${NC} $ICON_PATH"
    # If it's a symlink, show the target
    if [ -L "$ICON_PATH" ]; then
      TARGET=$(readlink -f "$ICON_PATH")
      echo -e "  → Symlink to: $TARGET"
      if [ ! -f "$TARGET" ]; then
        echo -e "${RED}  ✗ WARNING: Target file does not exist!${NC}"
      fi
    fi
    # Show file details
    ls -lah "$ICON_PATH"
  else
    echo -e "${RED}✗ Icon file NOT found:${NC} $ICON_PATH"
  fi
done

echo -e "\n${YELLOW}${BOLD}STEP 2: Updating resource timestamp${NC}"
echo -e "${BLUE}Touching resource file to update timestamp...${NC}"
touch "$QRC_FILE"
echo -e "${GREEN}✓ Resource file timestamp updated:${NC}"
ls -lah "$QRC_FILE"

echo -e "\n${YELLOW}${BOLD}STEP 3: Removing compiled resource objects${NC}"
# Find and remove any compiled resource objects
QRC_OBJECTS=$(find "$BUILD_DIR" -name "qrc_*.cpp.o" -o -name "qrc_*.cpp" 2>/dev/null)
if [ -z "$QRC_OBJECTS" ]; then
  echo -e "${BLUE}No existing resource objects found.${NC}"
else
  echo -e "${BLUE}Removing existing resource objects:${NC}"
  for OBJ in $QRC_OBJECTS; do
    echo -e "  Removing: $OBJ"
    rm -f "$OBJ"
  done
  echo -e "${GREEN}✓ Removed compiled resource objects${NC}"
fi

echo -e "\n${YELLOW}${BOLD}STEP 4: Cleaning build directory${NC}"
cd "$BUILD_DIR"
echo -e "${BLUE}Running make clean...${NC}"
make clean
if [ $? -ne 0 ]; then
  echo -e "${YELLOW}Make clean returned non-zero, but continuing anyway...${NC}"
fi

# Remove CMake generated files that might cache resource information
echo -e "${BLUE}Removing CMake resource cache files...${NC}"
find . -name "*.cmake" -o -name "CMakeCache.txt" | xargs grep -l "warpkate.qrc" | while read file; do
  echo "  Touching: $file"
  touch "$file"
done

echo -e "\n${YELLOW}${BOLD}STEP 5: Reconfiguring and rebuilding${NC}"
echo -e "${BLUE}Running CMake to reconfigure...${NC}"
cmake .. -DCMAKE_BUILD_TYPE=Debug
if [ $? -ne 0 ]; then
  echo -e "${RED}CMake configuration failed!${NC}"
  cd ..
  exit 1
fi

echo -e "${BLUE}Running build with VERBOSE=1 to show resource compilation...${NC}"
make VERBOSE=1 | grep -E "qrc|resource|warpkate"
BUILD_RESULT=$?

# Check if compilation was successful
if [ ${PIPESTATUS[0]} -ne 0 ]; then
  echo -e "${RED}Build failed!${NC}"
  cd ..
  exit 1
fi

echo -e "${GREEN}✓ Build completed successfully${NC}"

echo -e "\n${YELLOW}${BOLD}STEP 6: Verifying resource compilation${NC}"
# Check that resource file was actually compiled
COMPILED_QRC=$(find . -name "qrc_*.cpp.o" -o -name "qrc_*.cpp" 2>/dev/null)
if [ -z "$COMPILED_QRC" ]; then
  echo -e "${RED}✗ No compiled resource files found after build!${NC}"
  echo -e "${RED}This suggests resources were not properly included in the build.${NC}"
else
  echo -e "${GREEN}✓ Found compiled resource files:${NC}"
  for QRC in $COMPILED_QRC; do
    echo -e "  $QRC"
    echo -e "  Last modified: $(ls -l --time-style=long-iso "$QRC" | awk '{print $6, $7}')"
  done
fi

# Return to project root
cd ..

echo -e "\n${YELLOW}${BOLD}STEP 7: Examining plugin binary${NC}"
PLUGIN_FILE="$BUILD_DIR/bin/kf6/ktexteditor/warpkateplugin.so"
if [ ! -f "$PLUGIN_FILE" ]; then
  echo -e "${RED}✗ Plugin file not found after build!${NC}"
  exit 1
fi

echo -e "${GREEN}✓ Plugin file exists:${NC} $PLUGIN_FILE"
echo -e "  Last modified: $(ls -l --time-style=long-iso "$PLUGIN_FILE" | awk '{print $6, $7}')"

# Check for resource strings in the plugin binary
echo -e "${BLUE}Checking for resource references in plugin binary...${NC}"
ICON_REFS=$(strings "$PLUGIN_FILE" | grep -E "icons/|\.svg" | sort | uniq)
if [ -z "$ICON_REFS" ]; then
  echo -e "${RED}✗ No icon references found in the plugin binary!${NC}"
else
  echo -e "${GREEN}✓ Found icon references in plugin binary:${NC}"
  echo "$ICON_REFS"
fi

echo -e "\n${YELLOW}${BOLD}STEP 8: Installing the plugin${NC}"
# Install to user directories
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

echo -e "\n${GREEN}${BOLD}====== Resource Rebuild Complete ======${NC}"
echo -e "${YELLOW}The WarpKate plugin has been aggressively rebuilt with updated resources.${NC}"
echo -e "${YELLOW}If Kate is running, please restart it to load the updated plugin.${NC}"


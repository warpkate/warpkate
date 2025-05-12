#!/bin/bash

# Script to install the WarpKate plugin to appropriate KDE directories
# Created: $(date)

# Set color codes for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}====== WarpKate Plugin Installation ======${NC}"

# Get the build directory path
if [ -d "build-test" ]; then
  BUILD_DIR="build-test"
elif [ -d "build" ]; then
  BUILD_DIR="build"
else
  echo -e "${RED}ERROR: Could not find build directory.${NC}"
  echo -e "${RED}Please run this script from the WarpKate project root directory.${NC}"
  exit 1
fi

# Define source and destination paths
SOURCE_PLUGIN="$BUILD_DIR/bin/kf6/ktexteditor/warpkateplugin.so"
SYSTEM_PLUGIN_DIR="/usr/lib64/qt6/plugins/kf6/ktexteditor"
USER_PLUGIN_DIR_1="$HOME/.local/lib/qt6/plugins/kf6/ktexteditor"
USER_PLUGIN_DIR_2="$HOME/.local/lib64/qt6/plugins/kf6/ktexteditor"

# Check if source plugin exists
if [ ! -f "$SOURCE_PLUGIN" ]; then
    echo -e "${RED}ERROR: Source plugin not found at $SOURCE_PLUGIN${NC}"
    echo -e "${RED}Make sure you have built the plugin successfully${NC}"
    exit 1
fi

echo -e "Source plugin: $SOURCE_PLUGIN"
echo -e "Last modified: $(ls -l --time-style=long-iso $SOURCE_PLUGIN | awk '{print $6, $7}')"

# Make sure destination directories exist
mkdir -p "$USER_PLUGIN_DIR_1"
mkdir -p "$USER_PLUGIN_DIR_2"

# Install to user directories (doesn't require sudo)
echo -e "\n${YELLOW}Installing to user directories...${NC}"
cp -v "$SOURCE_PLUGIN" "$USER_PLUGIN_DIR_1/"
USER_INSTALL_1=$?
cp -v "$SOURCE_PLUGIN" "$USER_PLUGIN_DIR_2/"
USER_INSTALL_2=$?

# Install to system directory (requires sudo)
echo -e "\n${YELLOW}Installing to system directory...${NC}"
echo "This operation requires sudo permissions:"
sudo cp -v "$SOURCE_PLUGIN" "$SYSTEM_PLUGIN_DIR/"
SYSTEM_INSTALL=$?

echo -e "\n${YELLOW}Verifying installation...${NC}"

# Check system installation
if [ $SYSTEM_INSTALL -eq 0 ] && [ -f "$SYSTEM_PLUGIN_DIR/warpkateplugin.so" ]; then
    SYSTEM_TIME=$(sudo ls -l --time-style=long-iso "$SYSTEM_PLUGIN_DIR/warpkateplugin.so" | awk '{print $6, $7}')
    echo -e "${GREEN}✓ System plugin installed successfully${NC}"
    echo -e "   Location: $SYSTEM_PLUGIN_DIR/warpkateplugin.so"
    echo -e "   Last modified: $SYSTEM_TIME"
else
    echo -e "${RED}✗ System plugin installation failed${NC}"
fi

# Check user installations
if [ $USER_INSTALL_1 -eq 0 ] && [ -f "$USER_PLUGIN_DIR_1/warpkateplugin.so" ]; then
    USER_TIME_1=$(ls -l --time-style=long-iso "$USER_PLUGIN_DIR_1/warpkateplugin.so" | awk '{print $6, $7}')
    echo -e "${GREEN}✓ User plugin installed successfully to lib${NC}"
    echo -e "   Location: $USER_PLUGIN_DIR_1/warpkateplugin.so"
    echo -e "   Last modified: $USER_TIME_1"
else
    echo -e "${RED}✗ User plugin installation to lib failed${NC}"
fi

if [ $USER_INSTALL_2 -eq 0 ] && [ -f "$USER_PLUGIN_DIR_2/warpkateplugin.so" ]; then
    USER_TIME_2=$(ls -l --time-style=long-iso "$USER_PLUGIN_DIR_2/warpkateplugin.so" | awk '{print $6, $7}')
    echo -e "${GREEN}✓ User plugin installed successfully to lib64${NC}"
    echo -e "   Location: $USER_PLUGIN_DIR_2/warpkateplugin.so"
    echo -e "   Last modified: $USER_TIME_2"
else
    echo -e "${RED}✗ User plugin installation to lib64 failed${NC}"
fi

# Overall status
if [ $SYSTEM_INSTALL -eq 0 ] && [ $USER_INSTALL_1 -eq 0 ] && [ $USER_INSTALL_2 -eq 0 ]; then
    echo -e "\n${GREEN}====== INSTALLATION COMPLETE - OK ======${NC}"
    echo -e "${YELLOW}Note: You may need to restart Kate to load the updated plugin${NC}"
    exit 0
else
    echo -e "\n${RED}====== INSTALLATION INCOMPLETE - CHECK ERRORS ABOVE ======${NC}"
    exit 1
fi

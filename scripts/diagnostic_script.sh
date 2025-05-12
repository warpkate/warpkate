#!/bin/bash

# Script to check the status of WarpKate plugin installation
# Created: $(date)

# Set color codes for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${YELLOW}====== WarpKate Plugin Diagnostic ======${NC}"

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

# Check plugin files in various locations
SYSTEM_PLUGIN="/usr/lib64/qt6/plugins/kf6/ktexteditor/warpkateplugin.so"
USER_PLUGIN_1="$HOME/.local/lib/qt6/plugins/kf6/ktexteditor/warpkateplugin.so"
USER_PLUGIN_2="$HOME/.local/lib64/qt6/plugins/kf6/ktexteditor/warpkateplugin.so"
BUILD_PLUGIN="$BUILD_DIR/bin/kf6/ktexteditor/warpkateplugin.so"

echo -e "\n${YELLOW}Checking plugin files:${NC}"

# Function to check and display file information
check_file() {
    local file=$1
    local label=$2
    local use_sudo=$3
    
    if [ "$use_sudo" = "sudo" ]; then
        if sudo test -f "$file"; then
            local mod_time=$(sudo ls -l --time-style=long-iso "$file" | awk '{print $6, $7}')
            local file_size=$(sudo ls -lh "$file" | awk '{print $5}')
            echo -e "${GREEN}✓ $label exists${NC}"
            echo -e "   Location: $file"
            echo -e "   Size: $file_size"
            echo -e "   Last modified: $mod_time"
            return 0
        else
            echo -e "${RED}✗ $label not found${NC}"
            echo -e "   Expected at: $file"
            return 1
        fi
    else
        if [ -f "$file" ]; then
            local mod_time=$(ls -l --time-style=long-iso "$file" | awk '{print $6, $7}')
            local file_size=$(ls -lh "$file" | awk '{print $5}')
            echo -e "${GREEN}✓ $label exists${NC}"
            echo -e "   Location: $file"
            echo -e "   Size: $file_size"
            echo -e "   Last modified: $mod_time"
            return 0
        else
            echo -e "${RED}✗ $label not found${NC}"
            echo -e "   Expected at: $file"
            return 1
        fi
    fi
}

# Check each location
check_file "$BUILD_PLUGIN" "Build directory plugin"
BUILD_EXISTS=$?

check_file "$SYSTEM_PLUGIN" "System plugin" "sudo"
check_file "$USER_PLUGIN_1" "User lib plugin"
check_file "$USER_PLUGIN_2" "User lib64 plugin"

# Check if Kate is running
echo -e "\n${YELLOW}Checking if Kate is running:${NC}"
KATE_RUNNING=$(pgrep -l kate)
if [ -n "$KATE_RUNNING" ]; then
    echo -e "${BLUE}Kate is currently running:${NC}"
    echo "$KATE_RUNNING"
    echo -e "${YELLOW}Note: You may need to restart Kate to load the updated plugin${NC}"
else
    echo -e "${BLUE}Kate is not currently running${NC}"
fi

# Check Kate plugin configuration
KATE_CONFIG="$HOME/.config/kate/kateconfig"
KATE_PLUGIN_RC="$HOME/.config/katerc"

echo -e "\n${YELLOW}Checking Kate configuration:${NC}"
if [ -f "$KATE_CONFIG" ]; then
    echo -e "${BLUE}Kate config file exists:${NC} $KATE_CONFIG"
    grep -i "warpkate" "$KATE_CONFIG" || echo "No WarpKate references found in config file"
else
    echo -e "${BLUE}Kate config file not found at:${NC} $KATE_CONFIG"
fi

if [ -f "$KATE_PLUGIN_RC" ]; then
    echo -e "${BLUE}Kate plugin config exists:${NC} $KATE_PLUGIN_RC"
    grep -i "warpkate" "$KATE_PLUGIN_RC" || echo "No WarpKate references found in plugin config"
else
    echo -e "${BLUE}Kate plugin config not found at:${NC} $KATE_PLUGIN_RC"
fi

# Resource file check
echo -e "\n${YELLOW}Checking resource file:${NC}"
QRC_FILE="src/warpkate.qrc"
if [ -f "$QRC_FILE" ]; then
    echo -e "${BLUE}Resource file exists:${NC} $QRC_FILE"
    echo -e "${BLUE}Resource contents:${NC}"
    cat "$QRC_FILE"
else
    echo -e "${RED}Resource file not found at:${NC} $QRC_FILE"
fi

# Check for plugin JSON file
echo -e "\n${YELLOW}Checking plugin JSON:${NC}"
JSON_FILE="src/warpkateplugin.json"
if [ -f "$JSON_FILE" ]; then
    echo -e "${BLUE}Plugin JSON exists:${NC} $JSON_FILE"
    echo -e "${BLUE}JSON contents:${NC}"
    cat "$JSON_FILE"
else
    echo -e "${RED}Plugin JSON not found at:${NC} $JSON_FILE"
fi

# Check build system status
echo -e "\n${YELLOW}Checking build system status:${NC}"
if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo -e "${BLUE}CMake build directory exists:${NC} $BUILD_DIR"
    
    # Check QRC files in build
    QRC_BUILD="$BUILD_DIR/src/CMakeFiles/warpkateplugin.dir/qrc_warpkate.cpp.o"
    if [ -f "$QRC_BUILD" ]; then
        echo -e "${BLUE}Compiled QRC file exists:${NC} $QRC_BUILD"
        echo -e "   Last modified: $(ls -l --time-style=long-iso $QRC_BUILD | awk '{print $6, $7}')"
    else
        echo -e "${RED}Compiled QRC file not found at:${NC} $QRC_BUILD"
    fi
else
    echo -e "${RED}CMake build directory not found at:${NC} $BUILD_DIR"
fi

echo -e "\n${YELLOW}====== Diagnostic Complete ======${NC}"
if [ $BUILD_EXISTS -eq 0 ]; then
    echo -e "${GREEN}The plugin build exists. You can install it using:${NC}"
    echo -e "${BLUE}./src/install_warpkate_plugin.sh${NC}"
else
    echo -e "${RED}The plugin build was not found. Please build the plugin first.${NC}"
fi

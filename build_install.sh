#!/bin/bash
# Script to build and install WarpKate

set -e  # Exit on error

# Set color codes for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${YELLOW}=== Building and Installing WarpKate ===${NC}"
echo

# Configuration - use relative paths
PROJECT_ROOT=$(pwd)
BUILD_DIR="$PROJECT_ROOT/build"

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${BLUE}Creating build directory...${NC}"
    mkdir -p "$BUILD_DIR"
fi

# Move to build directory
cd "$BUILD_DIR"

# Run CMake if needed
if [ ! -f "$BUILD_DIR/Makefile" ]; then
    echo -e "${BLUE}Running CMake...${NC}"
    cmake .. -DCMAKE_INSTALL_PREFIX=`kf6-config --prefix` -DCMAKE_BUILD_TYPE=Release
fi

# Build the project
echo -e "${BLUE}Building the project...${NC}"
make -j$(nproc)

# Verify plugin was built
if [ ! -f "$BUILD_DIR/bin/kf6/ktexteditor/warpkateplugin.so" ]; then
    echo -e "${RED}Build failed! Plugin file not created.${NC}"
    exit 1
fi

# Install
echo -e "${YELLOW}Installing WarpKate (requires sudo)...${NC}"
sudo make install

echo
echo -e "${GREEN}Build and installation completed.${NC}"
echo -e "${YELLOW}You may need to restart Kate to use the updated plugin.${NC}"

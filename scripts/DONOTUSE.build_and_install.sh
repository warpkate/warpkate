#!/bin/bash

# Script to build and install the WarpKate plugin
# Created: $(date)

# Set color codes for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${YELLOW}====== WarpKate Build and Install ======${NC}"

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

# Step 1: Build the plugin
echo -e "\n${YELLOW}Building WarpKate plugin...${NC}"
cd $BUILD_DIR

# Check if we need to configure
if [ ! -f "CMakeCache.txt" ]; then
  echo -e "${BLUE}Configuring build with CMake...${NC}"
  cmake .. -DCMAKE_BUILD_TYPE=Debug
  if [ $? -ne 0 ]; then
    echo -e "${RED}CMake configuration failed!${NC}"
    exit 1
  fi
fi

# Build the plugin
echo -e "${BLUE}Building plugin...${NC}"
make
if [ $? -ne 0 ]; then
  echo -e "${RED}Build failed!${NC}"
  exit 1
fi

# Return to project root
cd ..

# Verify the plugin was built
if [ ! -f "$BUILD_DIR/bin/kf6/ktexteditor/warpkateplugin.so" ]; then
  echo -e "${RED}Plugin was not built successfully.${NC}"
  exit 1
fi

echo -e "${GREEN}Plugin built successfully.${NC}"

# Step 2: Install the plugin using the installation script
echo -e "\n${YELLOW}Running installation script...${NC}"
./scripts/install_warpkate_plugin.sh

# Step 3: Run the diagnostic script
echo -e "\n${YELLOW}Running diagnostic check...${NC}"
./scripts/diagnostic_script.sh

echo -e "\n${GREEN}====== Build and Install Process Complete ======${NC}"
echo -e "${YELLOW}If Kate is running, you should restart it to load the updated plugin.${NC}"

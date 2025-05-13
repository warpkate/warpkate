#!/bin/bash
# WarpKate v0.15 Release Preparation Script
# This script prepares the codebase for release by:
# 1. Backing up the codebase
# 2. Creating a GitHub release commit
# 3. Building the plugin
# 4. Copying the compiled binary to a release directory
# 5. Cleaning up temporary files

set -e  # Exit on error

# Configuration
VERSION="0.15"
RELEASE_DIR="release_v${VERSION}"
BACKUP_DIR="backup_$(date +%Y%m%d_%H%M%S)"
SRC_DIR="src"
BUILD_DIR="build"
PLUGIN_NAME="warpkateplugin.so"
PLUGIN_PATH="${BUILD_DIR}/src/${PLUGIN_NAME}"

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Header
echo -e "${BLUE}===== WarpKate v${VERSION} Release Preparation Script =====${NC}"
echo "This script will prepare WarpKate v${VERSION} for release."
echo ""

# Function to check if command exists
check_command() {
    if ! command -v $1 &> /dev/null; then
        echo -e "${RED}Error: Required command '$1' is not installed.${NC}"
        exit 1
    fi
}

# Check for required commands
check_command "git"
check_command "cmake"
check_command "make"

# Step 1: Backup the codebase
echo -e "${YELLOW}Step 1: Creating backup of the codebase...${NC}"
mkdir -p ${BACKUP_DIR}
rsync -av --exclude="${BACKUP_DIR}" --exclude="${RELEASE_DIR}" --exclude="build" --exclude=".git" . ${BACKUP_DIR}/
echo -e "${GREEN}Backup created at $(pwd)/${BACKUP_DIR}${NC}"
echo ""

# Step 2: Create GitHub release commit
echo -e "${YELLOW}Step 2: Creating GitHub release commit...${NC}"

# Check Git status
if [ -n "$(git status --porcelain)" ]; then
    echo "There are uncommitted changes. Would you like to continue? (y/n)"
    read answer
    if [ "$answer" != "y" ]; then
        echo "Aborting."
        exit 1
    fi
fi

# Add fix for QMessageBox include (missing from the build errors)
echo "Fixing QMessageBox include error..."
if ! grep -q "#include <QMessageBox>" ${SRC_DIR}/warpkateview.cpp; then
    sed -i '/#include <QTimer>/a #include <QMessageBox>' ${SRC_DIR}/warpkateview.cpp
    echo -e "${GREEN}Added missing QMessageBox include${NC}"
fi

# Fix QTextEdit::setOpenExternalLinks error - replace with QTextBrowser
echo "Fixing QTextBrowser conversion error..."
if grep -q "m_conversationArea->setOpenExternalLinks" ${SRC_DIR}/warpkateview.cpp; then
    sed -i 's/m_conversationArea = new QTextEdit/m_conversationArea = new QTextBrowser/g' ${SRC_DIR}/warpkateview.cpp
    sed -i 's/QPointer<QTextEdit> m_conversationArea/QPointer<QTextBrowser> m_conversationArea/g' ${SRC_DIR}/warpkateview.h
    echo -e "${GREEN}Converted QTextEdit to QTextBrowser${NC}"
fi

# Fix deprecated setFontFamily warnings by using setFontFamilies
echo "Fixing deprecated setFontFamily warnings..."
if grep -q "setFontFamily" ${SRC_DIR}/warpkateview.cpp; then
    sed -i 's/setFontFamily(QStringLiteral("Monospace"))/setFontFamilies(QStringList() << QStringLiteral("Monospace"))/g' ${SRC_DIR}/warpkateview.cpp
    echo -e "${GREEN}Updated deprecated setFontFamily calls to setFontFamilies${NC}"
fi

# Make sure all necessary includes are present
echo "Adding any other missing includes..."
for include in "QMessageBox" "QTextBrowser" "QSplitter" "QMenu" "QClipboard" "QDesktopServices" "QUrl"; do
    if ! grep -q "#include <${include}>" ${SRC_DIR}/warpkateview.cpp; then
        sed -i "/#include <QDebug>/a #include <${include}>" ${SRC_DIR}/warpkateview.cpp
        echo -e "${GREEN}Added missing ${include} include${NC}"
    fi
done

# Update version number in CMakeLists.txt
echo "Updating version number in CMakeLists.txt..."
sed -i "s/set(WARPKATE_VERSION .*/set(WARPKATE_VERSION \"${VERSION}\")/g" CMakeLists.txt
echo -e "${GREEN}Updated version number to ${VERSION}${NC}"

# Commit changes
echo "Committing changes for v${VERSION}..."
git add .
git commit -m "Prepare for WarpKate v${VERSION} release
* Add clickable file paths in terminal output
* Fix mode switching behavior with '>' character
* Fix build warnings and errors"

# Create tag for release
git tag -a "v${VERSION}" -m "WarpKate v${VERSION}"
echo -e "${GREEN}Changes committed and tagged as v${VERSION}${NC}"
echo ""

# Step 3: Build the plugin
echo -e "${YELLOW}Step 3: Building the plugin...${NC}"
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}
cmake ..
make -j$(nproc)

# Check if build was successful
if [ ! -f "src/${PLUGIN_NAME}" ]; then
    echo -e "${RED}Error: Build failed. Plugin not found at ${PLUGIN_PATH}${NC}"
    exit 1
fi
cd ..
echo -e "${GREEN}Build successful.${NC}"
echo ""

# Step 4: Copy compiled binary to release directory
echo -e "${YELLOW}Step 4: Copying compiled binary to release directory...${NC}"
mkdir -p ${RELEASE_DIR}
cp ${PLUGIN_PATH} ${RELEASE_DIR}/
cp RELEASE_NOTES_v${VERSION}.md ${RELEASE_DIR}/
cp README.md ${RELEASE_DIR}/
cp LICENSE ${RELEASE_DIR}/
cp -r icons ${RELEASE_DIR}/

# Create a simple install script in the release directory
cat > ${RELEASE_DIR}/install.sh << EOF
#!/bin/bash
# WarpKate v${VERSION} Installation Script

if [ \$(id -u) -ne 0 ]; then
    echo "This script needs to be run as root to install the plugin to system directories."
    echo "Please run with sudo or as root."
    exit 1
fi

# Detect system plugin directory
if command -v kf6-config &> /dev/null; then
    PLUGIN_DIR=\$(kf6-config --path qtplugins | cut -d ':' -f 1)
elif command -v kf5-config &> /dev/null; then
    PLUGIN_DIR=\$(kf5-config --path qtplugins | cut -d ':' -f 1)
else
    PLUGIN_DIR="/usr/lib64/qt6/plugins"
fi

# Create directories if they don't exist
mkdir -p "\${PLUGIN_DIR}/ktexteditor"

# Copy plugin
cp warpkateplugin.so "\${PLUGIN_DIR}/ktexteditor/"

# Copy icons
mkdir -p "/usr/share/icons/hicolor/scalable/apps"
cp -r icons/* "/usr/share/icons/hicolor/scalable/apps/"

echo "WarpKate v${VERSION} has been installed successfully."
echo "Please restart Kate for the plugin to take effect."
EOF

chmod +x ${RELEASE_DIR}/install.sh

# Create source tarball
tar -czf ${RELEASE_DIR}/warpkate-${VERSION}-src.tar.gz \
    --exclude=${BACKUP_DIR} \
    --exclude=${RELEASE_DIR} \
    --exclude=${BUILD_DIR} \
    --exclude=.git \
    --transform="s,^,warpkate-${VERSION}/," \
    .

echo -e "${GREEN}Release files prepared in the ${RELEASE_DIR} directory.${NC}"
echo ""

# Step 5: Clean up
echo -e "${YELLOW}Step 5: Cleaning up temporary files...${NC}"
find . -name "*.o" -delete
find . -name "moc_*.cpp" -delete
find . -name "*.a" -delete
find ${BUILD_DIR} -name "CMakeCache.txt" -delete
echo -e "${GREEN}Cleanup completed.${NC}"
echo ""

echo -e "${BLUE}===== Release preparation completed =====${NC}"
echo "To push the release to GitHub, run:"
echo "  git push origin master"
echo "  git push origin v${VERSION}"
echo ""
echo "To install the plugin, run:"
echo "  cd ${RELEASE_DIR} && sudo ./install.sh"
echo ""
echo "Release files are available in the ${RELEASE_DIR} directory."


#!/bin/bash
# Script to fix the anchorClicked connection issue in warpkateview.cpp

set -e  # Exit on error

CPP_FILE="/home/yani/Projects/WarpKate/src/warpkateview.cpp"

echo "===== WarpKate anchorClicked Fixer ====="
echo "This script will fix the remaining anchorClicked issue in warpkateview.cpp."
echo

# Backup the file
echo "Creating backup of warpkateview.cpp..."
BACKUP_FILE="$CPP_FILE.bak.$(date +%Y%m%d_%H%M%S)"
cp "$CPP_FILE" "$BACKUP_FILE"
echo "Backup created at $BACKUP_FILE"
echo

# Fix the QTextEdit::anchorClicked reference
echo "Fixing anchorClicked connection..."
FIXED=$(grep -l "&QTextEdit::anchorClicked" "$CPP_FILE")

if [ -n "$FIXED" ]; then
    # Replace QTextEdit with QTextBrowser in the connection
    sed -i 's/&QTextEdit::anchorClicked/&QTextBrowser::anchorClicked/g' "$CPP_FILE"
    echo "Successfully fixed anchorClicked connection in warpkateview.cpp"
else
    echo "No anchorClicked issue found in warpkateview.cpp"
fi

echo
echo "Now rebuild the project with:"
echo "  cd /home/yani/Projects/WarpKate/build"
echo "  cmake .."
echo "  make -j$(nproc)"
echo "  sudo make install"


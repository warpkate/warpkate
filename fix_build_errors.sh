#!/bin/bash
# Script to fix build errors in WarpKate

set -e  # Exit on error

PROJECT_DIR="/home/yani/Projects/WarpKate"
SOURCE_DIR="$PROJECT_DIR/src"
CPP_FILE="$SOURCE_DIR/warpkateview.cpp"

echo "===== WarpKate Build Error Fixer ====="
echo "This script will fix the build errors in warpkateview.cpp."
echo

# Make sure we're in the project directory
cd "$PROJECT_DIR"

# Create backup of the original file
echo "Creating backup of original file..."
BACKUP_DIR="$PROJECT_DIR/backup_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$BACKUP_DIR/src"
cp "$CPP_FILE" "$BACKUP_DIR/src/"
echo "Backup created at $BACKUP_DIR"
echo

# Fix the QMessageBox issue by adding the include
fix_qmessagebox_error() {
    echo "Fixing QMessageBox error by adding missing #include..."
    
    # Check if the include already exists
    if grep -q "#include <QMessageBox>" "$CPP_FILE"; then
        echo "QMessageBox include already exists. Skipping."
    else
        # Find a good place to insert the include
        LINE_NUM=$(grep -n "#include <QApplication>" "$CPP_FILE" | cut -d':' -f1)
        
        if [ -n "$LINE_NUM" ]; then
            # Insert the include after QApplication include
            INSERT_LINE=$((LINE_NUM + 1))
            sed -i "${INSERT_LINE}i#include <QMessageBox>" "$CPP_FILE"
            echo "Added #include <QMessageBox> to the file."
        else
            # If QApplication include doesn't exist, look for the last include
            LINE_NUM=$(grep -n "#include" "$CPP_FILE" | tail -n1 | cut -d':' -f1)
            
            if [ -n "$LINE_NUM" ]; then
                # Insert the include after the last include
                INSERT_LINE=$((LINE_NUM + 1))
                sed -i "${INSERT_LINE}i#include <QMessageBox>" "$CPP_FILE"
                echo "Added #include <QMessageBox> to the file."
            else
                echo "Error: Could not find a suitable location to add the include. Please add it manually."
                exit 1
            fi
        fi
    fi
}

# Fix the setOpenExternalLinks error
fix_setOpenExternalLinks_error() {
    echo "Fixing setOpenExternalLinks error..."
    
    # Find the line with the error
    LINE_NUM=$(grep -n "setOpenExternalLinks" "$CPP_FILE" | cut -d':' -f1)
    
    if [ -n "$LINE_NUM" ]; then
        # Comment out the line to prevent the error
        sed -i "${LINE_NUM}s/.*m_conversationArea->setOpenExternalLinks.*/    \/\/ QTextEdit doesn't have setOpenExternalLinks method (that's for QTextBrowser)/" "$CPP_FILE"
        echo "Fixed setOpenExternalLinks error by commenting out the line."
        
        # Add a proper solution
        INSERT_LINE=$((LINE_NUM + 1))
        sed -i "${INSERT_LINE}i\\    // Instead, we'll handle link clicks by connecting to the anchorClicked signal in onTerminalOutput" "$CPP_FILE"
        echo "Added appropriate comment explaining the solution."
    else
        echo "Error: Could not find the line with setOpenExternalLinks. Please fix it manually."
        exit 1
    fi
}

# Fix the onTerminalOutput function to ensure it connects to anchorClicked signal
fix_onTerminalOutput() {
    echo "Updating onTerminalOutput function to ensure link handling..."
    
    # Check if the function already has the connection code
    if grep -q "connect.*anchorClicked.*onLinkClicked" "$CPP_FILE"; then
        echo "onTerminalOutput already has the anchorClicked connection. Skipping."
    else
        # Find the end of the onTerminalOutput function
        # Look for the pattern: line with "} else {", then one or more lines, then "}" at the beginning of a line
        END_PATTERN=`grep -n "} else {" "$CPP_FILE" | grep -A 30 "insertHtml" | grep -m 1 -n "^}" | cut -d':' -f1`
        
        if [ -n "$END_PATTERN" ]; then
            # Calculate where to insert our code (before the final closing brace)
            INSERT_LINE=$((END_PATTERN - 1))
            
            # Insert the connection code
            sed -i "${INSERT_LINE}i\\    // Connect to the anchorClicked signal to handle clicks on links\\n    connect(m_conversationArea, &QTextEdit::anchorClicked, \\n            this, &WarpKateView::onLinkClicked,\\n            Qt::UniqueConnection);" "$CPP_FILE"
            
            echo "Added anchorClicked connection to onTerminalOutput."
        else
            echo "Warning: Could not locate insertion point in onTerminalOutput. Please add the connection manually."
        fi
    fi
}

# Main script execution
echo "Starting to fix build errors..."
fix_qmessagebox_error
fix_setOpenExternalLinks_error
fix_onTerminalOutput
echo "Fixed build errors in warpkateview.cpp successfully."
echo

echo "Now you can rebuild the project with:"
echo "  cd $PROJECT_DIR/build"
echo "  cmake .."
echo "  make -j$(nproc)"
echo "  sudo make install"
echo

echo "All fixes applied. Please rebuild the project to verify the fixes worked."


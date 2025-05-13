#!/bin/bash
# Script to replace QTextEdit with QTextBrowser for link handling in WarpKate

set -e  # Exit on error

PROJECT_DIR="/home/yani/Projects/WarpKate"
SOURCE_DIR="$PROJECT_DIR/src"
H_FILE="$SOURCE_DIR/warpkateview.h"
CPP_FILE="$SOURCE_DIR/warpkateview.cpp"

echo "===== WarpKate Link Handling Fixer ====="
echo "This script will replace QTextEdit with QTextBrowser for the conversation area."
echo

# Make sure we're in the project directory
cd "$PROJECT_DIR"

# Create backup of the original files
echo "Creating backup of original files..."
BACKUP_DIR="$PROJECT_DIR/backup_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$BACKUP_DIR/src"
cp "$H_FILE" "$BACKUP_DIR/src/"
cp "$CPP_FILE" "$BACKUP_DIR/src/"
echo "Backup created at $BACKUP_DIR"
echo

# Update the header file to use QTextBrowser
update_header_file() {
    echo "Updating header file to use QTextBrowser instead of QTextEdit..."
    
    # Check if header already includes QTextBrowser
    if grep -q "#include <QTextBrowser>" "$H_FILE"; then
        echo "QTextBrowser already included in header. Skipping include."
    else
        # Add the include for QTextBrowser
        LINE_NUM=$(grep -n "#include <QTextEdit>" "$H_FILE" | cut -d':' -f1)
        if [ -n "$LINE_NUM" ]; then
            sed -i "${LINE_NUM}s/#include <QTextEdit>/#include <QTextEdit>\n#include <QTextBrowser>/" "$H_FILE"
            echo "Added #include <QTextBrowser> to header file."
        else
            echo "Warning: Could not find #include <QTextEdit> in header file."
            # Try to add after other includes
            LINE_NUM=$(grep -n "#include" "$H_FILE" | tail -n1 | cut -d':' -f1)
            if [ -n "$LINE_NUM" ]; then
                sed -i "${LINE_NUM}a#include <QTextBrowser>" "$H_FILE"
                echo "Added #include <QTextBrowser> after last include in header file."
            else
                echo "Error: Could not add #include <QTextBrowser> to header file."
                exit 1
            fi
        fi
    fi
    
    # Update the class member declaration
    LINE_NUM=$(grep -n "QTextEdit \*m_conversationArea;" "$H_FILE" | cut -d':' -f1)
    if [ -n "$LINE_NUM" ]; then
        sed -i "${LINE_NUM}s/QTextEdit \*m_conversationArea;/QTextBrowser \*m_conversationArea;/" "$H_FILE"
        echo "Updated m_conversationArea declaration to QTextBrowser in header file."
    else
        echo "Error: Could not update m_conversationArea declaration in header file."
        exit 1
    fi
}

# Update the cpp file to use QTextBrowser
update_cpp_file() {
    echo "Updating cpp file to use QTextBrowser instead of QTextEdit..."
    
    # Update the creation of conversation area
    LINE_NUM=$(grep -n "m_conversationArea = new QTextEdit" "$CPP_FILE" | cut -d':' -f1)
    if [ -n "$LINE_NUM" ]; then
        sed -i "${LINE_NUM}s/m_conversationArea = new QTextEdit/m_conversationArea = new QTextBrowser/" "$CPP_FILE"
        echo "Updated m_conversationArea initialization to QTextBrowser in cpp file."
    else
        echo "Error: Could not update m_conversationArea initialization in cpp file."
        exit 1
    fi
    
    # Fix the setOpenExternalLinks line
    LINE_NUM=$(grep -n "setOpenExternalLinks" "$CPP_FILE" | cut -d':' -f1)
    if [ -n "$LINE_NUM" ]; then
        # Uncomment the line if it was commented out
        sed -i "${LINE_NUM}s/\/\/ QTextEdit doesn't have setOpenExternalLinks/    m_conversationArea->setOpenExternalLinks(false);  \/\/ Now using QTextBrowser which supports this/" "$CPP_FILE"
        echo "Fixed setOpenExternalLinks call in cpp file."
    else
        # Add the line if it doesn't exist
        LINE_NUM=$(grep -n "m_conversationArea->setAcceptRichText" "$CPP_FILE" | cut -d':' -f1)
        if [ -n "$LINE_NUM" ]; then
            INSERT_LINE=$((LINE_NUM + 1))
            sed -i "${INSERT_LINE}i\\    m_conversationArea->setOpenExternalLinks(false);  // Using QTextBrowser which supports this" "$CPP_FILE"
            echo "Added setOpenExternalLinks call in cpp file."
        else
            echo "Warning: Could not add setOpenExternalLinks call in cpp file."
        fi
    fi
    
    # Fix the connection in onTerminalOutput
    LINE_NUM=$(grep -n "connect.*anchorClicked.*onLinkClicked" "$CPP_FILE" | cut -d':' -f1)
    if [ -n "$LINE_NUM" ]; then
        # Update the connection to use QTextBrowser
        sed -i "${LINE_NUM}s/&QTextEdit::anchorClicked/&QTextBrowser::anchorClicked/" "$CPP_FILE"
        echo "Updated anchorClicked connection to use QTextBrowser in cpp file."
    else
        echo "Warning: Could not find anchorClicked connection in cpp file."
        
        # Try to find the onTerminalOutput function and add the connection
        START_LINE=$(grep -n "void WarpKateView::onTerminalOutput" "$CPP_FILE" | cut -d':' -f1)
        if [ -n "$START_LINE" ]; then
            # Find the end of the function (closing brace)
            END_LINE=$(tail -n +$START_LINE "$CPP_FILE" | grep -n "^}" | head -1 | cut -d':' -f1)
            END_LINE=$((START_LINE + END_LINE - 1))
            
            # Add the connection just before the closing brace
            INSERT_LINE=$((END_LINE - 1))
            sed -i "${INSERT_LINE}i\\    // Connect to the anchorClicked signal to handle clicks on links\\n    connect(m_conversationArea, &QTextBrowser::anchorClicked, \\n            this, &WarpKateView::onLinkClicked,\\n            Qt::UniqueConnection);\\n\\n    // Ensure cursor is visible\\n    m_conversationArea->ensureCursorVisible();" "$CPP_FILE"
            echo "Added anchorClicked connection to onTerminalOutput function."
        else
            echo "Error: Could not find onTerminalOutput function in cpp file."
            exit 1
        fi
    fi
}

# Main script execution
echo "Starting to fix link handling..."
update_header_file
update_cpp_file
echo "Link handling fixes completed successfully."
echo

echo "Now you can rebuild the project with:"
echo "  cd $PROJECT_DIR/build"
echo "  cmake .."
echo "  make -j$(nproc)"
echo "  sudo make install"
echo

echo "All fixes applied. Please rebuild the project to verify the fixes worked."


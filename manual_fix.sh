#!/bin/bash
# Script to manually add the link handling code to onTerminalOutput

set -e  # Exit on error

CPP_FILE="/home/yani/Projects/WarpKate/src/warpkateview.cpp"

echo "Adding link handling code to onTerminalOutput function..."

# Find the end of the onTerminalOutput function
LINE_NUM=$(grep -n "void WarpKateView::onTerminalOutput" "$CPP_FILE" | cut -d':' -f1)

# Find the end of the function by looking for the closing brace
if [ -n "$LINE_NUM" ]; then
    # Start from the function declaration line and find the closing brace
    START_LINE=$LINE_NUM
    END_LINE=$(tail -n +$START_LINE "$CPP_FILE" | grep -n "^}" | head -1 | cut -d':' -f1)
    
    if [ -n "$END_LINE" ]; then
        # Calculate the actual line number in the file
        END_LINE=$((START_LINE + END_LINE - 1))
        
        # Insert the link handling code before the closing brace
        sed -i "${END_LINE}i\\    // Connect to the anchorClicked signal to handle clicks on links\\n    connect(m_conversationArea, &QTextEdit::anchorClicked, \\n            this, &WarpKateView::onLinkClicked,\\n            Qt::UniqueConnection);\\n\\n    // Ensure cursor is visible\\n    m_conversationArea->ensureCursorVisible();" "$CPP_FILE"
        
        echo "Successfully added link handling code to onTerminalOutput function."
    else
        echo "Could not find the end of the onTerminalOutput function."
        exit 1
    fi
else
    echo "Could not find the onTerminalOutput function."
    exit 1
fi

echo
echo "Now rebuild the project with:"
echo "  cd /home/yani/Projects/WarpKate/build"
echo "  cmake .."
echo "  make -j$(nproc)"
echo "  sudo make install"


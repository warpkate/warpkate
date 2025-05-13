#!/bin/bash
# Final script to fix the syntax error in the connection

CPP_FILE="/home/yani/Projects/WarpKate/src/warpkateview.cpp"

echo "Fixing the final syntax error in the connection..."

# Create a backup
cp "$CPP_FILE" "${CPP_FILE}.bak.$(date +%Y%m%d_%H%M%S)"

# Find the connection lines
LINE_NUM=$(grep -n "connect(m_conversationArea" "$CPP_FILE" | cut -d':' -f1)

if [ -n "$LINE_NUM" ]; then
    # Calculate the range of lines to replace
    END_LINE=$((LINE_NUM + 2))
    
    # Replace the entire connection block with properly formatted code
    sed -i "${LINE_NUM},${END_LINE}c\\    connect(m_conversationArea, \\&QTextBrowser::anchorClicked, this, \\&WarpKateView::onLinkClicked, Qt::UniqueConnection);" "$CPP_FILE"
    
    echo "Successfully fixed the connection syntax."
else
    echo "Could not find the problematic connection."
    exit 1
fi

echo "Fix applied. Please rebuild the project."

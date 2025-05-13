#!/bin/bash
# Script to manually fix the connection issue in warpkateview.cpp

CPP_FILE="/home/yani/Projects/WarpKate/src/warpkateview.cpp"

echo "Fixing the anchorClicked connection line..."

# Create a backup
cp "$CPP_FILE" "${CPP_FILE}.bak.$(date +%Y%m%d_%H%M%S)"

# Find the line with the bad connection
LINE_NUM=$(grep -n "anchorClickedQTextBrowser" "$CPP_FILE" | cut -d':' -f1)

if [ -n "$LINE_NUM" ]; then
    # Replace the entire line with the correct connection
    sed -i "${LINE_NUM}c\\    connect(m_conversationArea, \&QTextBrowser::anchorClicked, \\n            this, \&WarpKateView::onLinkClicked,\\n            Qt::UniqueConnection);" "$CPP_FILE"
    echo "Successfully fixed the connection line."
else
    echo "Could not find the problematic line."
    exit 1
fi

echo "Fix applied. Please rebuild the project."

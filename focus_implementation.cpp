// ==== Add these methods to warpkateview.cpp ====

/**
 * Update the list of interactive elements in the conversation area
 */
void WarpKateView::updateInteractiveElements()
{
    m_interactiveElements.clear();
    
    QTextDocument *doc = m_conversationArea->document();
    
    // Scan document for links (a href tags)
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            QTextFragment fragment = it.fragment();
            
            if (fragment.isValid()) {
                QTextCharFormat format = fragment.charFormat();
                
                if (format.isAnchor()) {
                    // Found a link - create an ExtraSelection for it
                    QString href = format.anchorHref();
                    
                    // Only include file:// URLs
                    if (href.startsWith(QStringLiteral("file://"))) {
                        QTextEdit::ExtraSelection selection;
                        
                        // Normal style (will be modified when focused)
                        selection.format.setForeground(QBrush(QColor(0, 0, 200))); // Dark blue
                        selection.format.setBackground(QBrush(QColor(240, 240, 255))); // Light blue background
                        selection.format.setProperty(QTextFormat::AnchorHref, href);
                        
                        // Get the cursor for this fragment
                        QTextCursor cursor(doc);
                        cursor.setPosition(fragment.position());
                        cursor.setPosition(fragment.position() + fragment.length(), QTextCursor::KeepAnchor);
                        selection.cursor = cursor;
                        
                        m_interactiveElements.append(selection);
                    }
                }
            }
        }
    }
    
    // Reset focus index if needed
    if (m_currentFocusIndex >= m_interactiveElements.size()) {
        m_currentFocusIndex = -1;
    }
    
    // Apply styles
    applyInteractiveElementStyles();
}

/**
 * Focus the next interactive element (when Tab is pressed)
 */
void WarpKateView::focusNextInteractiveElement()
{
    // Update the list first in case content has changed
    updateInteractiveElements();
    
    if (m_interactiveElements.isEmpty()) {
        return;
    }
    
    // Move to next item or wrap around
    m_currentFocusIndex++;
    if (m_currentFocusIndex >= m_interactiveElements.size()) {
        m_currentFocusIndex = 0;
    }
    
    // Apply styles and scroll to the focused element
    applyInteractiveElementStyles();
    
    // Scroll to make the focused element visible
    if (m_currentFocusIndex >= 0 && m_currentFocusIndex < m_interactiveElements.size()) {
        m_conversationArea->setTextCursor(m_interactiveElements[m_currentFocusIndex].cursor);
        m_conversationArea->ensureCursorVisible();
    }
}

/**
 * Focus the previous interactive element (when Shift+Tab is pressed)
 */
void WarpKateView::focusPreviousInteractiveElement()
{
    // Update the list first in case content has changed
    updateInteractiveElements();
    
    if (m_interactiveElements.isEmpty()) {
        return;
    }
    
    // Move to previous item or wrap around
    m_currentFocusIndex--;
    if (m_currentFocusIndex < 0) {
        m_currentFocusIndex = m_interactiveElements.size() - 1;
    }
    
    // Apply styles and scroll to the focused element
    applyInteractiveElementStyles();
    
    // Scroll to make the focused element visible
    if (m_currentFocusIndex >= 0 && m_currentFocusIndex < m_interactiveElements.size()) {
        m_conversationArea->setTextCursor(m_interactiveElements[m_currentFocusIndex].cursor);
        m_conversationArea->ensureCursorVisible();
    }
}

/**
 * Apply highlighting styles to interactive elements based on current focus
 */
void WarpKateView::applyInteractiveElementStyles()
{
    // Create a new list of extra selections
    QList<QTextEdit::ExtraSelection> extraSelections;
    
    // Add each interactive element with appropriate style
    for (int i = 0; i < m_interactiveElements.size(); ++i) {
        QTextEdit::ExtraSelection selection = m_interactiveElements[i];
        
        if (i == m_currentFocusIndex) {
            // Focused item - highlight more intensely
            selection.format.setForeground(QBrush(QColor(0, 0, 200))); // Dark blue text
            selection.format.setBackground(QBrush(QColor(200, 220, 255))); // Stronger blue background
            selection.format.setFontWeight(QFont::Bold);
        } else if (i == m_lastClickedIndex && m_clickFeedbackTimer->isActive()) {
            // Clicked item during feedback flash - red highlight
            selection.format.setForeground(QBrush(QColor(200, 0, 0))); // Red text
            selection.format.setBackground(QBrush(QColor(255, 220, 220))); // Light red background
            selection.format.setFontWeight(QFont::Bold);
        } else {
            // Normal item (not focused) - subtle styling
            selection.format.setForeground(QBrush(QColor(0, 0, 150))); // Blue text
            selection.format.setBackground(QBrush(Qt::transparent)); // No background
            selection.format.setFontWeight(QFont::Normal);
        }
        
        extraSelections.append(selection);
    }
    
    // Apply the extra selections to the text edit
    m_conversationArea->setExtraSelections(extraSelections);
}

/**
 * Flash a click feedback effect on the clicked element
 */
void WarpKateView::flashClickFeedback(int elementIndex)
{
    if (elementIndex >= 0 && elementIndex < m_interactiveElements.size()) {
        m_lastClickedIndex = elementIndex;
        
        // Apply styles with click feedback
        applyInteractiveElementStyles();
        
        // Start timer to remove feedback after a short delay
        m_clickFeedbackTimer->start();
    }
}

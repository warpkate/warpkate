/*
 * Enhancements for interactive file paths in WarpKate terminal output
 * 
 * This adds:
 * 1. Tab key navigation for interactive elements
 * 2. Blue highlighting for focused elements 
 * 3. Red flash effect when clicking items
 * 4. Visual styling for the entire item
 */

// ======== Add to the WarpKateView class header (warpkateview.h) ========

// Add these members to the private section:
private:
    // For interactive element focus handling
    int m_currentFocusIndex;
    QList<QTextEdit::ExtraSelection> m_interactiveElements;
    QTimer *m_clickFeedbackTimer;
    int m_lastClickedIndex;
    
    // Add these method declarations
    void updateInteractiveElements();
    void focusNextInteractiveElement();
    void focusPreviousInteractiveElement();
    void applyInteractiveElementStyles();
    void flashClickFeedback(int elementIndex);

// ======== Add to WarpKateView constructor (in warpkateview.cpp) ========

// Add this to the member initialization list:
, m_currentFocusIndex(-1)
, m_lastClickedIndex(-1)

// Add this to the constructor body:
    // Create click feedback timer
    m_clickFeedbackTimer = new QTimer(this);
    m_clickFeedbackTimer->setSingleShot(true);
    m_clickFeedbackTimer->setInterval(200); // 200ms flash
    connect(m_clickFeedbackTimer, &QTimer::timeout, this, [this]() {
        applyInteractiveElementStyles(); // Restore normal styles after flash
    });

// ======== Add these methods to warpkateview.cpp ========

/**
 * Update the list of interactive elements in the conversation area
 * This scans the document for links and builds a list of ExtraSelection objects
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
 * @param elementIndex Index of the clicked element
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

// ======== Update the onLinkClicked method to include click feedback ========

void WarpKateView::onLinkClicked(const QUrl &url)
{
    qDebug() << "WarpKate: Link clicked:" << url.toString();
    
    // Handle file:// URLs specially for our interactive file listings
    if (url.scheme() == QStringLiteral("file")) {
        QString filePath = url.toLocalFile();
        QFileInfo fileInfo(filePath);
        
        // Find which element was clicked for feedback
        int clickedIndex = -1;
        for (int i = 0; i < m_interactiveElements.size(); ++i) {
            QTextEdit::ExtraSelection selection = m_interactiveElements[i];
            QString href = selection.format.anchorHref();
            if (href == url.toString()) {
                clickedIndex = i;
                break;
            }
        }
        
        // Show click feedback
        flashClickFeedback(clickedIndex);
        
        // Rest of the method continues as before...
        // [existing code from onLinkClicked]
    }
}

// ======== Add keyboard event handling to the conversation area ========

// In setupUI method, after creating m_conversationArea, add:
    m_conversationArea->installEventFilter(this);

// Then update the eventFilter method:
bool WarpKateView::eventFilter(QObject *obj, QEvent *event)
{
    // Handle events for the conversation area (interactive navigation)
    if (obj == m_conversationArea && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        
        if (keyEvent->key() == Qt::Key_Tab) {
            if (keyEvent->modifiers() & Qt::ShiftModifier) {
                // Shift+Tab - focus previous interactive element
                focusPreviousInteractiveElement();
            } else {
                // Tab - focus next interactive element
                focusNextInteractiveElement();
            }
            return true; // Event handled
        } else if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            // Enter key - activate the focused element
            if (m_currentFocusIndex >= 0 && m_currentFocusIndex < m_interactiveElements.size()) {
                // Get the URL from the focused element
                QString href = m_interactiveElements[m_currentFocusIndex].format.anchorHref();
                
                // Show click feedback
                flashClickFeedback(m_currentFocusIndex);
                
                // Trigger the link click
                QUrl url(href);
                onLinkClicked(url);
                return true; // Event handled
            }
        }
    }
    
    // Handle events for the prompt input as before
    if (obj == m_promptInput) {
        // [existing code for m_promptInput event handling]
    }
    
    // Let the default handler process other events
    return QObject::eventFilter(obj, event);
}

// ======== Update the onTerminalOutput method to set up interactive elements ========

// At the end of the onTerminalOutput method, after inserting the processed output:
    // If the output contains HTML (and potentially links), update interactive elements
    if (processedOutput.contains(QStringLiteral("<a "))) {
        QTimer::singleShot(100, this, &WarpKateView::updateInteractiveElements);
    }

# Manual Integration Guide for Focus Enhancement

This guide will help you integrate the focus enhancement feature into WarpKate's codebase.

## 1. Update the warpkateview.h file

First, you need to add member variables and method declarations to the header file:

1. Open the file: `/home/yani/Projects/WarpKate/src/warpkateview.h`

2. Find the `private:` section near the end of the file.

3. Add these member variables and method declarations right after the `private:` line:

```cpp
    // For interactive element highlighting
    int m_currentFocusIndex = -1;
    QList<QTextEdit::ExtraSelection> m_interactiveElements;
    QTimer *m_clickFeedbackTimer = nullptr;
    int m_lastClickedIndex = -1;
    
    // Methods for interactive element handling
    void updateInteractiveElements();
    void focusNextInteractiveElement();
    void focusPreviousInteractiveElement();
    void applyInteractiveElementStyles();
    void flashClickFeedback(int elementIndex);
```

## 2. Update the warpkateview.cpp file

Now, you need to make several updates to the implementation file:

1. Open the file: `/home/yani/Projects/WarpKate/src/warpkateview.cpp`

2. Initialize the timer in the constructor:
   - Find the WarpKateView constructor (around line 38)
   - Add this code right before the `setupUI();` call:
   
   ```cpp
   // Create click feedback timer
   m_clickFeedbackTimer = new QTimer(this);
   m_clickFeedbackTimer->setSingleShot(true);
   m_clickFeedbackTimer->setInterval(200); // 200ms flash
   connect(m_clickFeedbackTimer, &QTimer::timeout, this, [this]() {
       applyInteractiveElementStyles(); // Restore normal styles
   });
   ```

3. Add event filter to the conversation area:
   - Find where m_conversationArea is created (search for `m_conversationArea = new QTextBrowser`)
   - Add this line after its creation:
   
   ```cpp
   // Install event filter for keyboard navigation
   m_conversationArea->installEventFilter(this);
   ```

4. Update the onTerminalOutput method:
   - Find the onTerminalOutput method (search for `void WarpKateView::onTerminalOutput`)
   - Add these lines right before the last `m_conversationArea->ensureCursorVisible();` call:
   
   ```cpp
   // If the output contains HTML links, update interactive elements
   if (processedOutput.contains(QStringLiteral("<a "))) {
       QTimer::singleShot(100, this, &WarpKateView::updateInteractiveElements);
   }
   ```

5. Update the eventFilter method:
   - Find the eventFilter method (search for `bool WarpKateView::eventFilter`)
   - Add these lines at the beginning of the method:
   
   ```cpp
   // Handle events for conversation area (Tab navigation)
   if (obj == m_conversationArea && event->type() == QEvent::KeyPress) {
       QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
       
       if (keyEvent->key() == Qt::Key_Tab) {
           if (keyEvent->modifiers() & Qt::ShiftModifier) {
               // Shift+Tab moves to previous element
               focusPreviousInteractiveElement();
           } else {
               // Tab moves to next element
               focusNextInteractiveElement();
           }
           return true; // Event handled
       } else if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
           // Enter key activates the focused element
           if (m_currentFocusIndex >= 0 && m_currentFocusIndex < m_interactiveElements.size()) {
               QString href = m_interactiveElements[m_currentFocusIndex].format.anchorHref();
               flashClickFeedback(m_currentFocusIndex);
               onLinkClicked(QUrl(href));
               return true; // Event handled
           }
       }
   }
   ```

6. Update the onLinkClicked method:
   - Find the onLinkClicked method (search for `void WarpKateView::onLinkClicked`)
   - Find where it handles file:// URLs (search for `if (url.scheme() == QStringLiteral("file")`)
   - Add these lines right after that if statement:
   
   ```cpp
   // Find which element was clicked for feedback
   int clickedIndex = -1;
   for (int i = 0; i < m_interactiveElements.size(); ++i) {
       QString href = m_interactiveElements[i].format.anchorHref();
       if (href == url.toString()) {
           clickedIndex = i;
           break;
       }
   }
   
   // Show click feedback
   flashClickFeedback(clickedIndex);
   ```

7. Add the implementation methods:
   - Copy all the code from the file `/home/yani/Projects/WarpKate/focus_implementation.cpp`
   - Paste it at the end of the warpkateview.cpp file (before the final closing brace)

## 3. Build and Install

After making these changes, build and install the plugin:

```bash
cd /home/yani/Projects/WarpKate
./build_and_install.sh
```

## 4. Testing

After installation:

1. Restart Kate
2. Open the WarpKate terminal
3. Run a command that displays files (like `ls -l`)
4. Press Tab to cycle through the file/folder items
5. Press Enter to open the focused item
6. Click on items to see the red flash feedback

## Troubleshooting

If you encounter compilation errors:

1. Check for typos in the added code
2. Make sure all code is added at the correct locations
3. Ensure all opening and closing braces are balanced
4. Check for missing semicolons

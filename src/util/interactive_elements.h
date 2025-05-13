/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INTERACTIVE_ELEMENTS_H
#define INTERACTIVE_ELEMENTS_H

#include <QObject>
#include <QTextEdit>
#include <QTextBrowser>
#include <QTimer>
#include <QList>
#include <QUrl>

/**
 * @brief The InteractiveElements class
 * 
 * This class manages interactive elements (clickable/selectable) in the terminal output,
 * such as file and directory paths, command suggestions, and other actionable items.
 * 
 * It handles:
 * - Tracking the list of interactive elements in the output
 * - Keyboard navigation between elements (Tab/Shift+Tab)
 * - Visual styling of focused and clicked elements
 * - Providing click feedback
 */
class InteractiveElements : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor
     * @param parent Parent object
     * @param textBrowser The text browser widget containing interactive elements
     */
    explicit InteractiveElements(QObject *parent = nullptr, QTextBrowser *textBrowser = nullptr);
    
    /**
     * Destructor
     */
    ~InteractiveElements();
    
    /**
     * Set the text browser to manage
     * @param textBrowser Text browser widget
     */
    void setTextBrowser(QTextBrowser *textBrowser);
    
    /**
     * Update the list of interactive elements in the text
     * Scans the document for links and builds a navigation structure
     */
    void updateInteractiveElements();
    
    /**
     * Focus the next interactive element (Tab navigation)
     */
    void focusNextInteractiveElement();
    
    /**
     * Focus the previous interactive element (Shift+Tab navigation)
     */
    void focusPreviousInteractiveElement();
    
    /**
     * Apply highlighting styles to elements based on focus
     */
    void applyInteractiveElementStyles();
    
    /**
     * Show feedback flash when an element is clicked
     * @param elementIndex Index of the clicked element
     */
    void flashClickFeedback(int elementIndex);
    
    /**
     * Get the URL of the currently focused element
     * @return URL of the focused element or empty URL if none focused
     */
    QUrl getCurrentElementUrl() const;
    
    /**
     * Check if there are any interactive elements
     * @return True if there are interactive elements
     */
    bool hasInteractiveElements() const;
    
    /**
     * Get the number of interactive elements
     * @return Count of interactive elements
     */
    int elementCount() const;

Q_SIGNALS:
    /**
     * Emitted when an element is focused via keyboard navigation
     * @param url URL of the focused element
     */
    void elementFocused(const QUrl &url);
    
    /**
     * Emitted when an element is activated (Enter key pressed)
     * @param url URL of the activated element
     */
    void elementActivated(const QUrl &url);

private Q_SLOTS:
    /**
     * Handle timeout of the click feedback timer
     */
    void onClickFeedbackTimeout();

private:
    /**
     * Structure representing an interactive element
     */
    struct InteractiveElement {
        QTextCursor cursor;        ///< Cursor positioned at the element
        QTextCharFormat format;    ///< Format of the element
        QString text;              ///< Text content of the element
        QUrl url;                  ///< URL associated with the element
    };
    
    QTextBrowser *m_textBrowser;                   ///< The text browser being managed
    QList<InteractiveElement> m_elements;          ///< List of interactive elements
    int m_currentFocusIndex;                       ///< Index of currently focused element
    int m_lastClickedIndex;                        ///< Index of last clicked element
    QTimer *m_clickFeedbackTimer;                  ///< Timer for click feedback effect
    
    /**
     * Initialize the component
     */
    void initialize();
    
    /**
     * Find elements in the document and add them to the list
     */
    void scanForElements();
};

#endif // INTERACTIVE_ELEMENTS_H


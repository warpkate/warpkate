/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interactive_elements.h"

#include <QDebug>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextFragment>
#include <QColor>
#include <QBrush>
#include <QTextEdit>
#include <QUrl>

InteractiveElements::InteractiveElements(QObject *parent, QTextBrowser *textBrowser)
    : QObject(parent)
    , m_textBrowser(textBrowser)
    , m_currentFocusIndex(-1)
    , m_lastClickedIndex(-1)
{
    initialize();
}

InteractiveElements::~InteractiveElements()
{
    // Clean up the timer if needed
    if (m_clickFeedbackTimer) {
        m_clickFeedbackTimer->stop();
    }
}

void InteractiveElements::initialize()
{
    // Create click feedback timer
    m_clickFeedbackTimer = new QTimer(this);
    m_clickFeedbackTimer->setSingleShot(true);
    m_clickFeedbackTimer->setInterval(200); // 200ms flash
    
    // Connect signals/slots
    connect(m_clickFeedbackTimer, &QTimer::timeout, 
            this, &InteractiveElements::onClickFeedbackTimeout);
}

void InteractiveElements::setTextBrowser(QTextBrowser *textBrowser)
{
    m_textBrowser = textBrowser;
    
    // When the text browser changes, update the element list
    if (m_textBrowser) {
        updateInteractiveElements();
    }
}

void InteractiveElements::updateInteractiveElements()
{
    // Clear existing elements
    m_elements.clear();
    
    if (!m_textBrowser) {
        qDebug() << "InteractiveElements: No text browser set";
        return;
    }
    
    // Scan for interactive elements in the document
    scanForElements();
    
    // Reset focus index if needed
    if (m_currentFocusIndex >= m_elements.size()) {
        m_currentFocusIndex = -1;
    }
    
    // Apply the styles to highlight the elements
    applyInteractiveElementStyles();
    
    qDebug() << "InteractiveElements: Found" << m_elements.size() << "interactive elements";
}

void InteractiveElements::scanForElements()
{
    if (!m_textBrowser) {
        return;
    }
    
    QTextDocument *doc = m_textBrowser->document();
    
    // Scan document for links (a href tags)
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            QTextFragment fragment = it.fragment();
            
            if (fragment.isValid()) {
                QTextCharFormat format = fragment.charFormat();
                
                if (format.isAnchor()) {
                    // Found a link - create an element for it
                    QString href = format.anchorHref();
                    
                    // Only include file:// URLs or other specific URL types if needed
                    if (href.startsWith(QStringLiteral("file://"))) {
                        InteractiveElement element;
                        element.format = format;
                        element.url = QUrl(href);
                        element.text = fragment.text();
                        
                        // Get the cursor for this fragment
                        QTextCursor cursor(doc);
                        cursor.setPosition(fragment.position());
                        cursor.setPosition(fragment.position() + fragment.length(), QTextCursor::KeepAnchor);
                        element.cursor = cursor;
                        
                        m_elements.append(element);
                    }
                }
            }
        }
    }
}

void InteractiveElements::focusNextInteractiveElement()
{
    if (m_elements.isEmpty()) {
        return;
    }
    
    // Move to next item or wrap around
    m_currentFocusIndex++;
    if (m_currentFocusIndex >= m_elements.size()) {
        m_currentFocusIndex = 0;
    }
    
    // Apply styles and scroll to the focused element
    applyInteractiveElementStyles();
    
    // Scroll to make the focused element visible
    if (m_currentFocusIndex >= 0 && m_currentFocusIndex < m_elements.size() && m_textBrowser) {
        m_textBrowser->setTextCursor(m_elements[m_currentFocusIndex].cursor);
        m_textBrowser->ensureCursorVisible();
        
        // Emit signal with the focused element's URL
        Q_EMIT elementFocused(m_elements[m_currentFocusIndex].url);
    }
    
    qDebug() << "InteractiveElements: Focused element" << m_currentFocusIndex;
}

void InteractiveElements::focusPreviousInteractiveElement()
{
    if (m_elements.isEmpty()) {
        return;
    }
    
    // Move to previous item or wrap around
    m_currentFocusIndex--;
    if (m_currentFocusIndex < 0) {
        m_currentFocusIndex = m_elements.size() - 1;
    }
    
    // Apply styles and scroll to the focused element
    applyInteractiveElementStyles();
    
    // Scroll to make the focused element visible
    if (m_currentFocusIndex >= 0 && m_currentFocusIndex < m_elements.size() && m_textBrowser) {
        m_textBrowser->setTextCursor(m_elements[m_currentFocusIndex].cursor);
        m_textBrowser->ensureCursorVisible();
        
        // Emit signal with the focused element's URL
        Q_EMIT elementFocused(m_elements[m_currentFocusIndex].url);
    }
    
    qDebug() << "InteractiveElements: Focused element" << m_currentFocusIndex;
}

void InteractiveElements::applyInteractiveElementStyles()
{
    if (!m_textBrowser) {
        return;
    }
    
    // Create a new list of extra selections for the text browser
    QList<QTextEdit::ExtraSelection> extraSelections;
    
    // Add each interactive element with appropriate style
    for (int i = 0; i < m_elements.size(); ++i) {
        QTextEdit::ExtraSelection selection;
        selection.cursor = m_elements[i].cursor;
        
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
        
        // Preserve the anchor property
        selection.format.setAnchor(true);
        selection.format.setAnchorHref(m_elements[i].format.anchorHref());
        
        extraSelections.append(selection);
    }
    
    // Apply the extra selections to the text browser
    m_textBrowser->setExtraSelections(extraSelections);
}

void InteractiveElements::flashClickFeedback(int elementIndex)
{
    if (elementIndex >= 0 && elementIndex < m_elements.size()) {
        m_lastClickedIndex = elementIndex;
        
        // Apply styles with click feedback
        applyInteractiveElementStyles();
        
        // Start timer to remove feedback after a short delay
        m_clickFeedbackTimer->start();
        
        qDebug() << "InteractiveElements: Flashing click feedback for element" << elementIndex;
    }
}

void InteractiveElements::onClickFeedbackTimeout()
{
    // Clear click feedback
    m_lastClickedIndex = -1;
    
    // Reapply styles without click feedback
    applyInteractiveElementStyles();
    
    qDebug() << "InteractiveElements: Click feedback timeout";
}

QUrl InteractiveElements::getCurrentElementUrl() const
{
    if (m_currentFocusIndex >= 0 && m_currentFocusIndex < m_elements.size()) {
        return m_elements[m_currentFocusIndex].url;
    }
    
    return QUrl();
}

bool InteractiveElements::hasInteractiveElements() const
{
    return !m_elements.isEmpty();
}

int InteractiveElements::elementCount() const
{
    return m_elements.size();
}


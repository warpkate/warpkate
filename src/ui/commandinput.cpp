/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "commandinput.h"

#include <QDebug>
#include <QKeyEvent>
#include <QApplication>
#include <QClipboard>
#include <QRegularExpression>
#include <QTimer>
#include <QTextCursor>
#include <KConfigGroup>
#include <KSharedConfig>

CommandInput::CommandInput(QWidget *parent)
    : QTextEdit(parent)
    , m_currentMode(CommandMode)
    , m_historyIndex(-1)
    , m_assistantName(QStringLiteral("WarpKate"))
    , m_autocompleteTimer(nullptr)
{
    initialize();
}

CommandInput::~CommandInput()
{
    // Clean up the timer if needed
    if (m_autocompleteTimer) {
        m_autocompleteTimer->stop();
    }
}

void CommandInput::initialize()
{
    // Set up the text edit
    setAcceptRichText(false);
    setTabChangesFocus(true);
    setLineWrapMode(QTextEdit::WidgetWidth);
    
    // Set fixed height for the input area
    setFixedHeight(80);
    setMaximumHeight(80);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    // Set a monospace font
    QFont font(QStringLiteral("Monospace"));
    setFont(font);
    
    // Apply styling - black background with white text
    setStyleSheet(QStringLiteral("QTextEdit { background-color: black; color: white; border: none; border-radius: 3px; }"));
    
    // Initialize the autocomplete timer
    m_autocompleteTimer = new QTimer(this);
    m_autocompleteTimer->setSingleShot(true);
    m_autocompleteTimer->setInterval(300); // 300ms delay before showing suggestions
    connect(m_autocompleteTimer, &QTimer::timeout, this, &CommandInput::showAutocompleteSuggestions);
    
    // Initialize placeholder text
    updatePlaceholderText();
}

void CommandInput::setInputMode(InputMode mode)
{
    if (m_currentMode != mode) {
        m_currentMode = mode;
        updatePlaceholderText();
        
        // Emit signal for mode change
        Q_EMIT inputModeChanged(mode == AIMode);
        
        qDebug() << "CommandInput: Mode changed to" << (mode == AIMode ? "AI Mode" : "Command Mode");
    }
}

CommandInput::InputMode CommandInput::inputMode() const
{
    return m_currentMode;
}

void CommandInput::setCommandHistory(const QStringList &history)
{
    m_commandHistory = history;
    m_historyIndex = -1; // Reset navigation
}

QStringList CommandInput::commandHistory() const
{
    return m_commandHistory;
}

void CommandInput::setAssistantName(const QString &name)
{
    m_assistantName = name;
    updatePlaceholderText();
}

void CommandInput::clear()
{
    // Clear the text edit
    QTextEdit::clear();
    
    // Reset history navigation
    m_historyIndex = -1;
    m_savedPartialCommand.clear();
}

void CommandInput::setModeIcons(const QIcon &commandIcon, const QIcon &aiIcon)
{
    m_commandIcon = commandIcon;
    m_aiIcon = aiIcon;
}

void CommandInput::submitInput()
{
    QString input = toPlainText().trimmed();
    if (input.isEmpty()) {
        return;
    }
    
    // Reset history navigation for the next command
    m_historyIndex = -1;
    m_savedPartialCommand.clear();
    
    // If this is a command, add it to history
    if (m_currentMode == CommandMode && !isAIQuery()) {
        // Add to command history (avoid duplicates at the end)
        if (m_commandHistory.isEmpty() || m_commandHistory.last() != input) {
            m_commandHistory.append(input);
        }
        
        // Emit command signal
        Q_EMIT commandSubmitted(input);
    } else {
        // Process as an AI query
        if (m_currentMode == CommandMode && isAIQuery()) {
            // Extract the query part (remove the ? or assistant name)
            if (input.startsWith(QStringLiteral("?"))) {
                input = input.mid(1).trimmed();
            } else if (!m_assistantName.isEmpty() && input.startsWith(m_assistantName)) {
                input = input.mid(m_assistantName.length()).trimmed();
            }
        }
        
        // Emit AI query signal
        Q_EMIT aiQuerySubmitted(input);
    }
    
    // Clear the input
    QTextEdit::clear();
}

void CommandInput::navigateCommandHistory(int direction)
{
    if (m_commandHistory.isEmpty()) {
        return;
    }
    
    // Save the current input if starting navigation
    if (m_historyIndex == -1) {
        m_savedPartialCommand = toPlainText();
    }
    
    // Navigate in the history
    if (direction > 0) {
        // Navigate backward (older commands)
        if (m_historyIndex < m_commandHistory.size() - 1) {
            m_historyIndex++;
        }
    } else {
        // Navigate forward (newer commands)
        if (m_historyIndex > 0) {
            m_historyIndex--;
        } else if (m_historyIndex == 0) {
            // Return to the saved partial command
            m_historyIndex = -1;
        }
    }
    
    // Update the text based on the new history index
    if (m_historyIndex >= 0 && m_historyIndex < m_commandHistory.size()) {
        // Show the historical command
        setText(m_commandHistory.at(m_commandHistory.size() - 1 - m_historyIndex));
    } else if (m_historyIndex == -1) {
        // Restore the saved partial command
        setText(m_savedPartialCommand);
    }
    
    // Move the cursor to the end
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);
}

void CommandInput::setAIMode(bool aiMode)
{
    setInputMode(aiMode ? AIMode : CommandMode);
}

bool CommandInput::event(QEvent *event)
{
    // Handle special events
    if (event->type() == QEvent::KeyPress) {
        // Let keyPressEvent handle it
        return QTextEdit::event(event);
    }
    
    // Default event handling
    return QTextEdit::event(event);
}

void CommandInput::keyPressEvent(QKeyEvent *event)
{
    // Handle special key combinations
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        // Enter key submits the input (but not Shift+Enter)
        if (!(event->modifiers() & Qt::ShiftModifier)) {
            submitInput();
            event->accept();
            return;
        }
    } else if (event->key() == Qt::Key_Up) {
        // Up arrow navigates backward in history
        navigateCommandHistory(1);
        event->accept();
        return;
    } else if (event->key() == Qt::Key_Down) {
        // Down arrow navigates forward in history
        navigateCommandHistory(-1);
        event->accept();
        return;
    } else if (event->key() == Qt::Key_Tab) {
        // Tab requests autocompletion
        if (m_currentMode == CommandMode) {
            // Request autocomplete suggestions
            QString text = toPlainText();
            int position = textCursor().position();
            Q_EMIT autocompleteRequested(text, position);
            event->accept();
            return;
        }
    } else if (event->key() == Qt::Key_Greater) {
        // ">" character toggles mode if input is empty
        if (toPlainText().isEmpty()) {
            setAIMode(!(m_currentMode == AIMode));
            event->accept();
            return;
        }
    }
    
    // Default key handling
    QTextEdit::keyPressEvent(event);
    
    // Start autocomplete timer if typing and in command mode
    if (m_currentMode == CommandMode && 
        event->key() != Qt::Key_Shift && 
        event->key() != Qt::Key_Control && 
        event->key() != Qt::Key_Alt && 
        event->key() != Qt::Key_Meta) {
        
        // Reset and start the timer
        m_autocompleteTimer->stop();
        m_autocompleteTimer->start();
    }
}

bool CommandInput::isAIQuery() const
{
    QString text = toPlainText().trimmed();
    
    // Check if it starts with "?"
    if (text.startsWith(QStringLiteral("?"))) {
        return true;
    }
    
    // Check if it starts with the assistant name
    if (!m_assistantName.isEmpty() && text.startsWith(m_assistantName, Qt::CaseInsensitive)) {
        return true;
    }
    
    return false;
}

void CommandInput::updatePlaceholderText()
{
    if (m_currentMode == AIMode) {
        setPlaceholderText(QStringLiteral("> Ask me anything..."));
    } else {
        // Command mode with assistant hint
        if (!m_assistantName.isEmpty()) {
            setPlaceholderText(QStringLiteral("> Type command or '?' for AI assistant"));
        } else {
            setPlaceholderText(QStringLiteral("> Type command..."));
        }
    }
}

void CommandInput::showAutocompleteSuggestions()
{
    // Stub implementation
    // In a real implementation, this would show autocomplete suggestions
    qDebug() << "CommandInput: Autocomplete suggestions requested";
    
    QString text = toPlainText();
    int position = textCursor().position();
    
    // Extract the current word being typed
    QString currentWord;
    if (position > 0) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
        currentWord = cursor.selectedText();
    }
    
    // Emit signal for autocomplete request
    Q_EMIT autocompleteRequested(text, position);
}


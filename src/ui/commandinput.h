/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef COMMANDINPUT_H
#define COMMANDINPUT_H

#include <QTextEdit>
#include <QStringList>
#include <QIcon>
#include <QTimer>

/**
 * @brief The CommandInput class
 * 
 * This class manages the command input area in the terminal, handling:
 * - Command input processing
 * - Command history navigation
 * - Autocomplete functionality
 * - Mode switching between terminal commands and AI queries
 */
class CommandInput : public QTextEdit
{
    Q_OBJECT

public:
    /**
     * Enum representing input modes
     */
    enum InputMode {
        CommandMode,    ///< Terminal command mode
        AIMode          ///< AI assistant mode
    };
    
    /**
     * Constructor
     * @param parent Parent widget
     */
    explicit CommandInput(QWidget *parent = nullptr);
    
    /**
     * Destructor
     */
    ~CommandInput();
    
    /**
     * Set the current input mode
     * @param mode The input mode to set
     */
    void setInputMode(InputMode mode);
    
    /**
     * Get the current input mode
     * @return Current input mode
     */
    InputMode inputMode() const;
    
    /**
     * Set the command history
     * @param history List of previous commands
     */
    void setCommandHistory(const QStringList &history);
    
    /**
     * Get the command history
     * @return List of commands in history
     */
    QStringList commandHistory() const;
    
    /**
     * Set the AI assistant name for recognition in commands
     * @param name Name of the AI assistant
     */
    void setAssistantName(const QString &name);
    
    /**
     * Clear the input field
     */
    void clear();
    
    /**
     * Set icons for mode display
     * @param commandIcon Icon for command mode
     * @param aiIcon Icon for AI mode
     */
    void setModeIcons(const QIcon &commandIcon, const QIcon &aiIcon);

public Q_SLOTS:
    /**
     * Submit the current input
     * If in command mode and input starts with '?' or AI name, switch to AI mode
     */
    void submitInput();
    
    /**
     * Navigate command history in the given direction
     * @param direction Direction to navigate (1 for older, -1 for newer)
     */
    void navigateCommandHistory(int direction);
    
    /**
     * Toggle between command and AI modes
     * @param aiMode True to switch to AI mode, false for command mode
     */
    void setAIMode(bool aiMode);

Q_SIGNALS:
    /**
     * Emitted when a command should be executed
     * @param command The command to execute
     */
    void commandSubmitted(const QString &command);
    
    /**
     * Emitted when an AI query should be processed
     * @param query The query to send to the AI
     */
    void aiQuerySubmitted(const QString &query);
    
    /**
     * Emitted when the input mode changes
     * @param aiMode True if switched to AI mode, false for command mode
     */
    void inputModeChanged(bool aiMode);
    
    /**
     * Emitted to request autocomplete suggestions
     * @param text Current input text
     * @param position Cursor position
     */
    void autocompleteRequested(const QString &text, int position);

protected:
    /**
     * Handle keyboard and other events
     * @param event The event to handle
     * @return True if the event was handled
     */
    bool event(QEvent *event) override;
    
    /**
     * Handle key press events
     * @param event Key event information
     */
    void keyPressEvent(QKeyEvent *event) override;
    
private:
    /**
     * Initialize the widget
     */
    void initialize();
    
    /**
     * Detect if the current input is an AI query
     * @return True if input appears to be an AI query
     */
    bool isAIQuery() const;
    
    /**
     * Update the placeholder text based on current mode
     */
    void updatePlaceholderText();
    
    /**
     * Show autocomplete suggestions
     */
    void showAutocompleteSuggestions();
    
    // Current state
    InputMode m_currentMode;
    QStringList m_commandHistory;
    int m_historyIndex;
    QString m_savedPartialCommand;
    QString m_assistantName;
    QString m_currentAutocompletion;
    
    // Icons for mode display
    QIcon m_commandIcon;
    QIcon m_aiIcon;
    
    // Autocomplete timer
    QTimer *m_autocompleteTimer;
};

#endif // COMMANDINPUT_H


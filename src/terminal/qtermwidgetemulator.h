/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef QTERMWIDGETEMULATOR_H
#define QTERMWIDGETEMULATOR_H

#include "terminalemulator.h"

#include <QObject>
#include <QColor>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QMap>
#include <QPoint>
#include <QRegularExpression>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QWidget>
#include <QProcess>

class QTermWidget;

/**
 * Terminal emulator implementation using QTermWidget
 * 
 * This class provides a terminal emulator implementation that uses the
 * QTermWidget library to handle terminal functionality. It implements the
 * same interface as TerminalEmulator but delegates the terminal handling
 * to QTermWidget.
 */
class QTermWidgetEmulator : public TerminalEmulator
{
    Q_OBJECT

public:
    /**
     * Constructor
     * @param parent Parent widget
     */
    explicit QTermWidgetEmulator(QWidget *parent = nullptr);
    
    /**
     * Destructor
     */
    ~QTermWidgetEmulator() override;
    
    /**
     * Initialize the terminal emulator
     * @param rows Number of rows
     * @param cols Number of columns
     * @return True if initialization was successful
     */
    bool initialize(int rows, int cols) override;
    
    /**
     * Start a shell process in the terminal
     * @param shellCommand Command to run (default: system shell)
     * @param initialWorkingDirectory Initial working directory
     * @return True if the shell was started successfully
     */
    bool startShell(const QString &shellCommand = QString(), 
                   const QString &initialWorkingDirectory = QString()) override;
    
    /**
     * Resize the terminal
     * @param rows New number of rows
     * @param cols New number of columns
     */
    void resize(int rows, int cols) override;
    
    /**
     * Process input from the user and send it to the shell
     * @param text Text input from the user
     */
    void processInput(const QString &text) override;
    
    /**
     * Process a key press and send it to the shell
     * @param key The Qt key code
     * @param modifiers Keyboard modifiers
     * @param text Text from the key press
     */
    void processKeyPress(int key, Qt::KeyboardModifiers modifiers, const QString &text) override;
    
    /**
     * Execute a command in the terminal
     * @param command Command to execute
     * @param addNewline Whether to add a newline at the end
     */
    void executeCommand(const QString &command, bool addNewline = true) override;
    
    /**
     * Clear the terminal screen
     */
    void clear() override;
    
    /**
     * Get the terminal size
     * @return Terminal size in columns and rows
     */
    QSize size() const override;
    
    /**
     * Get the character at the specified position
     * @param x Column
     * @param y Row
     * @return Character at the position
     */
    QChar characterAt(int x, int y) const override;
    
    /**
     * Get the format at the specified position
     * @param x Column
     * @param y Row
     * @return Format at the position
     */
    TerminalCharFormat formatAt(int x, int y) const override;
    
    /**
     * Get the cursor position
     * @return Current cursor position (x, y)
     */
    QPoint cursorPosition() const override;
    
    /**
     * Set the cursor position
     * @param x Column
     * @param y Row
     */
    void setCursorPosition(int x, int y) override;
    
    /**
     * Get the cursor style
     * @return Current cursor style
     */
    CursorStyle cursorStyle() const override;
    
    /**
     * Set the cursor style
     * @param style New cursor style
     */
    void setCursorStyle(CursorStyle style) override;
    
    /**
     * Get whether the cursor is visible
     * @return True if the cursor is visible
     */
    bool isCursorVisible() const override;
    
    /**
     * Set whether the cursor is visible
     * @param visible Whether the cursor should be visible
     */
    void setCursorVisible(bool visible) override;
    
    /**
     * Get whether the terminal is in alternative screen mode
     * @return True if in alternative screen mode
     */
    bool isAlternateScreenActive() const override;
    
    /**
     * Set the default foreground color
     * @param color Default foreground color
     */
    void setDefaultForegroundColor(const QColor &color) override;
    
    /**
     * Set the default background color
     * @param color Default background color
     */
    void setDefaultBackgroundColor(const QColor &color) override;
    
    /**
     * Get the terminal content as text
     * @param stripFormatting Whether to strip formatting
     * @return Terminal content as text
     */
    QString getText(bool stripFormatting = true) const override;
    
    /**
     * Get a line of text from the terminal
     * @param line Line number
     * @param stripFormatting Whether to strip formatting
     * @return Line of text
     */
    QString getLine(int line, bool stripFormatting = true) const override;
    
    /**
     * Get the current working directory of the shell
     * @return Current working directory
     */
    QString currentWorkingDirectory() const override;
    
    /**
     * Check if the terminal is busy (shell process is running)
     * @return True if the terminal is busy
     */
    bool isBusy() const override;
    
    /**
     * Get the exit code of the last command
     * @return Exit code of the last command
     */
    int lastExitCode() const override;
    
    /**
     * Get all screen data
     * @return Vector of screen lines
     */
    const QVector<TerminalLine> &screenData() const override;
    
    /**
     * Get the current command being typed
     * @return Current command
     */
    QString currentCommand() const override;
    
    /**
     * Get the current prompt
     * @return Current prompt string
     */
    QString currentPrompt() const override;
    
    /**
     * Get the command history
     * @return List of previous commands
     */
    QStringList commandHistory() const override;

    /**
     * Get access to the underlying QTermWidget instance
     * @return Pointer to the QTermWidget
     */
    QTermWidget* termWidget() const;

public Q_SLOTS:
    /**
     * Handle shell output
     */
    void readFromShell() override;
    
    /**
     * Handle shell process finished
     * @param exitCode Exit code of the shell process
     * @param exitStatus Exit status of the shell process
     */
    void shellProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) override;
    
    /**
     * Handle shell process error
     * @param error Error that occurred
     */
    void shellProcessError(QProcess::ProcessError error) override;
    
    /**
     * Copy selected text to clipboard
     */
    void copyToClipboard() override;
    
    /**
     * Paste text from clipboard
     */
    void pasteFromClipboard() override;
    
    /**
     * Select all text
     */
    void selectAll() override;
    
    /**
     * Find text in the terminal
     * @param text Text to find
     * @param caseSensitive Whether the search is case sensitive
     * @param searchForward Whether to search forward
     * @return True if text was found
     */
    bool findText(const QString &text, bool caseSensitive = false, bool searchForward = true) override;

private Q_SLOTS:
    /**
     * Handle terminal output from QTermWidget
     */
    void onTerminalOutput();
    
    /**
     * Handle terminal finished signal
     * @param exitCode Exit code of the terminal process
     */
    void onTerminalFinished(int exitCode);
    
    /**
     * Handle title changed signal
     * @param title New terminal title
     */
    void onTitleChanged(QString title);
    
    /**
     * Handle cursor position changed
     */
    void onCursorPositionChanged();
    
    /**
     * Handle command detection timer timeout
     */
    void onCommandDetectionTimer();

private:
    /**
     * Setup the QTermWidget
     */
    void setupTermWidget();
    
    /**
     * Connect QTermWidget signals to slots
     */
    void connectSignals();
    
    /**
     * Detect command from terminal output
     */
    void detectCommand();
    
    /**
     * Detect working directory from terminal output
     */
    void detectWorkingDirectory();
    
    /**
     * Parse screendump output and update screen data
     */
    void updateScreenData();
    
    /**
     * Parse and update command information from terminal
     */
    void updateCommandInfo();

private:
    QTermWidget *m_termWidget;                          ///< QTermWidget instance
    QSize m_termSize;                                   ///< Terminal size
    bool m_initialized;                                 ///< Whether the terminal is initialized
    
    // Command tracking
    QString m_currentCommand;                           ///< Current command being typed
    QString m_currentPrompt;                            ///< Current prompt
    QString m_currentOutput;                            ///< Current command output
    QStringList m_commandHistory;                       ///< Command history
    int m_lastExitCode;                                 ///< Last command exit code
    
    // Directory tracking
    QString m_workingDirectory;                         ///< Current working directory
    
    // Screen data
    QVector<TerminalLine> m_screenData;                 ///< Screen data
    QPoint m_cursorPos;                                 ///< Current cursor position
    bool m_cursorVisible;                               ///< Cursor visibility
    CursorStyle m_cursorStyle;                          ///< Cursor style
    bool m_alternateScreenActive;                       ///< Whether alternate screen is active
    
    // Command detection
    QTimer m_commandDetectionTimer;                     ///< Timer for command detection
    bool m_commandExecuting;                            ///< Whether a command is currently executing
    QDateTime m_commandStartTime;                       ///< Start time of current command
    
    // Regexes for detection
    QRegularExpression m_promptRegex;                   ///< Regex for detecting prompts
    QRegularExpression m_cwdRegex;                      ///< Regex for detecting working directory
    QRegularExpression m_exitCodeRegex;                 ///< Regex for detecting exit codes
};

#endif // QTERMWIDGETEMULATOR_H


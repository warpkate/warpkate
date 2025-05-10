/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TERMINALEMULATOR_H
#define TERMINALEMULATOR_H

#include <QObject>
#include <QColor>
#include <QHash>
#include <QList>
#include <QMap>
#include <QPoint>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QWidget>
#include <QProcess>
#include <QSocketNotifier>

/**
 * Terminal character format attributes
 */
enum TerminalAttribute {
    Bold = 0x01,
    Italic = 0x02,
    Underline = 0x04,
    StrikeThrough = 0x08,
    Reverse = 0x10,
    Blink = 0x20,
    Dim = 0x40,
    Invisible = 0x80
};

/**
 * Character format for a terminal cell
 */
struct TerminalCharFormat {
    QColor foreground;      ///< Foreground color
    QColor background;      ///< Background color
    int attributes;         ///< Attributes (combination of TerminalAttribute flags)
    
    TerminalCharFormat() : foreground(Qt::white), background(Qt::black), attributes(0) {}
    
    bool operator==(const TerminalCharFormat &other) const {
        return foreground == other.foreground &&
               background == other.background &&
               attributes == other.attributes;
    }
    
    bool operator!=(const TerminalCharFormat &other) const {
        return !(*this == other);
    }
};

/**
 * Terminal cell containing a character and its format
 */
struct TerminalCell {
    QChar character;               ///< The character in this cell
    TerminalCharFormat format;     ///< Format of this cell
    
    TerminalCell() : character(' ') {}
    TerminalCell(QChar ch, const TerminalCharFormat &fmt) : character(ch), format(fmt) {}
    
    bool operator==(const TerminalCell &other) const {
        return character == other.character && format == other.format;
    }
    
    bool operator!=(const TerminalCell &other) const {
        return !(*this == other);
    }
};

/**
 * Terminal screen line
 */
typedef QVector<TerminalCell> TerminalLine;

/**
 * Cursor state in the terminal
 */
enum CursorStyle {
    Block,
    Underline,
    IBeam
};

/**
 * Class for handling terminal emulation
 * 
 * This class provides VT100/ANSI terminal emulation capabilities, 
 * manages the terminal state, and interfaces with a pseudo-terminal
 * for communication with the shell.
 */
class TerminalEmulator : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor
     * @param parent Parent widget
     */
    explicit TerminalEmulator(QWidget *parent = nullptr);
    
    /**
     * Destructor
     */
    ~TerminalEmulator() override;
    
    /**
     * Initialize the terminal emulator
     * @param rows Number of rows
     * @param cols Number of columns
     * @return True if initialization was successful
     */
    bool initialize(int rows, int cols);
    
    /**
     * Start a shell process in the terminal
     * @param shellCommand Command to run (default: system shell)
     * @param initialWorkingDirectory Initial working directory
     * @return True if the shell was started successfully
     */
    bool startShell(const QString &shellCommand = QString(), 
                   const QString &initialWorkingDirectory = QString());
    
    /**
     * Resize the terminal
     * @param rows New number of rows
     * @param cols New number of columns
     */
    void resize(int rows, int cols);
    
    /**
     * Process input from the user and send it to the shell
     * @param text Text input from the user
     */
    void processInput(const QString &text);
    
    /**
     * Process a key press and send it to the shell
     * @param key The Qt key code
     * @param modifiers Keyboard modifiers
     * @param text Text from the key press
     */
    void processKeyPress(int key, Qt::KeyboardModifiers modifiers, const QString &text);
    
    /**
     * Execute a command in the terminal
     * @param command Command to execute
     * @param addNewline Whether to add a newline at the end
     */
    void executeCommand(const QString &command, bool addNewline = true);
    
    /**
     * Clear the terminal screen
     */
    void clear();
    
    /**
     * Get the terminal size
     * @return Terminal size in columns and rows
     */
    QSize size() const;
    
    /**
     * Get the character at the specified position
     * @param x Column
     * @param y Row
     * @return Character at the position
     */
    QChar characterAt(int x, int y) const;
    
    /**
     * Get the format at the specified position
     * @param x Column
     * @param y Row
     * @return Format at the position
     */
    TerminalCharFormat formatAt(int x, int y) const;
    
    /**
     * Get the cursor position
     * @return Current cursor position (x, y)
     */
    QPoint cursorPosition() const;
    
    /**
     * Set the cursor position
     * @param x Column
     * @param y Row
     */
    void setCursorPosition(int x, int y);
    
    /**
     * Get the cursor style
     * @return Current cursor style
     */
    CursorStyle cursorStyle() const;
    
    /**
     * Set the cursor style
     * @param style New cursor style
     */
    void setCursorStyle(CursorStyle style);
    
    /**
     * Get whether the cursor is visible
     * @return True if the cursor is visible
     */
    bool isCursorVisible() const;
    
    /**
     * Set whether the cursor is visible
     * @param visible Whether the cursor should be visible
     */
    void setCursorVisible(bool visible);
    
    /**
     * Get whether the terminal is in alternative screen mode
     * @return True if in alternative screen mode
     */
    bool isAlternateScreenActive() const;
    
    /**
     * Set the default foreground color
     * @param color Default foreground color
     */
    void setDefaultForegroundColor(const QColor &color);
    
    /**
     * Set the default background color
     * @param color Default background color
     */
    void setDefaultBackgroundColor(const QColor &color);
    
    /**
     * Get the terminal content as text
     * @param stripFormatting Whether to strip formatting
     * @return Terminal content as text
     */
    QString getText(bool stripFormatting = true) const;
    
    /**
     * Get a line of text from the terminal
     * @param line Line number
     * @param stripFormatting Whether to strip formatting
     * @return Line of text
     */
    QString getLine(int line, bool stripFormatting = true) const;
    
    /**
     * Get the current working directory of the shell
     * @return Current working directory
     */
    QString currentWorkingDirectory() const;
    
    /**
     * Check if the terminal is busy (shell process is running)
     * @return True if the terminal is busy
     */
    bool isBusy() const;
    
    /**
     * Get the exit code of the last command
     * @return Exit code of the last command
     */
    int lastExitCode() const;
    
    /**
     * Get all screen data
     * @return Vector of screen lines
     */
    const QVector<TerminalLine> &screenData() const;
    
    /**
     * Get the current command being typed
     * @return Current command
     */
    QString currentCommand() const;
    
    /**
     * Get the current prompt
     * @return Current prompt string
     */
    QString currentPrompt() const;
    
    /**
     * Get the command history
     * @return List of previous commands
     */
    QStringList commandHistory() const;

public Q_SLOTS:
    /**
     * Handle shell output
     */
    void readFromShell();
    
    /**
     * Handle shell process finished
     * @param exitCode Exit code of the shell process
     * @param exitStatus Exit status of the shell process
     */
    void shellProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    
    /**
     * Handle shell process error
     * @param error Error that occurred
     */
    void shellProcessError(QProcess::ProcessError error);
    
    /**
     * Copy selected text to clipboard
     */
    void copyToClipboard();
    
    /**
     * Paste text from clipboard
     */
    void pasteFromClipboard();
    
    /**
     * Select all text
     */
    void selectAll();
    
    /**
     * Find text in the terminal
     * @param text Text to find
     * @param caseSensitive Whether the search is case sensitive
     * @param searchForward Whether to search forward
     * @return True if text was found
     */
    bool findText(const QString &text, bool caseSensitive = false, bool searchForward = true);

Q_SIGNALS:
    /**
     * Emitted when terminal output is available
     * @param text Output text
     */
    void outputAvailable(const QString &text);
    
    /**
     * Emitted when the terminal size changes
     * @param size New terminal size
     */
    void sizeChanged(const QSize &size);
    
    /**
     * Emitted when the cursor position changes
     * @param position New cursor position
     */
    void cursorPositionChanged(const QPoint &position);
    
    /**
     * Emitted when the shell process finishes
     * @param exitCode Exit code of the shell process
     */
    void shellFinished(int exitCode);
    
    /**
     * Emitted when a command is detected
     * @param command Command text
     */
    void commandDetected(const QString &command);
    
    /**
     * Emitted when a command execution is completed
     * @param command Command text
     * @param output Command output
     * @param exitCode Exit code of the command
     */
    void commandExecuted(const QString &command, const QString &output, int exitCode);
    
    /**
     * Emitted when the working directory changes
     * @param directory New working directory
     */
    void workingDirectoryChanged(const QString &directory);
    
    /**
     * Emitted when the terminal requires a redraw
     */
    void redrawRequired();
    
    /**
     * Emitted when the terminal bell is triggered
     */
    void bellTriggered();
    
    /**
     * Emitted when the terminal title changes
     * @param title New terminal title
     */
    void titleChanged(const QString &title);

private:
    /**
     * Process VT100/ANSI escape sequences
     * @param data Data to process
     */
    void processOutputData(const QByteArray &data);
    
    /**
     * Process a single escape sequence
     * @param sequence Escape sequence to process
     * @return Number of bytes processed
     */
    int processEscapeSequence(const QByteArray &sequence);
    
    /**
     * Parse an SGR (Select Graphic Rendition) sequence
     * @param parameters Parameters of the sequence
     */
    void processSGR(const QList<int> &parameters);
    
    /**
     * Parse a CSI (Control Sequence Introducer) sequence
     * @param sequence CSI sequence to parse
     * @return Number of bytes processed
     */
    int processCSI(const QByteArray &sequence);
    
    /**
     * Parse an OSC (Operating System Command) sequence
     * @param sequence OSC sequence to parse
     * @return Number of bytes processed
     */
    int processOSC(const QByteArray &sequence);
    
    /**
     * Put a character at the current cursor position
     * @param ch Character to put
     */
    void putCharacter(QChar ch);
    
    /**
     * Move the cursor
     * @param dx Change in x position
     * @param dy Change in y position
     */
    void moveCursor(int dx, int dy);
    
    /**
     * Set the cursor position absolutely
     * @param x New x position (column)
     * @param y New y position (row)
     * @param clampToScreen Whether to clamp the position to the screen bounds
     */
    void setCursorPositionInternal(int x, int y, bool clampToScreen = true);
    
    /**
     * Scroll the screen
     * @param lines Number of lines to scroll (positive: up, negative: down)
     */
    void scrollScreen(int lines);
    
    /**
     * Create a new blank line
     * @return Blank line initialized with spaces
     */
    TerminalLine createBlankLine() const;
    
    /**
     * Detect a command in the terminal output
     */
    void detectCommand();
    
    /**
     * Detect the current working directory in the terminal output
     */
    void detectWorkingDirectory();
    
    /**
     * Detect the command prompt
     */
    void detectPrompt();
    
    /**
     * Detect the exit code of the last command
     */
    void detectExitCode();
    
    /**
     * Parse parameters from an escape sequence
     * @param sequence Escape sequence
     * @param start Start position within the sequence
     * @param length Length of the parameter section
     * @return List of parsed parameters
     */
    QList<int> parseParameters(const QByteArray &sequence, int start, int length);
    
    /**
     * Is the character a valid escape sequence final byte?
     * @param ch Character to check
     * @return True if it's a valid escape sequence final byte
     */
    bool isEscapeSequenceFinal(char ch) const;

private:
    // Terminal state
    QVector<TerminalLine> m_screen;            ///< Screen buffer
    QVector<TerminalLine> m_alternateScreen;   ///< Alternate screen buffer
    TerminalCharFormat m_currentFormat;        ///< Current character format
    QPoint m_cursorPosition;                   ///< Current cursor position
    QSize m_terminalSize;                      ///< Terminal size in columns and rows
    bool m_cursorVisible;                      ///< Whether the cursor is visible
    CursorStyle m_cursorStyle;                 ///< Current cursor style
    bool m_alternateScreenActive;              ///< Whether alternate screen is active
    bool m_applicationCursorKeys;              ///< Whether application cursor keys mode is active
    bool m_bracketedPasteMode;                 ///< Whether bracketed paste mode is active
    int m_scrollRegionTop;                     ///< Top of scroll region
    int m_scrollRegionBottom;                  ///< Bottom of scroll region
    QColor m_defaultForeground;                ///< Default foreground color
    QColor m_defaultBackground;                ///< Default background color
    QByteArray m_escapeBuffer;                 ///< Buffer for escape sequences
    bool m_parsingEscapeSequence;              ///< Whether an escape sequence is being parsed
    bool m_newLineMode;                        ///< Line feed/new line mode
    
    // Process handling
    int m_ptyFd;                               ///< File descriptor for the pseudo-terminal
    QSocketNotifier *m_ptyNotifier;            ///< Socket notifier for the PTY
    pid_t m_shellPid;                          ///< PID of the shell process
    QString m_shellCommand;                    ///< Command used to start the shell
    QString m_workingDirectory;                ///< Current working directory
    int m_lastExitCode;                        ///< Exit code of the last command
    
    // Command tracking
    QString m_currentCommand;                  ///< Current command being typed
    QString m_currentPrompt;                   ///< Current prompt string
    QString m_currentOutput;                   ///< Output of the current command
    QStringList m_commandHistory;              ///< History of executed commands
    bool m_commandExecuting;                   ///< Whether a command is currently executing
    QDateTime m_commandStartTime;              ///< Start time of the current command
    
    // Selection and clipboard
    QPoint m_selectionStart;                   ///< Start position of the selection
    QPoint m_selectionEnd;                     ///< End position of the selection
    bool m_hasSelection;                       ///< Whether there is an active selection
    
    // Color palette
    QMap<int, QColor> m_colorPalette;          ///< Terminal color palette (0-255)
    
    // Timers
    QTimer m_cursorBlinkTimer;                 ///< Timer for cursor blinking
    QTimer m_commandDetectionTimer;            ///< Timer for command detection
    
    // State tracking
    bool m_initialized;                        ///< Whether the terminal has been initialized
    bool m_busy;                               ///< Whether the terminal is busy
    QString m_terminalTitle;                   ///< Current terminal title
    QByteArray m_pendingOutput;                ///< Pending output to be processed
    
    // Block model integration
    bool m_blockModeEnabled;                   ///< Whether block mode is enabled
    int m_currentBlockId;                      ///< ID of the current command block
    
    // Regular expressions for detection
    QRegularExpression m_promptRegex;          ///< Regex for detecting prompts
    QRegularExpression m_cwdRegex;             ///< Regex for detecting working directory
    QRegularExpression m_exitCodeRegex;        ///< Regex for detecting exit codes
};

#endif // TERMINALEMULATOR_H

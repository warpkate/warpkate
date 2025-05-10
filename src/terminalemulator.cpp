/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "terminalemulator.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDebug>
#include <QRegularExpression>
#include <QSocketNotifier>

// System includes for PTY handling
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <utmp.h>

// For PTY functions
#include <pty.h>

// ANSI/VT100 control sequences
#define ESC "\033"
#define CSI "\033["
#define OSC "\033]"
#define BEL "\007"
#define ST  "\033\\"

TerminalEmulator::TerminalEmulator(QWidget *parent)
    : QObject(parent)
    , m_ptyFd(-1)
    , m_ptyNotifier(nullptr)
    , m_shellPid(0)
    , m_lastExitCode(0)
    , m_commandExecuting(false)
    , m_hasSelection(false)
    , m_initialized(false)
    , m_busy(false)
    , m_blockModeEnabled(true)
    , m_currentBlockId(0)
{
    // Initialize default values
    m_cursorPosition = QPoint(0, 0);
    m_cursorVisible = true;
    m_cursorStyle = Block;
    m_alternateScreenActive = false;
    m_applicationCursorKeys = false;
    m_bracketedPasteMode = false;
    m_parsingEscapeSequence = false;
    m_newLineMode = false;
    
    // Default colors
    m_defaultForeground = Qt::white;
    m_defaultBackground = Qt::black;
    m_currentFormat.foreground = m_defaultForeground;
    m_currentFormat.background = m_defaultBackground;
    m_currentFormat.attributes = 0;
    
    // Initialize color palette (ANSI 16 colors)
    m_colorPalette[0] = QColor(0, 0, 0);          // Black
    m_colorPalette[1] = QColor(170, 0, 0);        // Red
    m_colorPalette[2] = QColor(0, 170, 0);        // Green
    m_colorPalette[3] = QColor(170, 85, 0);       // Yellow
    m_colorPalette[4] = QColor(0, 0, 170);        // Blue
    m_colorPalette[5] = QColor(170, 0, 170);      // Magenta
    m_colorPalette[6] = QColor(0, 170, 170);      // Cyan
    m_colorPalette[7] = QColor(170, 170, 170);    // White
    m_colorPalette[8] = QColor(85, 85, 85);       // Bright Black
    m_colorPalette[9] = QColor(255, 85, 85);      // Bright Red
    m_colorPalette[10] = QColor(85, 255, 85);     // Bright Green
    m_colorPalette[11] = QColor(255, 255, 85);    // Bright Yellow
    m_colorPalette[12] = QColor(85, 85, 255);     // Bright Blue
    m_colorPalette[13] = QColor(255, 85, 255);    // Bright Magenta
    m_colorPalette[14] = QColor(85, 255, 255);    // Bright Cyan
    m_colorPalette[15] = QColor(255, 255, 255);   // Bright White
    
    // Set up timers
    connect(&m_cursorBlinkTimer, &QTimer::timeout, this, [this]() {
        // Toggle cursor visibility and emit signal for redraw
        m_cursorVisible = !m_cursorVisible;
        emit redrawRequired();
    });
    
    connect(&m_commandDetectionTimer, &QTimer::timeout, this, &TerminalEmulator::detectCommand);
    
    // Set up regular expressions for detection
    m_promptRegex = QRegularExpression(R"(^\s*[\w\-]+(:\s*[\w~/\-.]+)?\s*[\$#%>](\s+|$))");
    m_cwdRegex = QRegularExpression(R"(^([a-zA-Z]:|~|/)[^:]*$)");
    m_exitCodeRegex = QRegularExpression(R"(\[(\d+)\])");
}

TerminalEmulator::~TerminalEmulator()
{
    // Clean up
    if (m_ptyNotifier) {
        delete m_ptyNotifier;
        m_ptyNotifier = nullptr;
    }
    
    // Close PTY
    if (m_ptyFd >= 0) {
        ::close(m_ptyFd);
        m_ptyFd = -1;
    }
    
    // Kill shell process if still running
    if (m_shellPid > 0) {
        ::kill(m_shellPid, SIGTERM);
        m_shellPid = 0;
    }
}

bool TerminalEmulator::initialize(int rows, int cols)
{
    if (m_initialized) {
        return true;
    }
    
    // Set terminal size
    m_terminalSize = QSize(cols, rows);
    
    // Initialize screen buffer
    m_screen.clear();
    m_screen.reserve(rows);
    for (int i = 0; i < rows; ++i) {
        m_screen.append(createBlankLine());
    }
    
    // Initialize alternate screen buffer
    m_alternateScreen.clear();
    m_alternateScreen.reserve(rows);
    for (int i = 0; i < rows; ++i) {
        m_alternateScreen.append(createBlankLine());
    }
    
    // Set scroll region to full terminal
    m_scrollRegionTop = 0;
    m_scrollRegionBottom = rows - 1;
    
    // Reset cursor
    m_cursorPosition = QPoint(0, 0);
    
    // Start cursor blinking
    m_cursorBlinkTimer.start(500);
    
    m_initialized = true;
    return true;
}

bool TerminalEmulator::startShell(const QString &shellCommand, const QString &initialWorkingDirectory)
{
    if (m_shellPid > 0) {
        // Shell already running
        return true;
    }
    
    // Get default shell if none specified
    QString shell = shellCommand;
    if (shell.isEmpty()) {
        // Use SHELL environment variable or fallback to /bin/bash
        const char *envShell = getenv("SHELL");
        if (envShell) {
            shell = QString::fromUtf8(envShell);
        } else {
            shell = "/bin/bash";
        }
    }
    
    // Set up working directory
    QString workingDir = initialWorkingDirectory;
    if (workingDir.isEmpty()) {
        // Use HOME environment variable or current directory
        const char *envHome = getenv("HOME");
        if (envHome) {
            workingDir = QString::fromUtf8(envHome);
        } else {
            workingDir = QDir::currentPath();
        }
    }
    
    // Store shell command
    m_shellCommand = shell;
    m_workingDirectory = workingDir;
    
    // Open a pseudo-terminal
    int master, slave;
    char ptyName[100];
    
    if (openpty(&master, &slave, ptyName, nullptr, nullptr) == -1) {
        qWarning() << "Failed to open pseudo-terminal:" << strerror(errno);
        return false;
    }
    
    // Fork a new process
    pid_t pid = fork();
    if (pid == -1) {
        qWarning() << "Failed to fork process:" << strerror(errno);
        ::close(master);
        ::close(slave);
        return false;
    }
    
    if (pid == 0) {
        // Child process - becomes the shell
        
        // Close master side of the PTY
        ::close(master);
        
        // Create a new session and set process group
        setsid();
        
        // Set controlling terminal
        ioctl(slave, TIOCSCTTY, 0);
        
        // Duplicate slave to stdin/stdout/stderr
        ::dup2(slave, STDIN_FILENO);
        ::dup2(slave, STDOUT_FILENO);
        ::dup2(slave, STDERR_FILENO);
        
        // Close remaining references to slave
        if (slave > STDERR_FILENO) {
            ::close(slave);
        }
        
        // Change to requested working directory
        if (chdir(workingDir.toUtf8().constData()) == -1) {
            qWarning() << "Failed to change directory to" << workingDir;
            exit(1);
        }
        
        // Set environment variables
        setenv("TERM", "xterm-256color", 1);
        
        // Execute the shell
        QStringList shellParts = shell.split(' ');
        QString shellProgram = shellParts.takeFirst();
        
        QVector<char*> args;
        QByteArray shellProgramBytes = shellProgram.toUtf8();
        args.append(shellProgramBytes.data());
        
        QVector<QByteArray> argBytes;
        for (const QString &arg : shellParts) {
            argBytes.append(arg.toUtf8());
            args.append(argBytes.last().data());
        }
        
        // Add null terminator
        args.append(nullptr);
        
        // Execute shell
        execvp(shellProgramBytes.constData(), args.data());
        
        // If we get here, exec failed
        fprintf(stderr, "exec failed: %s\n", strerror(errno));
        exit(1);
    }
    
    // Parent process
    
    // Close slave side
    ::close(slave);
    
    // Store master file descriptor and shell PID
    m_ptyFd = master;
    m_shellPid = pid;
    
    // Set file descriptor to non-blocking mode
    int flags = fcntl(m_ptyFd, F_GETFL, 0);
    fcntl(m_ptyFd, F_SETFL, flags | O_NONBLOCK);
    
    // Set up notifier for shell output
    m_ptyNotifier = new QSocketNotifier(m_ptyFd, QSocketNotifier::Read, this);
    connect(m_ptyNotifier, &QSocketNotifier::activated, this, &TerminalEmulator::readFromShell);
    
    // Set terminal size
    resize(m_terminalSize.height(), m_terminalSize.width());
    
    // Set flag
    m_busy = true;
    
    return true;
}

void TerminalEmulator::resize(int rows, int cols)
{
    // Update size
    m_terminalSize = QSize(cols, rows);
    
    // Resize screen buffer
    QVector<TerminalLine> newScreen;
    newScreen.reserve(rows);
    
    // Copy existing screen content
    for (int i = 0; i < qMin(rows, m_screen.size()); ++i) {
        TerminalLine newLine;
        newLine.reserve(cols);
        
        // Copy existing content
        for (int j = 0; j < qMin(cols, m_screen[i].size()); ++j) {
            newLine.append(m_screen[i][j]);
        }
        
        // Fill the rest with spaces
        while (newLine.size() < cols) {
            newLine.append(TerminalCell(' ', m_currentFormat));
        }
        
        newScreen.append(newLine);
    }
    
    // Add blank lines if needed
    while (newScreen.size() < rows) {
        newScreen.append(createBlankLine());
    }
    
    // Update screen
    m_screen = newScreen;
    
    // Apply the same resizing to alternate screen
    if (m_alternateScreenActive) {
        QVector<TerminalLine> newAltScreen;
        newAltScreen.reserve(rows);
        
        for (int i = 0; i < qMin(rows, m_alternateScreen.size()); ++i) {
            TerminalLine newLine;
            newLine.reserve(cols);
            
            for (int j = 0; j < qMin(cols, m_alternateScreen[i].size()); ++j) {
                newLine.append(m_alternateScreen[i][j]);
            }
            
            while (newLine.size() < cols) {
                newLine.append(TerminalCell(' ', m_currentFormat));
            }
            
            newAltScreen.append(newLine);
        }
        
        while (newAltScreen.size() < rows) {
            newAltScreen.append(createBlankLine());
        }
        
        m_alternateScreen = newAltScreen;
    }
    
    // Clamp cursor position
    setCursorPositionInternal(m_cursorPosition.x(), m_cursorPosition.y(), true);
    
    // Adjust scroll region if necessary
    m_scrollRegionTop = qMin(m_scrollRegionTop, rows - 1);
    m_scrollRegionBottom = qMin(m_scrollRegionBottom, rows - 1);
    
    // Update PTY size
    if (m_ptyFd >= 0) {
        struct winsize size;
        size.ws_row = rows;
        size.ws_col = cols;
        size.ws_xpixel = 0;
        size.ws_ypixel = 0;
        
        if (ioctl(m_ptyFd, TIOCSWINSZ, &size) == -1) {
            qWarning() << "Failed to set terminal size:" << strerror(errno);
        }
    }
    
    // Emit size changed signal
    emit sizeChanged(m_terminalSize);
    emit redrawRequired();
}

void TerminalEmulator::processInput(const QString &text)
{
    if (m_ptyFd < 0 || !m_busy) {
        return;
    }
    
    // Write input to the PTY
    QByteArray data = text.toUtf8();
    
    // Handle bracketed paste mode
    if (m_bracketedPasteMode && data.length() > 0) {
        QByteArray bracketedData = QByteArray(CSI) + "200~" + data + QByteArray(CSI) + "201~";
        ::write(m_ptyFd, bracketedData.constData(), bracketedData.length());
    } else {
        ::write(m_ptyFd, data.constData(), data.length());
    }
    
    // Detect command
    m_commandDetectionTimer.start(100);
}

void TerminalEmulator::processKeyPress(int key, Qt::KeyboardModifiers modifiers, const QString &text)
{
    if (m_ptyFd < 0 || !m_busy) {
        return;
    }
    
    // Handle special keys with appropriate escape sequences
    QByteArray data;
    
    bool altPressed = modifiers & Qt::AltModifier;
    bool ctrlPressed = modifiers & Qt::ControlModifier;
    bool shiftPressed = modifiers & Qt::ShiftModifier;
    
    switch (key) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            data = QByteArray("\r");
            break;
            
        case Qt::Key_Tab:
            data = QByteArray("\t");
            break;
            
        case Qt::Key_Backspace:
            data = QByteArray("\x7f"); // ASCII DEL
            break;
            
        case Qt::Key_Escape:
            data = QByteArray(ESC);
            break;
            
        case Qt::Key_Up:
            if (m_applicationCursorKeys) {
                data = QByteArray(CSI) + "A";
            } else {
                data = QByteArray(ESC) + "[A";
            }
            break;
            
        case Qt::Key_Down:
            if (m_applicationCursorKeys) {
                data = QByteArray(CSI) + "B";
            } else {
                data = QByteArray(ESC) + "[B";
            }
            break;
            
        case Qt::Key_Right:
            if (m_applicationCursorKeys) {
                data = QByteArray(CSI) + "C";
            } else {
                data = QByteArray(ESC) + "[C";
            }
            break;
            
        case Qt::Key_Left:
            if (m_applicationCursorKeys) {
                data = QByteArray(CSI) + "D";
            } else {
                data = QByteArray(ESC) + "[D";
            }
            break;
            
        case Qt::Key_Home:
            data = QByteArray(CSI) + "H";
            break;
            
        case Qt::Key_End:
            data = QByteArray(CSI) + "F";
            break;
            
        case Qt::Key_Insert:
            data = QByteArray(CSI) + "2~";
            break;
            
        case Qt::Key_Delete:
            data = QByteArray(CSI) + "3~";
            break;
            
        case Qt::Key_PageUp:
            data = QByteArray(CSI) + "5~";
            break;
            
        case Qt::Key_PageDown:
            data = QByteArray(CSI) + "6~";
            break;
            
        case Qt::Key_F1:
            data = QByteArray(ESC) + "OP";
            break;
            
        case Qt::Key_F2:
            data = QByteArray(ESC) + "OQ";
            break;
            
        case Qt::Key_F3:
            data = QByteArray(ESC) + "OR";
            break;
            
        case Qt::Key_F4:
            data = QByteArray(ESC) + "OS";
            break;
            
        case Qt::Key_F5:
            data = QByteArray(CSI) + "15~";
            break;
            
        case Qt::Key_F6:
            data = QByteArray(CSI) + "17~";
            break;
            
        case Qt::Key_F7:
            data = QByteArray(CSI) + "18~";
            break;
            
        case Qt::Key_F8:
            data = QByteArray(CSI) + "19~";
            break;
            
        case Qt::Key_F9:
            data = QByteArray(CSI) + "20~";
            break;
            
        case Qt::Key_F10:
            data = QByteArray(CSI) + "21~";
            break;
            
        case Qt::Key_F11:
            data = QByteArray(CSI) + "23~";
            break;
            
        case Qt::Key_F12:
            data = QByteArray(CSI) + "24~";
            break;
            
        default:
            // For most keys, just use the text
            if (!text.isEmpty()) {
                // Handle Ctrl+key combinations
                if (ctrlPressed && key >= Qt::Key_A && key <= Qt::Key_Z) {
                    // Ctrl+A is 1, Ctrl+B is 2, etc.
                    char ctrlChar = key - Qt::Key_A + 1;
                    data = QByteArray(1, ctrlChar);
                } else if (ctrlPressed && key >= Qt::Key_BracketLeft && key <= Qt::Key_BracketRight) {
                    // Special control characters for [ ] and others
                    char ctrlChar = key - Qt::Key_BracketLeft + 27;
                    data = QByteArray(1, ctrlChar);
                } else if (altPressed) {
                    // Alt+key sends ESC+key
                    data = QByteArray(ESC) + text.toUtf8();
                } else {
                    data = text.toUtf8();
                }
            }
            break;
    }
    
    // Send the key to the PTY
    if (!data.isEmpty()) {
        ::write(m_ptyFd, data.constData(), data.length());
    }
    
    // Detect command
    m_commandDetectionTimer.start(100);
}

void TerminalEmulator::executeCommand(const QString &command, bool addNewline)
{
    if (m_ptyFd < 0 || !m_busy) {
        return;
    }
    
    // Write command to the PTY
    QByteArray data = command.toUtf8();
    ::write(m_ptyFd, data.constData(), data.length());
    
    // Add newline if requested
    if (addNewline) {
        ::write(m_ptyFd, "\r", 1);
    }
    
    // Record command start
    m_currentCommand = command;
    m_commandExecuting = true;
    m_commandStartTime = QDateTime::currentDateTime();
    
    // Update block if block mode is enabled
    if (m_blockModeEnabled) {
        m_currentBlockId++;
        // Notify that a new command was started
        emit commandDetected(command);
    }
}

void TerminalEmulator::readFromShell()
{
    if (m_ptyFd < 0) {
        return;
    }
    
    // Read data from the PTY
    char buffer[4096];
    ssize_t bytesRead = ::read(m_ptyFd, buffer, sizeof(buffer));
    
    if (bytesRead > 0) {
        // Process the data
        QByteArray data = QByteArray::fromRawData(buffer, bytesRead);
        processOutputData(data);
        
        // Accumulate output if a command is executing
        if (m_commandExecuting) {
            m_currentOutput.append(QString::fromUtf8(data));
        }
        
        // Emit signal for raw output
        emit outputAvailable(QString::fromUtf8(data));
        
        // Schedule command detection
        m_commandDetectionTimer.start(100);
    } else if (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        qWarning() << "Error reading from shell:" << strerror(errno);
    } else if (bytesRead == 0) {
        // EOF - shell has exited
        m_busy = false;
        m_shellPid = 0;
        
        if (m_ptyNotifier) {
            m_ptyNotifier->setEnabled(false);
        }
        
        if (m_ptyFd >= 0) {
            ::close(m_ptyFd);
            m_ptyFd = -1;
        }
        
        emit shellFinished(m_lastExitCode);
    }
}

void TerminalEmulator::processOutputData(const QByteArray &data)
{
    // Process each character
    for (int i = 0; i < data.size(); ++i) {
        char ch = data[i];
        
        // Check if we're in the middle of parsing an escape sequence
        if (m_parsingEscapeSequence) {
            m_escapeBuffer.append(ch);
            
            // Check if the sequence is complete
            if (isEscapeSequenceFinal(ch)) {
                // Process the entire escape sequence
                processEscapeSequence(m_escapeBuffer);
                
                // Reset for next sequence
                m_escapeBuffer.clear();
                m_parsingEscapeSequence = false;
            }
            continue;
        }
        
        // Handle special control characters
        switch (ch) {
            case '\033': // ESC
                // Start of an escape sequence
                m_escapeBuffer.clear();
                m_escapeBuffer.append(ch);
                m_parsingEscapeSequence = true;
                break;
                
            case '\b': // Backspace
                // Move cursor left
                if (m_cursorPosition.x() > 0) {
                    setCursorPositionInternal(m_cursorPosition.x() - 1, m_cursorPosition.y());
                }
                break;
                
            case '\t': // Tab
                // Move to next tab stop (every 8 columns)
                {
                    int newX = (m_cursorPosition.x() + 8) & ~7;
                    if (newX >= m_terminalSize.width()) {
                        newX = m_terminalSize.width() - 1;
                    }
                    setCursorPositionInternal(newX, m_cursorPosition.y());
                }
                break;
                
            case '\r': // Carriage return
                // Move to beginning of line
                setCursorPositionInternal(0, m_cursorPosition.y());
                break;
                
            case '\n': // Line feed
                // Move to next line
                if (m_newLineMode) {
                    // In newline mode, LF also implies CR
                    setCursorPositionInternal(0, m_cursorPosition.y() + 1);
                } else {
                    // Just go down one line
                    setCursorPositionInternal(m_cursorPosition.x(), m_cursorPosition.y() + 1);
                }
                
                // Check if we need to scroll
                if (m_cursorPosition.y() > m_scrollRegionBottom) {
                    scrollScreen(1);
                    setCursorPositionInternal(m_cursorPosition.x(), m_scrollRegionBottom);
                }
                break;
                
            case '\a': // Bell
                // Trigger terminal bell
                emit bellTriggered();
                break;
                
            case '\f': // Form feed (clear screen)
                // Clear screen and reset cursor position
                clear();
                break;
                
            default:
                // Regular character - put it at the current cursor position
                putCharacter(QChar(ch));
                break;
        }
    }
    
    // Trigger redraw
    emit redrawRequired();
}

int TerminalEmulator::processEscapeSequence(const QByteArray &sequence)
{
    if (sequence.isEmpty()) {
        return 0;
    }
    
    // Check the sequence type
    if (sequence.length() >= 2 && sequence[0] == '\033') {
        switch (sequence[1]) {
            case '[': // CSI - Control Sequence Introducer
                return processCSI(sequence);
                
            case ']': // OSC - Operating System Command
                return processOSC(sequence);
                
            case 'D': // IND - Index (line feed)
                setCursorPositionInternal(m_cursorPosition.x(), m_cursorPosition.y() + 1);
                return 2;
                
            case 'M': // RI - Reverse Index
                setCursorPositionInternal(m_cursorPosition.x(), m_cursorPosition.y() - 1);
                return 2;
                
            case 'E': // NEL - Next Line
                setCursorPositionInternal(0, m_cursorPosition.y() + 1);
                return 2;
                
            case 'c': // RIS - Reset to Initial State
                clear();
                m_currentFormat = TerminalCharFormat();
                setCursorPositionInternal(0, 0);
                return 2;
                
            case '7': // DECSC - Save Cursor
                // Save cursor position and attributes
                // (simple implementation for now)
                m_savedCursorPosition = m_cursorPosition;
                m_savedFormat = m_currentFormat;
                return 2;
                
            case '8': // DECRC - Restore Cursor
                // Restore cursor position and attributes
                m_cursorPosition = m_savedCursorPosition;
                m_currentFormat = m_savedFormat;
                return 2;
                
            default:
                // Unsupported or invalid escape sequence
                qDebug() << "Unsupported escape sequence:" << sequence.toHex();
                return 2; // Skip ESC + the next character
        }
    }
    
    // Default: skip the escape character
    return 1;
}

int TerminalEmulator::processCSI(const QByteArray &sequence)
{
    // CSI sequences are of the form: ESC [ <parameters> <final_byte>
    // First, find the final byte (a letter)
    
    int finalBytePos = 2; // Start after "ESC ["
    while (finalBytePos < sequence.length() && 
          !((sequence[finalBytePos] >= 'A' && sequence[finalBytePos] <= 'Z') || 
            (sequence[finalBytePos] >= 'a' && sequence[finalBytePos] <= 'z'))) {
        finalBytePos++;
    }
    
    if (finalBytePos >= sequence.length()) {
        // Incomplete sequence
        return 0;
    }
    
    // Get the final byte
    char finalByte = sequence[finalBytePos];
    
    // Parse parameters
    QList<int> parameters = parseParameters(sequence, 2, finalBytePos - 2);
    
    // Handle the sequence based on the final byte
    switch (finalByte) {
        case 'A': // CUU - Cursor Up
        {
            int n = parameters.isEmpty() ? 1 : parameters[0];
            if (n < 1) n = 1;
            setCursorPositionInternal(m_cursorPosition.x(), m_cursorPosition.y() - n);
            break;
        }
        
        case 'B': // CUD - Cursor Down
        {
            int n = parameters.isEmpty() ? 1 : parameters[0];
            if (n < 1) n = 1;
            setCursorPositionInternal(m_cursorPosition.x(), m_cursorPosition.y() + n);
            break;
        }
        
        case 'C': // CUF - Cursor Forward
        {
            int n = parameters.isEmpty() ? 1 : parameters[0];
            if (n < 1) n = 1;
            setCursorPositionInternal(m_cursorPosition.x() + n, m_cursorPosition.y());
            break;
        }
        
        case 'D': // CUB - Cursor Backward
        {
            int n = parameters.isEmpty() ? 1 : parameters[0];
            if (n < 1) n = 1;
            setCursorPositionInternal(m_cursorPosition.x() - n, m_cursorPosition.y());
            break;
        }
        
        case 'E': // CNL - Cursor Next Line
        {
            int n = parameters.isEmpty() ? 1 : parameters[0];
            if (n < 1) n = 1;
            setCursorPositionInternal(0, m_cursorPosition.y() + n);
            break;
        }
        
        case 'F': // CPL - Cursor Previous Line
        {
            int n = parameters.isEmpty() ? 1 : parameters[0];
            if (n < 1) n = 1;
            setCursorPositionInternal(0, m_cursorPosition.y() - n);
            break;
        }
        
        case 'G': // CHA - Cursor Horizontal Absolute
        {
            int n = parameters.isEmpty() ? 1 : parameters[0];
            if (n < 1) n = 1;
            setCursorPositionInternal(n - 1, m_cursorPosition.y());
            break;
        }
        
        case 'H': // CUP - Cursor Position
        case 'f': // HVP - Horizontal and Vertical Position
        {
            int row = parameters.isEmpty() ? 1 : parameters[0];
            int col = parameters.size() < 2 ? 1 : parameters[1];
            if (row < 1) row = 1;
            if (col < 1) col = 1;
            setCursorPositionInternal(col - 1, row - 1);
            break;
        }
        
        case 'J': // ED - Erase Display
        {
            int mode = parameters.isEmpty() ? 0 : parameters[0];
            QVector<TerminalLine> &activeScreen = m_alternateScreenActive ? m_alternateScreen : m_screen;
            
            switch (mode) {
                case 0: // From cursor to end of screen
                {
                    // Clear current line from cursor to end
                    int currentX = m_cursorPosition.x();
                    int currentY = m_cursorPosition.y();
                    
                    if (currentY < activeScreen.size()) {
                        TerminalLine &line = activeScreen[currentY];
                        for (int i = currentX; i < line.size(); ++i) {
                            line[i] = TerminalCell(' ', m_currentFormat);
                        }
                        
                        // Clear all lines below
                        for (int y = currentY + 1; y < activeScreen.size(); ++y) {
                            TerminalLine &clearLine = activeScreen[y];
                            for (int x = 0; x < clearLine.size(); ++x) {
                                clearLine[x] = TerminalCell(' ', m_currentFormat);
                            }
                        }
                    }
                    break;
                }
                
                case 1: // From start of screen to cursor
                {
                    // Clear all lines above
                    int currentY = m_cursorPosition.y();
                    
                    for (int y = 0; y < currentY && y < activeScreen.size(); ++y) {
                        TerminalLine &clearLine = activeScreen[y];
                        for (int x = 0; x < clearLine.size(); ++x) {
                            clearLine[x] = TerminalCell(' ', m_currentFormat);
                        }
                    }
                    
                    // Clear current line up to cursor
                    if (currentY < activeScreen.size()) {
                        TerminalLine &line = activeScreen[currentY];
                        int currentX = qMin(m_cursorPosition.x(), line.size() - 1);
                        
                        for (int i = 0; i <= currentX; ++i) {
                            line[i] = TerminalCell(' ', m_currentFormat);
                        }
                    }
                    break;
                }
                
                case 2: // Entire screen
                case 3: // Entire screen + scrollback (treat same as 2 for now)
                    for (int y = 0; y < activeScreen.size(); ++y) {
                        TerminalLine &clearLine = activeScreen[y];
                        for (int x = 0; x < clearLine.size(); ++x) {
                            clearLine[x] = TerminalCell(' ', m_currentFormat);
                        }
                    }
                    break;
            }
            break;
        }
        
        case 'K': // EL - Erase in Line
        {
            int mode = parameters.isEmpty() ? 0 : parameters[0];
            QVector<TerminalLine> &activeScreen = m_alternateScreenActive ? m_alternateScreen : m_screen;
            int currentY = m_cursorPosition.y();
            
            if (currentY < 0 || currentY >= activeScreen.size()) {
                break;
            }
            
            TerminalLine &line = activeScreen[currentY];
            int currentX = m_cursorPosition.x();
            
            switch (mode) {
                case 0: // From cursor to end of line
                    for (int i = currentX; i < line.size(); ++i) {
                        line[i] = TerminalCell(' ', m_currentFormat);
                    }
                    break;
                    
                case 1: // From start of line to cursor
                    for (int i = 0; i <= currentX && i < line.size(); ++i) {
                        line[i] = TerminalCell(' ', m_currentFormat);
                    }
                    break;
                    
                case 2: // Entire line
                    for (int i = 0; i < line.size(); ++i) {
                        line[i] = TerminalCell(' ', m_currentFormat);
                    }
                    break;
            }
            break;
        }
        
        case 'm': // SGR - Select Graphic Rendition
            processSGR(parameters);
            break;
            
        case 'r': // DECSTBM - Set Top and Bottom Margins
        {
            // Set scrolling region
            int top = parameters.isEmpty() ? 1 : parameters[0];
            int bottom = parameters.size() < 2 ? m_terminalSize.height() : parameters[1];
            
            // Convert from 1-based to 0-based
            top = qMax(1, top) - 1;
            bottom = qMin(m_terminalSize.height(), bottom) - 1;
            
            // Ensure top < bottom
            if (top < bottom) {
                m_scrollRegionTop = top;
                m_scrollRegionBottom = bottom;
                
                // Home cursor
                setCursorPositionInternal(0, top);
            }
            break;
        }
        
        case 's': // SCP - Save Cursor Position
            m_savedCursorPosition = m_cursorPosition;
            m_savedFormat = m_currentFormat;
            break;
            
        case 'u': // RCP - Restore Cursor Position
            m_cursorPosition = m_savedCursorPosition;
            m_currentFormat = m_savedFormat;
            break;
            
        case 'h': // Set Mode
        case 'l': // Reset Mode
        {
            bool set = (finalByte == 'h');
            
            // Check for ? prefix for DEC private modes
            bool isPrivateMode = sequence.length() > 2 && sequence[2] == '?';
            
            // Get the mode number
            int modeOffset = isPrivateMode ? 3 : 2;
            QList<int> modeParams = parseParameters(sequence, modeOffset, finalBytePos - modeOffset);
            
            if (modeParams.isEmpty()) {
                break;
            }
            
            // Handle modes
            for (int mode : modeParams) {
                if (isPrivateMode) {
                    // DEC Private modes
                    switch (mode) {
                        case 1: // DECCKM - Application Cursor Keys
                            m_applicationCursorKeys = set;
                            break;
                            
                        case 3: // DECCOLM - 80/132 Column Mode
                            // Not implemented yet
                            break;
                            
                        case 6: // DECOM - Origin Mode
                            // Not implemented yet
                            break;
                            
                        case 7: // DECAWM - Auto Wrap Mode
                            // Auto wrap is always on for now
                            break;
                            
                        case 25: // DECTCEM - Text Cursor Enable Mode
                            m_cursorVisible = set;
                            break;
                            
                        case 47: // Alternate Screen Buffer
                        case 1047:
                            if (set != m_alternateScreenActive) {
                                m_alternateScreenActive = set;
                                // Reset cursor position when switching screens
                                setCursorPositionInternal(0, 0);
                            }
                            break;
                            
                        case 1048: // Save/Restore cursor position
                            if (set) {
                                m_savedCursorPosition = m_cursorPosition;
                                m_savedFormat = m_currentFormat;
                            } else {
                                m_cursorPosition = m_savedCursorPosition;
                                m_currentFormat = m_savedFormat;
                            }
                            break;
                            
                        case 1049: // Alternate Screen + Save Cursor
                            if (set) {
                                // Save cursor position
                                m_savedCursorPosition = m_cursorPosition;
                                m_savedFormat = m_currentFormat;
                                
                                // Switch to alternate screen
                                m_alternateScreenActive = true;
                                
                                // Reset cursor position
                                setCursorPositionInternal(0, 0);
                            } else {
                                // Switch back to normal screen
                                m_alternateScreenActive = false;
                                
                                // Restore cursor position
                                m_cursorPosition = m_savedCursorPosition;
                                m_currentFormat = m_savedFormat;
                            }
                            break;
                            
                        case 2004: // Bracketed paste mode
                            m_bracketedPasteMode = set;
                            break;
                    }
                } else {
                    // Standard modes
                    switch (mode) {
                        case 4: // IRM - Insert Mode
                            // Not implemented yet
                            break;
                            
                        case 20: // LNM - Line Feed/New Line Mode
                            m_newLineMode = set;
                            break;
                    }
                }
            }
            break;
        }
        
        default:
            qDebug() << "Unhandled CSI sequence:" << QByteArray(sequence.data(), finalBytePos + 1).toHex();
            break;
    }
    
    return finalBytePos + 1;
}

int TerminalEmulator::processOSC(const QByteArray &sequence)
{
    // OSC sequences are of the form: ESC ] <parameters> BEL or ESC ] <parameters> ESC \
    
    // Find the end of the sequence (BEL or ST)
    int endPos = 2; // Start after "ESC ]"
    while (endPos < sequence.length()) {
        if (sequence[endPos] == '\007') { // BEL
            break;
        } else if (endPos + 1 < sequence.length() && sequence[endPos] == '\033' && sequence[endPos + 1] == '\\') { // ST
            endPos++; // Include the backslash
            break;
        }
        endPos++;
    }
    
    if (endPos >= sequence.length()) {
        // Incomplete sequence
        return 0;
    }
    
    // Get the parameter string
    QByteArray paramString = sequence.mid(2, endPos - 2);
    
    // Find the first semicolon
    int semicolonPos = paramString.indexOf(';');
    if (semicolonPos < 0) {
        // No semicolon, can't parse
        return endPos + 1;
    }
    
    // Get the command number and the parameter
    bool ok;
    int cmdNum = paramString.left(semicolonPos).toInt(&ok);
    if (!ok) {
        return endPos + 1;
    }
    
    QString param = QString::fromUtf8(paramString.mid(semicolonPos + 1));
    
    // Handle OSC commands
    switch (cmdNum) {
        case 0: // Set window title and icon name
        case 2: // Set window title
            m_terminalTitle = param;
            emit titleChanged(m_terminalTitle);
            break;
            
        case 1: // Set icon name
            // Not implemented
            break;
            
        case 4: // Set color
            // Not implemented yet
            break;
            
        case 7: // Set current directory for shell integration
            m_workingDirectory = param;
            emit workingDirectoryChanged(m_workingDirectory);
            break;
            
        default:
            qDebug() << "Unhandled OSC sequence:" << cmdNum << param;
            break;
    }
    
    return endPos + 1;
}

void TerminalEmulator::processSGR(const QList<int> &parameters)
{
    // If no parameters, reset attributes
    if (parameters.isEmpty()) {
        m_currentFormat = TerminalCharFormat();
        m_currentFormat.foreground = m_defaultForeground;
        m_currentFormat.background = m_defaultBackground;
        return;
    }
    
    // Process each parameter
    for (int i = 0; i < parameters.size(); ++i) {
        int param = parameters[i];
        
        switch (param) {
            case 0: // Reset all attributes
                m_currentFormat = TerminalCharFormat();
                m_currentFormat.foreground = m_defaultForeground;
                m_currentFormat.background = m_defaultBackground;
                break;
                
            case 1: // Bold
                m_currentFormat.attributes |= Bold;
                break;
                
            case 2: // Dim
                m_currentFormat.attributes |= Dim;
                break;
                
            case 3: // Italic
                m_currentFormat.attributes |= Italic;
                break;
                
            case 4: // Underline
                m_currentFormat.attributes |= Underline;
                break;
                
            case 5: // Blink (slow)
            case 6: // Blink (rapid)
                m_currentFormat.attributes |= Blink;
                break;
                
            case 7: // Reverse video
                m_currentFormat.attributes |= Reverse;
                break;
                
            case 8: // Invisible
                m_currentFormat.attributes |= Invisible;
                break;
                
            case 9: // Strikethrough
                m_currentFormat.attributes |= StrikeThrough;
                break;
                
            case 21: // Double underline (or not bold)
                // Not implemented
                break;
                
            case 22: // Not bold and not dim
                m_currentFormat.attributes &= ~(Bold | Dim);
                break;
                
            case 23: // Not italic
                m_currentFormat.attributes &= ~Italic;
                break;
                
            case 24: // Not underlined
                m_currentFormat.attributes &= ~Underline;
                break;
                
            case 25: // Not blinking
                m_currentFormat.attributes &= ~Blink;
                break;
                
            case 27: // Not reverse
                m_currentFormat.attributes &= ~Reverse;
                break;
                
            case 28: // Not invisible
                m_currentFormat.attributes &= ~Invisible;
                break;
                
            case 29: // Not strikethrough
                m_currentFormat.attributes &= ~StrikeThrough;
                break;
                
            case 30: // Foreground Black
            case 31: // Foreground Red
            case 32: // Foreground Green
            case 33: // Foreground Yellow
            case 34: // Foreground Blue
            case 35: // Foreground Magenta
            case 36: // Foreground Cyan
            case 37: // Foreground White
                m_currentFormat.foreground = m_colorPalette[param - 30];
                break;
                
            case 38: // Extended foreground color
                if (i + 2 < parameters.size() && parameters[i + 1] == 5) {
                    // 8-bit color (256 colors)
                    int colorCode = parameters[i + 2];
                    if (m_colorPalette.contains(colorCode)) {
                        m_currentFormat.foreground = m_colorPalette[colorCode];
                    } else {
                        // Compute extended color if not in palette
                        if (colorCode < 16) {
                            // Standard colors
                            m_currentFormat.foreground = m_colorPalette[colorCode];
                        } else if (colorCode < 232) {
                            // 6x6x6 color cube
                            int colorIndex = colorCode - 16;
                            int r = (colorIndex / 36) * 51;
                            int g = ((colorIndex / 6) % 6) * 51;
                            int b = (colorIndex % 6) * 51;
                            m_currentFormat.foreground = QColor(r, g, b);
                        } else {
                            // 24 grayscale ramp
                            int grayLevel = (colorCode - 232) * 11;
                            m_currentFormat.foreground = QColor(grayLevel, grayLevel, grayLevel);
                        }
                    }
                    i += 2; // Skip the next two parameters
                } else if (i + 4 < parameters.size() && parameters[i + 1] == 2) {
                    // 24-bit color (RGB)
                    int r = parameters[i + 2];
                    int g = parameters[i + 3];
                    int b = parameters[i + 4];
                    m_currentFormat.foreground = QColor(r, g, b);
                    i += 4; // Skip the next four parameters
                }
                break;
                
            case 39: // Default foreground color
                m_currentFormat.foreground = m_defaultForeground;
                break;
                
            case 40: // Background Black
            case 41: // Background Red
            case 42: // Background Green
            case 43: // Background Yellow
            case 44: // Background Blue
            case 45: // Background Magenta
            case 46: // Background Cyan
            case 47: // Background White
                m_currentFormat.background = m_colorPalette[param - 40];
                break;
                
            case 48: // Extended background color
                if (i + 2 < parameters.size() && parameters[i + 1] == 5) {
                    // 8-bit color (256 colors)
                    int colorCode = parameters[i + 2];
                    if (m_colorPalette.contains(colorCode)) {
                        m_currentFormat.background = m_colorPalette[colorCode];
                    } else {
                        // Compute extended color if not in palette
                        if (colorCode < 16) {
                            // Standard colors
                            m_currentFormat.background = m_colorPalette[colorCode];
                        } else if (colorCode < 232) {
                            // 6x6x6 color cube
                            int colorIndex = colorCode - 16;
                            int r = (colorIndex / 36) * 51;
                            int g = ((colorIndex / 6) % 6) * 51;
                            int b = (colorIndex % 6) * 51;
                            m_currentFormat.background = QColor(r, g, b);
                        } else {
                            // 24 grayscale ramp
                            int grayLevel = (colorCode - 232) * 11;
                            m_currentFormat.background = QColor(grayLevel, grayLevel, grayLevel);
                        }
                    }
                    i += 2; // Skip the next two parameters
                } else if (i + 4 < parameters.size() && parameters[i + 1] == 2) {
                    // 24-bit color (RGB)
                    int r = parameters[i + 2];
                    int g = parameters[i + 3];
                    int b = parameters[i + 4];
                    m_currentFormat.background = QColor(r, g, b);
                    i += 4; // Skip the next four parameters
                }
                break;
                
            case 49: // Default background color
                m_currentFormat.background = m_defaultBackground;
                break;
                
            case 90: // Bright foreground black
            case 91: // Bright foreground red
            case 92: // Bright foreground green
            case 93: // Bright foreground yellow
            case 94: // Bright foreground blue
            case 95: // Bright foreground magenta
            case 96: // Bright foreground cyan
            case 97: // Bright foreground white
                m_currentFormat.foreground = m_colorPalette[param - 90 + 8];
                break;
                
            case 100: // Bright background black
            case 101: // Bright background red
            case 102: // Bright background green
            case 103: // Bright background yellow
            case 104: // Bright background blue
            case 105: // Bright background magenta
            case 106: // Bright background cyan
            case 107: // Bright background white
                m_currentFormat.background = m_colorPalette[param - 100 + 8];
                break;
                
            default:
                qDebug() << "Unhandled SGR parameter:" << param;
                break;
        }
    }
}

void TerminalEmulator::putCharacter(QChar ch)
{
    // Get the active screen buffer
    QVector<TerminalLine> &activeScreen = m_alternateScreenActive ? m_alternateScreen : m_screen;
    
    // Ensure cursor is within bounds
    if (m_cursorPosition.y() < 0 || m_cursorPosition.y() >= activeScreen.size()) {
        return;
    }
    
    // Get the current line
    TerminalLine &currentLine = activeScreen[m_cursorPosition.y()];
    
    // Ensure the line is wide enough
    if (m_cursorPosition.x() >= currentLine.size()) {
        // Extend the line with spaces
        while (currentLine.size() <= m_cursorPosition.x()) {
            currentLine.append(TerminalCell(' ', m_currentFormat));
        }
    }
    
    // Put the character at the cursor position
    currentLine[m_cursorPosition.x()] = TerminalCell(ch, m_currentFormat);
    
    // Move cursor to the next position
    if (m_cursorPosition.x() + 1 >= m_terminalSize.width()) {
        // Wrap to next line
        setCursorPositionInternal(0, m_cursorPosition.y() + 1);
        
        // Check if we need to scroll
        if (m_cursorPosition.y() > m_scrollRegionBottom) {
            scrollScreen(1);
            setCursorPositionInternal(0, m_scrollRegionBottom);
        }
    } else {
        // Move one character to the right
        setCursorPositionInternal(m_cursorPosition.x() + 1, m_cursorPosition.y());
    }
}

void TerminalEmulator::moveCursor(int dx, int dy)
{
    // Move cursor relative to current position
    setCursorPositionInternal(m_cursorPosition.x() + dx, m_cursorPosition.y() + dy);
}

void TerminalEmulator::setCursorPositionInternal(int x, int y, bool clampToScreen)
{
    // Get screen dimensions
    int width = m_terminalSize.width();
    int height = m_terminalSize.height();
    
    // Clamp coordinates if requested
    if (clampToScreen) {
        x = qMax(0, qMin(x, width - 1));
        y = qMax(0, qMin(y, height - 1));
    }
    
    // Update cursor position
    QPoint oldPos = m_cursorPosition;
    m_cursorPosition = QPoint(x, y);
    
    // If position changed, emit signal
    if (oldPos != m_cursorPosition) {
        emit cursorPositionChanged(m_cursorPosition);
    }
}

void TerminalEmulator::scrollScreen(int lines)
{
    // Get the active screen buffer
    QVector<TerminalLine> &activeScreen = m_alternateScreenActive ? m_alternateScreen : m_screen;
    
    if (lines == 0) {
        return;
    }
    
    // Scroll up (positive lines) or down (negative lines)
    if (lines > 0) {
        // Scroll up - remove lines from top, add new lines at bottom
        for (int i = 0; i < lines; ++i) {
            if (m_scrollRegionTop < m_scrollRegionBottom) {
                // Remove line from the top of the scroll region
                activeScreen.removeAt(m_scrollRegionTop);
                
                // Add a new blank line at the bottom of the scroll region
                activeScreen.insert(m_scrollRegionBottom, createBlankLine());
            }
        }
    } else {
        // Scroll down - remove lines from bottom, add new lines at top
        lines = -lines; // Make positive for loop
        for (int i = 0; i < lines; ++i) {
            if (m_scrollRegionTop < m_scrollRegionBottom) {
                // Remove line from the bottom of the scroll region
                activeScreen.removeAt(m_scrollRegionBottom);
                
                // Add a new blank line at the top of the scroll region
                activeScreen.insert(m_scrollRegionTop, createBlankLine());
            }
        }
    }
}

TerminalLine TerminalEmulator::createBlankLine() const
{
    // Create a new line filled with spaces
    TerminalLine line;
    line.reserve(m_terminalSize.width());
    
    for (int i = 0; i < m_terminalSize.width(); ++i) {
        line.append(TerminalCell(' ', m_currentFormat));
    }
    
    return line;
}

QList<int> TerminalEmulator::parseParameters(const QByteArray &sequence, int start, int length)
{
    QList<int> result;
    
    // Adjust length if needed
    length = qMin(length, sequence.length() - start);
    if (length <= 0) {
        return result;
    }
    
    // Extract parameter substring
    QByteArray params = sequence.mid(start, length);
    
    // Split by semicolons
    QList<QByteArray> paramList = params.split(';');
    
    // Convert to integers
    for (const QByteArray &param : paramList) {
        bool ok;
        int value = param.toInt(&ok);
        
        // If conversion failed, use default value 0
        if (!ok) {
            value = 0;
        }
        
        result.append(value);
    }
    
    return result;
}

bool TerminalEmulator::isEscapeSequenceFinal(char ch) const
{
    // Control sequence final bytes are in the range 0x40-0x7E
    return (ch >= 0x40 && ch <= 0x7E);
}

void TerminalEmulator::clear()
{
    // Clear the active screen
    QVector<TerminalLine> &activeScreen = m_alternateScreenActive ? m_alternateScreen : m_screen;
    
    // Fill with blank lines
    for (int i = 0; i < activeScreen.size(); ++i) {
        TerminalLine &line = activeScreen[i];
        for (int j = 0; j < line.size(); ++j) {
            line[j] = TerminalCell(' ', m_currentFormat);
        }
    }
    
    // Reset cursor position
    setCursorPositionInternal(0, 0);
}

// Accessor Methods

QSize TerminalEmulator::size() const
{
    return m_terminalSize;
}

QChar TerminalEmulator::characterAt(int x, int y) const
{
    // Get the active screen buffer
    const QVector<TerminalLine> &activeScreen = m_alternateScreenActive ? m_alternateScreen : m_screen;
    
    // Check bounds
    if (y < 0 || y >= activeScreen.size() || x < 0 || x >= m_terminalSize.width()) {
        return QChar();
    }
    
    const TerminalLine &line = activeScreen[y];
    
    // Check if the position is within the line
    if (x >= line.size()) {
        return QChar(' ');
    }
    
    return line[x].character;
}

TerminalCharFormat TerminalEmulator::formatAt(int x, int y) const
{
    // Get the active screen buffer
    const QVector<TerminalLine> &activeScreen = m_alternateScreenActive ? m_alternateScreen : m_screen;
    
    // Check bounds
    if (y < 0 || y >= activeScreen.size() || x < 0 || x >= m_terminalSize.width()) {
        return TerminalCharFormat();
    }
    
    const TerminalLine &line = activeScreen[y];
    
    // Check if the position is within the line
    if (x >= line.size()) {
        return m_currentFormat;
    }
    
    return line[x].format;
}

QPoint TerminalEmulator::cursorPosition() const
{
    return m_cursorPosition;
}

CursorStyle TerminalEmulator::cursorStyle() const
{
    return m_cursorStyle;
}

void TerminalEmulator::setCursorStyle(CursorStyle style)
{
    m_cursorStyle = style;
    emit redrawRequired();
}

bool TerminalEmulator::isCursorVisible() const
{
    return m_cursorVisible;
}

void TerminalEmulator::setCursorVisible(bool visible)
{
    m_cursorVisible = visible;
    emit redrawRequired();
}

bool TerminalEmulator::isAlternateScreenActive() const
{
    return m_alternateScreenActive;

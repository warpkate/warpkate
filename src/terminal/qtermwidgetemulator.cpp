/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "qtermwidgetemulator.h"

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDir>
#include <QRegularExpression>
#include <QTimer>
#include <QTemporaryFile>
#include <QTextStream>
#include <QTextCodec>

// Include QTermWidget
#include <qtermwidget.h>

QTermWidgetEmulator::QTermWidgetEmulator(QWidget *parent)
    : TerminalEmulator(parent)
    , m_termWidget(new QTermWidget(parent))
    , m_lastExitCode(0)
    , m_commandExecuting(false)
    , m_initialized(false)
    , m_busy(false)
    , m_blockModeEnabled(true)
    , m_currentBlockId(0)
{
    // Set up the QTermWidget with sensible defaults
    m_termWidget->setScrollBarPosition(QTermWidget::ScrollBarRight);
    m_termWidget->setColorScheme("Linux");
    m_termWidget->setBlinkingCursor(true);
    m_termWidget->setTerminalFont(QFont("Monospace", 10));
    m_termWidget->setHistorySize(5000);
    m_termWidget->setMotionAfterPasting(2); // Paste and move cursor to end

    // Working directory defaults to user's home
    m_workingDirectory = QDir::homePath();

    // Connect QTermWidget signals to our slots
    connect(m_termWidget, &QTermWidget::receivedData, this, &QTermWidgetEmulator::processTerminalOutput);
    connect(m_termWidget, &QTermWidget::titleChanged, this, &QTermWidgetEmulator::handleTitleChanged);
    connect(m_termWidget, &QTermWidget::finished, this, &QTermWidgetEmulator::handleFinished);
    connect(m_termWidget, &QTermWidget::bellSignal, this, &QTermWidgetEmulator::handleBell);

    // Set up a timer to periodically check for commands and working directory
    QTimer *detectionTimer = new QTimer(this);
    connect(detectionTimer, &QTimer::timeout, this, &QTermWidgetEmulator::detectCommand);
    connect(detectionTimer, &QTimer::timeout, this, &QTermWidgetEmulator::detectWorkingDirectory);
    detectionTimer->start(1000); // Check every second
}

QTermWidgetEmulator::~QTermWidgetEmulator()
{
    // QTermWidget will be destroyed by its parent
}

bool QTermWidgetEmulator::initialize(int rows, int cols)
{
    if (m_initialized) {
        return true;
    }

    // Set initial size
    m_termWidget->setSize(QSize(cols, rows));
    
    m_initialized = true;
    return true;
}

bool QTermWidgetEmulator::startShell(const QString &shellCommand, const QString &initialWorkingDirectory)
{
    // Set working directory if provided
    if (!initialWorkingDirectory.isEmpty()) {
        m_workingDirectory = initialWorkingDirectory;
    }

    // Get shell command or use system default
    QString shell = shellCommand;
    if (shell.isEmpty()) {
        shell = QString::fromUtf8(qgetenv("SHELL"));
        if (shell.isEmpty()) {
            shell = QStringLiteral("/bin/bash");
        }
    }

    // Start the terminal with the specified shell
    m_termWidget->startShellProgram();
    
    // Flag as busy once shell is started
    m_busy = true;
    
    return true;
}

void QTermWidgetEmulator::resize(int rows, int cols)
{
    // QTermWidget handles resizing internally, just update the size
    m_termWidget->setSize(QSize(cols, rows));
    Q_EMIT sizeChanged(QSize(cols, rows));
}

void QTermWidgetEmulator::processInput(const QString &text)
{
    // Send the text to the terminal
    if (m_busy) {
        m_termWidget->sendText(text);
    }
}

void QTermWidgetEmulator::processKeyPress(int key, Qt::KeyboardModifiers modifiers, const QString &text)
{
    // For this implementation, we'll rely on QTermWidget's own key event handling
    // by letting the parent widget send the key events directly to the QTermWidget
    Q_UNUSED(key);
    Q_UNUSED(modifiers);
    Q_UNUSED(text);
}

void QTermWidgetEmulator::executeCommand(const QString &command, bool addNewline)
{
    if (!m_busy) {
        return;
    }

    // Record command start
    m_currentCommand = command;
    m_commandExecuting = true;
    m_commandStartTime = QDateTime::currentDateTime();
    m_currentOutput.clear();

    // Send the command to the terminal
    m_termWidget->sendText(command);
    
    // Add newline if requested
    if (addNewline) {
        m_termWidget->sendText("\r");
    }

    // Update block if block mode is enabled
    if (m_blockModeEnabled) {
        m_currentBlockId++;
        // Notify that a new command was started
        Q_EMIT commandDetected(command);
    }
}

void QTermWidgetEmulator::clear()
{
    m_termWidget->clear();
}

QSize QTermWidgetEmulator::size() const
{
    return m_termWidget->size();
}

QString QTermWidgetEmulator::currentWorkingDirectory() const
{
    return m_workingDirectory;
}

bool QTermWidgetEmulator::isBusy() const
{
    return m_busy;
}

int QTermWidgetEmulator::lastExitCode() const
{
    return m_lastExitCode;
}

QString QTermWidgetEmulator::currentCommand() const
{
    return m_currentCommand;
}

QStringList QTermWidgetEmulator::commandHistory() const
{
    return m_commandHistory;
}

QTermWidget* QTermWidgetEmulator::termWidget() const
{
    return m_termWidget;
}

void QTermWidgetEmulator::copyToClipboard()
{
    m_termWidget->copyClipboard();
}

void QTermWidgetEmulator::pasteFromClipboard()
{
    m_termWidget->pasteClipboard();
}

void QTermWidgetEmulator::selectAll()
{
    m_termWidget->selectAll();
}

bool QTermWidgetEmulator::findText(const QString &text, bool caseSensitive, bool searchForward)
{
    return m_termWidget->search(text, searchForward, caseSensitive);
}

void QTermWidgetEmulator::processTerminalOutput()
{
    // This slot is called when QTermWidget has new output
    
    // Get the displayed text to analyze
    QString text = m_termWidget->getSelectedText();
    
    // Accumulate output if a command is executing
    if (m_commandExecuting) {
        m_currentOutput.append(text);
    }
    
    // Emit signal for raw output
    Q_EMIT outputAvailable(text);
    Q_EMIT redrawRequired();
}

void QTermWidgetEmulator::handleTitleChanged(const QString &title)
{
    Q_EMIT titleChanged(title);
}

void QTermWidgetEmulator::handleFinished()
{
    m_busy = false;
    Q_EMIT shellFinished(m_lastExitCode);
}

void QTermWidgetEmulator::handleBell()
{
    Q_EMIT bellTriggered();
}

void QTermWidgetEmulator::detectCommand()
{
    if (!m_busy) {
        return;
    }

    // We need to run a command to check the last command status
    // This will be done by creating a temporary script and executing it
    
    QTemporaryFile tempFile;
    tempFile.setAutoRemove(true);
    if (tempFile.open()) {
        QTextStream stream(&tempFile);
        
        // Write a script to get information about the current shell state
        stream << "#!/bin/bash\n";
        stream << "echo '--- WARPKATE_CMD_INFO_START ---'\n";
        stream << "echo \"EXITCODE=$?\"\n";
        stream << "echo \"PWD=$(pwd)\"\n";
        stream << "echo \"LAST_CMD=$(history 1 | awk '{$1=\"\"; print substr($0,2)}')\"\n";
        stream << "echo '--- WARPKATE_CMD_INFO_END ---'\n";
        
        stream.flush();
        tempFile.close();
        
        // Make the script executable
        QFile(tempFile.fileName()).setPermissions(
            QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
        
        // Run the script in the terminal
        QString scriptCmd = QString("source %1\r").arg(tempFile.fileName());
        
        // Save current command state
        bool wasExecuting = m_commandExecuting;
        QString currentCmd = m_currentCommand;
        QString currentOutput = m_currentOutput;
        
        // Temporarily disable command tracking to avoid interference
        m_commandExecuting = false;
        
        // Execute the info gathering script
        m_termWidget->sendText(scriptCmd);
        
        // Restore command state
        m_commandExecuting = wasExecuting;
        m_currentCommand = currentCmd;
        m_currentOutput = currentOutput;
    }
    
    // Check if the output from the script is in the terminal
    QString terminalText = m_termWidget->getScreenContents();
    
    QRegularExpression infoRegex(
        QStringLiteral("--- WARPKATE_CMD_INFO_START ---\\s*"
                      "EXITCODE=(\\d+)\\s*"
                      "PWD=([^\\n]*)\\s*"
                      "LAST_CMD=([^\\n]*)\\s*"
                      "--- WARPKATE_CMD_INFO_END ---"));
                      
    QRegularExpressionMatch match = infoRegex.match(terminalText);
    
    if (match.hasMatch()) {
        // Extract command information
        m_lastExitCode = match.captured(1).toInt();
        
        QString newWorkingDir = match.captured(2).trimmed();
        if (!newWorkingDir.isEmpty() && newWorkingDir != m_workingDirectory) {
            m_workingDirectory = newWorkingDir;
            Q_EMIT workingDirectoryChanged(m_workingDirectory);
        }
        
        QString lastCmd = match.captured(3).trimmed();
        if (!lastCmd.isEmpty() && m_commandExecuting) {
            // If we have a non-empty last command and we were tracking a command,
            // it means the command has completed
            m_commandExecuting = false;
            
            // Emit command executed signal
            Q_EMIT commandExecuted(m_currentCommand, m_currentOutput, m_lastExitCode);
            
            // Add to history if not already present
            if (!m_commandHistory.contains(m_currentCommand) && !m_currentCommand.isEmpty()) {
                m_commandHistory.append(m_currentCommand);
            }
            
            // Clear current output
            m_currentOutput.clear();
        }
    }
}

void QTermWidgetEmulator::detectWorkingDirectory()
{
    // Working directory detection is handled in detectCommand()
    // This is kept as a separate method for API compatibility
}

void QTermWidgetEmulator::detectExitCode()
{
    // Exit code detection is handled in detectCommand()
    // This is kept as a separate method for API compatibility
}


/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
 * This file contains the complete executeFile function for WarpKate.
 * This function safely executes files from the terminal with appropriate safeguards.
 */

#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>
#include <QTextCursor>
#include <QTextEdit>
#include <QUrl>

#include <KLocalizedString>

/**
 * Execute a file safely
 * @param filePath Full path to the file to execute
 */
void WarpKateView::executeFile(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    
    // Check if file exists and is executable
    if (!fileInfo.exists()) {
        qWarning() << "WarpKate: Cannot execute file (not found):" << filePath;
        
        // Add a notification to the conversation area
        QTextCursor cursor = m_conversationArea->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_conversationArea->setTextCursor(cursor);
        
        QTextCharFormat errorFormat;
        errorFormat.setForeground(QBrush(QColor(200, 0, 0))); // Red
        
        cursor.insertBlock();
        cursor.setCharFormat(errorFormat);
        cursor.insertText(i18n("Error: File does not exist: %1", filePath));
        cursor.setCharFormat(QTextCharFormat());
        
        m_conversationArea->ensureCursorVisible();
        return;
    }
    
    if (!fileInfo.isExecutable()) {
        qWarning() << "WarpKate: Cannot execute file (not executable):" << filePath;
        
        // Add a notification to the conversation area
        QTextCursor cursor = m_conversationArea->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_conversationArea->setTextCursor(cursor);
        
        QTextCharFormat errorFormat;
        errorFormat.setForeground(QBrush(QColor(200, 0, 0))); // Red
        
        cursor.insertBlock();
        cursor.setCharFormat(errorFormat);
        cursor.insertText(i18n("Error: File is not executable: %1", filePath));
        cursor.setCharFormat(QTextCharFormat());
        
        m_conversationArea->ensureCursorVisible();
        return;
    }
    
    // Show confirmation dialog
    QMessageBox::StandardButton confirmation = QMessageBox::question(
        m_mainWindow->window(),
        i18n("Execute File"),
        i18n("Are you sure you want to execute '%1'?", fileInfo.fileName()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (confirmation != QMessageBox::Yes) {
        return;
    }
    
    // Determine how to execute the file based on its type
    QString command;
    QString extension = fileInfo.suffix().toLower();
    
    // Check if this is a script that needs an interpreter
    if (extension == QStringLiteral("sh")) {
        command = QStringLiteral("bash \"%1\"").arg(filePath);
    } else if (extension == QStringLiteral("py")) {
        command = QStringLiteral("python3 \"%1\"").arg(filePath);
    } else if (extension == QStringLiteral("pl")) {
        command = QStringLiteral("perl \"%1\"").arg(filePath);
    } else if (extension == QStringLiteral("rb")) {
        command = QStringLiteral("ruby \"%1\"").arg(filePath);
    } else if (extension == QStringLiteral("js")) {
        command = QStringLiteral("node \"%1\"").arg(filePath);
    } else {
        // For other executable files, check if it's a binary or text file
        QFile file(filePath);
        bool isBinary = false;
        
        if (file.open(QIODevice::ReadOnly)) {
            // Read the first few bytes to check if it's binary
            QByteArray start = file.read(4096);
            file.close();
            
            // Check for NULL bytes or other binary content
            for (int i = 0; i < start.size(); i++) {
                if (start.at(i) == '\0') {
                    isBinary = true;
                    break;
                }
            }
        }
        
        if (isBinary) {
            // For binary files, execute directly, but be more cautious
            QMessageBox::StandardButton execConfirmation = QMessageBox::warning(
                m_mainWindow->window(),
                i18n("Execute Binary File"),
                i18n("'%1' appears to be a binary file. Are you sure you want to execute it?", fileInfo.fileName()),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No
            );
            
            if (execConfirmation != QMessageBox::Yes) {
                return;
            }
            
            command = QStringLiteral("\"%1\"").arg(filePath);
        } else {
            // For other text files, assume it might be a script and use bash
            command = QStringLiteral("bash \"%1\"").arg(filePath);
        }
    }
    
    // Log what we're executing
    qDebug() << "WarpKate: Executing file:" << filePath << "with command:" << command;
    
    // Check if we need to cd to the directory first
    QString workingDir = fileInfo.absolutePath();
    QString currentDir;
    
    if (m_terminalEmulator) {
        currentDir = m_terminalEmulator->currentWorkingDirectory();
    }
    
    if (!workingDir.isEmpty() && workingDir != currentDir) {
        // We need to change directory first
        command = QStringLiteral("cd \"%1\" && %2").arg(workingDir, command);
    }
    
    // Execute the command through our terminal
    if (m_terminalEmulator) {
        // Add a notification to the conversation area
        QTextCursor cursor = m_conversationArea->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_conversationArea->setTextCursor(cursor);
        
        QTextCharFormat infoFormat;
        infoFormat.setForeground(QBrush(QColor(0, 150, 0))); // Green
        
        cursor.insertBlock();
        cursor.setCharFormat(infoFormat);
        cursor.insertText(i18n("Executing: %1", command));
        cursor.setCharFormat(QTextCharFormat());
        
        m_conversationArea->ensureCursorVisible();
        
        // Execute the command
        executeCommand(command);
    } else {
        // If terminal emulator is not available, try using QProcess as fallback
        qWarning() << "WarpKate: Terminal emulator not available, using QProcess as fallback";
        
        QProcess *process = new QProcess(this);
        process->setWorkingDirectory(workingDir);
        
        // For process cleanup
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                process, &QProcess::deleteLater);
        
        // Start the process
        process->start(QStringLiteral("/bin/bash"), QStringList() << QStringLiteral("-c") << command);
        
        // Add a notification to the conversation area
        QTextCursor cursor = m_conversationArea->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_conversationArea->setTextCursor(cursor);
        
        QTextCharFormat infoFormat;
        infoFormat.setForeground(QBrush(QColor(0, 150, 0))); // Green
        
        cursor.insertBlock();
        cursor.setCharFormat(infoFormat);
        cursor.insertText(i18n("Executing (external): %1", command));
        cursor.setCharFormat(QTextCharFormat());
        
        m_conversationArea->ensureCursorVisible();
    }
}


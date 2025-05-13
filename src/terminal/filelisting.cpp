/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "filelisting.h"

#include <QDebug>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>
#include <QMimeDatabase>
#include <QMimeType>

FileListing::FileListing(QObject *parent)
    : QObject(parent)
    , m_mimeDb()  // Direct initialization
{
    // Initialize default terminal directory
    m_terminalDirectory = QDir::homePath();
    
    // Initialize list of common non-file words
    m_nonFileWords << QStringLiteral("total")
                  << QStringLiteral("ls")
                  << QStringLiteral("cd")
                  << QStringLiteral("grep")
                  << QStringLiteral("find");
}

FileListing::~FileListing()
{
    // No specific cleanup needed
}

QMenu* FileListing::createFileContextMenu(const QString &filePath, bool isDirectory)
{
    qDebug() << "FileListing: Creating context menu for" << filePath
             << "isDirectory:" << isDirectory;
    
    QMenu *menu = new QMenu();
    
    // Add common actions for both files and directories
    QAction *copyPathAction = menu->addAction(QIcon::fromTheme(QStringLiteral("edit-copy")), 
                                             QStringLiteral("Copy Path to Clipboard"));
    connect(copyPathAction, &QAction::triggered, this, [this, filePath]() {
        copyPathToClipboard(filePath);
    });
    
    menu->addSeparator();
    
    if (isDirectory) {
        // Directory-specific actions
        QAction *openDirAction = menu->addAction(QIcon::fromTheme(QStringLiteral("folder-open")), 
                                               QStringLiteral("Open in File Manager"));
        connect(openDirAction, &QAction::triggered, this, [this, filePath]() {
            openDirectory(filePath);
        });
        
        // Change directory action
        QAction *cdHereAction = menu->addAction(QIcon::fromTheme(QStringLiteral("utilities-terminal")), 
                                             QStringLiteral("Change Directory Here"));
        connect(cdHereAction, &QAction::triggered, this, [this, filePath]() {
            // Emit signal to execute cd command
            QString command = QStringLiteral("cd \"%1\"").arg(filePath);
            Q_EMIT executeCommand(command);
        });
    } else {
        // File-specific actions
        QAction *openFileAction = menu->addAction(QIcon::fromTheme(QStringLiteral("document-open")), 
                                                QStringLiteral("Open with Default Application"));
        connect(openFileAction, &QAction::triggered, this, [this, filePath]() {
            openFile(filePath);
        });
        
        QAction *openInKateAction = menu->addAction(QIcon::fromTheme(QStringLiteral("kate")), 
                                                  QStringLiteral("Open in Kate"));
        connect(openInKateAction, &QAction::triggered, this, [this, filePath]() {
            openFileInKate(filePath);
        });
        
        // Add execute action for executable files
        if (isExecutable(filePath)) {
            menu->addSeparator();
            QAction *executeAction = menu->addAction(QIcon::fromTheme(QStringLiteral("media-playback-start")), 
                                                  QStringLiteral("Execute"));
            connect(executeAction, &QAction::triggered, this, [this, filePath]() {
                executeFile(filePath);
            });
        }
    }
    
    return menu;
}

void FileListing::handleFileItemClicked(const QString &filePath, bool isDirectory)
{
    qDebug() << "FileListing: File item clicked:" << filePath
             << "isDirectory:" << isDirectory;
    
    // Default action based on file type
    if (isDirectory) {
        // For directories, open in file manager
        openDirectory(filePath);
    } else {
        // For files, detect type and decide what to do
        QString fileType = detectFileType(filePath);
        
        // Text files should open in Kate
        if (fileType.startsWith(QStringLiteral("text/")) || 
            fileType.contains(QStringLiteral("javascript")) ||
            fileType.contains(QStringLiteral("json")) || 
            fileType.contains(QStringLiteral("xml")) ||
            fileType.contains(QStringLiteral("html")) ||
            fileType.contains(QStringLiteral("css")) ||
            fileType.endsWith(QStringLiteral("/x-c")) ||
            fileType.endsWith(QStringLiteral("/x-c++")) ||
            fileType.endsWith(QStringLiteral("/x-python")) ||
            fileType.endsWith(QStringLiteral("/x-java"))) {
            
            openFileInKate(filePath);
        } else {
            // Non-text files open with default application
            openFile(filePath);
        }
    }
}

bool FileListing::openFile(const QString &filePath)
{
    qDebug() << "FileListing: Opening file:" << filePath;
    
    // Verify the file exists
    if (!fileExists(filePath)) {
        QString message = QStringLiteral("File does not exist: %1").arg(filePath);
        qWarning() << "FileListing:" << message;
        Q_EMIT operationComplete(message, false);
        return false;
    }
    
    // Use QDesktopServices to open the file with the default application
    QUrl fileUrl = QUrl::fromLocalFile(filePath);
    bool success = QDesktopServices::openUrl(fileUrl);
    
    if (!success) {
        QString message = QStringLiteral("Failed to open file: %1").arg(filePath);
        qWarning() << "FileListing:" << message;
        Q_EMIT operationComplete(message, false);
    } else {
        QString message = QStringLiteral("Opened file: %1").arg(filePath);
        Q_EMIT operationComplete(message, true);
    }
    
    return success;
}

bool FileListing::openDirectory(const QString &dirPath)
{
    qDebug() << "FileListing: Opening directory:" << dirPath;
    
    // Verify the directory exists
    QFileInfo info(dirPath);
    if (!info.exists() || !info.isDir()) {
        QString message = QStringLiteral("Directory does not exist: %1").arg(dirPath);
        qWarning() << "FileListing:" << message;
        Q_EMIT operationComplete(message, false);
        return false;
    }
    
    // Use QDesktopServices to open the directory in the file manager
    QUrl dirUrl = QUrl::fromLocalFile(dirPath);
    bool success = QDesktopServices::openUrl(dirUrl);
    
    if (!success) {
        QString message = QStringLiteral("Failed to open directory: %1").arg(dirPath);
        qWarning() << "FileListing:" << message;
        Q_EMIT operationComplete(message, false);
    } else {
        QString message = QStringLiteral("Opened directory: %1").arg(dirPath);
        Q_EMIT operationComplete(message, true);
    }
    
    return success;
}

bool FileListing::openFileInKate(const QString &filePath)
{
    qDebug() << "FileListing: Opening file in Kate:" << filePath;
    
    // Verify the file exists
    if (!fileExists(filePath)) {
        QString message = QStringLiteral("File does not exist: %1").arg(filePath);
        qWarning() << "FileListing:" << message;
        Q_EMIT operationComplete(message, false);
        return false;
    }
    
    // In a real implementation, this would use KService or similar to open in Kate
    // For now, just use a basic system call
    QProcess process;
    process.startDetached(QStringLiteral("kate"), QStringList() << filePath);
    
    QString message = QStringLiteral("Opening file in Kate: %1").arg(filePath);
    Q_EMIT operationComplete(message, true);
    
    return true;
}

void FileListing::copyPathToClipboard(const QString &filePath)
{
    qDebug() << "FileListing: Copying path to clipboard:" << filePath;
    
    // Copy the file path to the clipboard
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(filePath);
    
    QString message = QStringLiteral("Copied to clipboard: %1").arg(filePath);
    Q_EMIT operationComplete(message, true);
}

bool FileListing::executeFile(const QString &filePath)
{
    qDebug() << "FileListing: Executing file:" << filePath;
    
    // Verify the file exists and is executable
    if (!fileExists(filePath)) {
        QString message = QStringLiteral("File does not exist: %1").arg(filePath);
        qWarning() << "FileListing:" << message;
        Q_EMIT operationComplete(message, false);
        return false;
    }
    
    if (!isExecutable(filePath)) {
        QString message = QStringLiteral("File is not executable: %1").arg(filePath);
        qWarning() << "FileListing:" << message;
        Q_EMIT operationComplete(message, false);
        return false;
    }
    
    // In a real implementation, this would have additional safety checks
    // and possibly show a confirmation dialog
    
    // Get the directory containing the file
    QString workingDir = QFileInfo(filePath).absolutePath();
    
    // Build a command to execute the file
    QString command = QStringLiteral("\"%1\"").arg(filePath);
    
    // Emit the command to be executed
    Q_EMIT executeCommand(command);
    
    QString message = QStringLiteral("Executing: %1").arg(filePath);
    Q_EMIT operationComplete(message, true);
    
    return true;
}

bool FileListing::isExecutable(const QString &filePath)
{
    qDebug() << "FileListing: Checking if file is executable:" << filePath;
    
    QFileInfo fileInfo(filePath);
    
    // Check if file exists and has executable permissions
    if (fileInfo.exists() && fileInfo.isFile()) {
        return fileInfo.isExecutable();
    }
    
    return false;
}

QString FileListing::detectFileType(const QString &filename)
{
    qDebug() << "FileListing: Detecting file type:" << filename;
    
    // Use Qt's MIME database to detect file type
    QMimeType mimeType = m_mimeDb.mimeTypeForFile(filename);
    
    // Return the MIME type name
    return mimeType.name();
}

bool FileListing::isDirectory(const QString &filename, const QString &output)
{
    // Skip common terminal output words that aren't actually files or directories
    if (m_nonFileWords.contains(filename) || filename.contains(QRegularExpression(QStringLiteral("^\\d+$")))) {
        return false;
    }
    
    // First, check if the filename ends with a slash (common directory indicator)
    if (filename.endsWith(QStringLiteral("/"))) {
        return true;
    }
    
    // Check if the name is a common directory name
    if (filename == QStringLiteral(".") || filename == QStringLiteral("..")) {
        return true;
    }
    
    // Look for directory indicators in detailed ls output
    QRegularExpression dirPattern(QStringLiteral("^d[rwx\\-]{9}.*\\s+") + 
                                 QRegularExpression::escape(filename) + 
                                 QStringLiteral("\\s*$"), 
                                 QRegularExpression::MultilineOption);
    
    if (output.contains(dirPattern)) {
        return true;
    }
    
    // Alternatively, check the filesystem directly if possible
    QString fullPath = resolveFilePath(filename);
    QFileInfo fileInfo(fullPath);
    if (fileInfo.exists()) {
        return fileInfo.isDir();
    }
    
    // If no extension and not all digits, it might be a directory
    if (!filename.contains(QStringLiteral(".")) && 
        !filename.contains(QRegularExpression(QStringLiteral("^\\d+$"))) &&
        !filename.contains(QRegularExpression(QStringLiteral("[\\(\\)\\[\\]\\{\\}\\<\\>\\|\\*\\&\\^\\%\\$\\#\\@\\!\\~\\`]")))) {
        // Increase the chance that this is a directory, but not definite
        return true;
    }
    
    // Default to false if we can't determine
    return false;
}

void FileListing::setTerminalDirectory(const QString &directory)
{
    qDebug() << "FileListing: Setting terminal directory:" << directory;
    
    if (QDir(directory).exists()) {
        m_terminalDirectory = directory;
    } else {
        qWarning() << "FileListing: Invalid directory:" << directory;
    }
}

QString FileListing::terminalDirectory() const
{
    return m_terminalDirectory;
}

QString FileListing::resolveFilePath(const QString &path)
{
    qDebug() << "FileListing: Resolving file path:" << path;
    
    QFileInfo fileInfo(path);
    
    // If it's already an absolute path, return it
    if (fileInfo.isAbsolute()) {
        return path;
    }
    
    // If it starts with ~, expand to home directory
    if (path.startsWith(QStringLiteral("~"))) {
        QString result = path;
        return result.replace(0, 1, QDir::homePath());
    }
    
    // Otherwise, resolve relative to current terminal directory
    return QDir(m_terminalDirectory).filePath(path);
}

bool FileListing::fileExists(const QString &path)
{
    QString resolvedPath = resolveFilePath(path);
    return QFileInfo::exists(resolvedPath);
}


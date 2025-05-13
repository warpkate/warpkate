/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef FILELISTING_H
#define FILELISTING_H

#include <QObject>
#include <QMenu>
#include <QUrl>
#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <QMimeDatabase>

/**
 * @brief The FileListing class
 * 
 * This class handles file listing operations in the terminal output:
 * - Creating context menus for file/directory items
 * - Handling clicks on file listings
 * - Opening files with appropriate applications
 * - Detecting file types
 * - Clipboard operations
 */
class FileListing : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor
     * @param parent Parent object
     */
    explicit FileListing(QObject *parent = nullptr);
    
    /**
     * Destructor
     */
    ~FileListing();
    
    /**
     * Create a context menu for a file or directory
     * @param filePath Path to the file/directory
     * @param isDirectory Whether the item is a directory
     * @return Context menu populated with appropriate actions
     */
    QMenu* createFileContextMenu(const QString &filePath, bool isDirectory);
    
    /**
     * Handle clicking on a file/directory item
     * @param filePath Path to the file/directory
     * @param isDirectory Whether the item is a directory
     */
    void handleFileItemClicked(const QString &filePath, bool isDirectory);
    
    /**
     * Open a file with the default application
     * @param filePath Path to the file
     * @return True if the operation was successful
     */
    bool openFile(const QString &filePath);
    
    /**
     * Open a directory in the file manager
     * @param dirPath Path to the directory
     * @return True if the operation was successful
     */
    bool openDirectory(const QString &dirPath);
    
    /**
     * Open a file in Kate editor
     * @param filePath Path to the file
     * @return True if the operation was successful
     */
    bool openFileInKate(const QString &filePath);
    
    /**
     * Copy a file path to the clipboard
     * @param filePath Path to copy
     */
    void copyPathToClipboard(const QString &filePath);
    
    /**
     * Execute a file safely
     * @param filePath Path to the file to execute
     * @return True if the operation was successful
     */
    bool executeFile(const QString &filePath);
    
    /**
     * Check if a file is executable
     * @param filePath Path to the file
     * @return True if the file is executable
     */
    bool isExecutable(const QString &filePath);
    
    /**
     * Detect the type of a file
     * @param filename Path to the file
     * @return MIME type of the file
     */
    QString detectFileType(const QString &filename);
    
    /**
     * Check if a filename represents a directory
     * @param filename Filename to check
     * @param output Terminal output for context
     * @return True if filename likely represents a directory
     */
    bool isDirectory(const QString &filename, const QString &output);
    
    /**
     * Set known terminal directory (for resolving relative paths)
     * @param directory Current terminal directory
     */
    void setTerminalDirectory(const QString &directory);
    
    /**
     * Get the current terminal directory
     * @return Current terminal directory
     */
    QString terminalDirectory() const;

Q_SIGNALS:
    /**
     * Emitted when a command should be executed in the terminal
     * @param command The command to execute
     */
    void executeCommand(const QString &command);
    
    /**
     * Emitted when a file operation completes
     * @param message Status message
     * @param success Whether the operation was successful
     */
    void operationComplete(const QString &message, bool success);

private:
    /**
     * Resolve a potentially relative path to absolute
     * @param path Path to resolve
     * @return Absolute path
     */
    QString resolveFilePath(const QString &path);
    
    /**
     * Check if a file exists
     * @param path Path to check
     * @return True if the file exists
     */
    bool fileExists(const QString &path);
    
    // Current terminal directory
    QString m_terminalDirectory;
    
    // List of common non-file words that appear in terminal output
    QStringList m_nonFileWords;
    
    // MIME database for file type detection
    QMimeDatabase m_mimeDb;
};

#endif // FILELISTING_H


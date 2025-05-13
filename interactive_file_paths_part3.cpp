/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
 * This file implements the remaining functions needed for the file paths feature in WarpKate.
 * It includes processDetailedListing, processSimpleListing, createFileContextMenu, and executeFile.
 */

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QProcess>
#include <QRegularExpression>
#include <QTextCursor>
#include <QTextEdit>
#include <QUrl>

#include <KLocalizedString>
#include <KTextEditor/MainWindow>

/**
 * Process detailed file listing (ls -l format) to add interactivity
 * @param output Terminal output to process
 * @param workingDir Current working directory
 * @return Formatted HTML output with interactive elements
 */
QString WarpKateView::processDetailedListing(const QString &output, const QString &workingDir)
{
    // Start with HTML pre tag for proper formatting
    QString htmlOutput = QStringLiteral("<pre style='margin: 0; padding: 0; white-space: pre-wrap;'>");
    
    // Process line by line
    QStringList lines = output.split(QStringLiteral("\n"));
    
    for (const QString &line : lines) {
        // Skip empty lines
        if (line.trimmed().isEmpty()) {
            htmlOutput += QStringLiteral("\n");
            continue;
        }
        
        // Check if this is a "total XX" line (ls -l header)
        if (line.startsWith(QStringLiteral("total "))) {
            htmlOutput += line.toHtmlEscaped() + QStringLiteral("\n");
            continue;
        }
        
        // Regular expression to match ls -l format: permissions, links, owner, group, size, date, filename
        QRegularExpression detailedRegex(QStringLiteral("^([d\\-][rwx\\-]{9})\\s+(\\d+)\\s+(\\w+)\\s+(\\w+)\\s+(\\d+)\\s+(\\w+\\s+\\d+\\s+[\\d:]+)\\s+(.+)$"));
        QRegularExpressionMatch match = detailedRegex.match(line);
        
        if (match.hasMatch()) {
            // Extract filename (which could be a directory)
            QString permissions = match.captured(1);
            QString filename = match.captured(7);
            
            // Check if this is a directory by permissions or by filename end slash
            bool isDir = permissions.startsWith(QStringLiteral("d")) || filename.endsWith(QStringLiteral("/"));
            
            // Create full path
            QString fullPath = workingDir;
            if (!fullPath.endsWith(QStringLiteral("/"))) {
                fullPath += QStringLiteral("/");
            }
            fullPath += filename;
            
            // Check if file exists and if it's executable
            QFileInfo fileInfo(fullPath);
            bool isExec = !isDir && isExecutable(fullPath);
            
            // Prepare HTML for this line
            QString formattedLine;
            QString fileStyle;
            QString fileIcon;
            
            // Create style and icon based on file type
            if (isDir) {
                fileStyle = QStringLiteral("color: #4040FF; font-weight: bold;");
                fileIcon = QStringLiteral("🗀 "); // Directory icon
            } else if (isExec) {
                fileStyle = QStringLiteral("color: #40AA40; font-weight: bold;");
                fileIcon = QStringLiteral("⚙️ "); // Executable icon
            } else {
                // Determine style based on file type
                QString fileType = detectFileType(fullPath);
                if (fileType.startsWith(QStringLiteral("text/"))) {
                    fileStyle = QStringLiteral("color: inherit;");
                    fileIcon = QStringLiteral("📄 "); // Text file icon
                } else if (fileType.startsWith(QStringLiteral("image/"))) {
                    fileStyle = QStringLiteral("color: #AA40AA;");
                    fileIcon = QStringLiteral("🖼️ "); // Image icon
                } else if (fileType.startsWith(QStringLiteral("video/"))) {
                    fileStyle = QStringLiteral("color: #DD4040;");
                    fileIcon = QStringLiteral("🎬 "); // Video icon
                } else if (fileType.startsWith(QStringLiteral("audio/"))) {
                    fileStyle = QStringLiteral("color: #40AAAA;");
                    fileIcon = QStringLiteral("🎵 "); // Audio icon
                } else {
                    fileStyle = QStringLiteral("color: inherit;");
                    fileIcon = QStringLiteral("📁 "); // Generic file icon
                }
            }
            
            // Create HTML for this line with clickable filename
            // First part contains permissions, links, owner, group, size, date
            QString linePart1 = line.left(line.length() - filename.length()).toHtmlEscaped();
            
            // Create clickable link for filename with appropriate style
            QString fileLink = QStringLiteral("<a href=\"file://%1\" style=\"%2 text-decoration: none;\">%3%4</a>")
                .arg(fullPath.toHtmlEscaped(), fileStyle, fileIcon, filename.toHtmlEscaped());
            
            // Combine parts and add to output
            formattedLine = linePart1 + fileLink;
            htmlOutput += formattedLine + QStringLiteral("\n");
        } else {
            // If not matched, just add the line as is
            htmlOutput += line.toHtmlEscaped() + QStringLiteral("\n");
        }
    }
    
    htmlOutput += QStringLiteral("</pre>");
    return htmlOutput;
}

/**
 * Process simple file listing (ls without options) to add interactivity
 * @param output Terminal output to process
 * @param workingDir Current working directory
 * @return Formatted HTML output with interactive elements
 */
QString WarpKateView::processSimpleListing(const QString &output, const QString &workingDir)
{
    QString htmlOutput = QStringLiteral("<pre style='margin: 0; padding: 0; white-space: pre-wrap;'>");
    QStringList lines = output.split(QStringLiteral("\n"));
    
    for (const QString &line : lines) {
        // Skip empty lines
        if (line.trimmed().isEmpty()) {
            htmlOutput += QStringLiteral("\n");
            continue;
        }
        
        // Split the line by whitespace to process individual filenames
        QStringList entries = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        
        // Process each file/directory entry in the line
        QString processedLine = line.toHtmlEscaped();
        for (const QString &entry : entries) {
            // Skip if entry is empty or just dots
            if (entry.isEmpty() || entry == QStringLiteral(".") || entry == QStringLiteral("..")) {
                continue;
            }
            
            // Create full path
            QString fullPath = workingDir;
            if (!fullPath.endsWith(QStringLiteral("/"))) {
                fullPath += QStringLiteral("/");
            }
            fullPath += entry;
            
            // Check if this entry is a directory or executable
            QFileInfo fileInfo(fullPath);
            bool isDir = false;
            bool isExec = false;
            
            if (fileInfo.exists()) {
                isDir = fileInfo.isDir();
                isExec = !isDir && isExecutable(fullPath);
            } else {
                // Heuristic: if entry ends with / assume it's a directory
                isDir = entry.endsWith(QStringLiteral("/"));
            }
            
            // Prepare style and icon
            QString fileStyle;
            QString fileIcon;
            
            if (isDir) {
                fileStyle = QStringLiteral("color: #4040FF; font-weight: bold;");
                fileIcon = QStringLiteral("🗀 "); // Directory icon
            } else if (isExec) {
                fileStyle = QStringLiteral("color: #40AA40; font-weight: bold;");
                fileIcon = QStringLiteral("⚙️ "); // Executable icon
            } else {
                // Determine style based on file type
                QString fileType = detectFileType(fullPath);
                if (fileType.startsWith(QStringLiteral("text/"))) {
                    fileStyle = QStringLiteral("color: inherit;");
                    fileIcon = QStringLiteral("📄 "); // Text file icon
                } else if (fileType.startsWith(QStringLiteral("image/"))) {
                    fileStyle = QStringLiteral("color: #AA40AA;");
                    fileIcon = QStringLiteral("🖼️ "); // Image icon
                } else if (fileType.startsWith(QStringLiteral("video/"))) {
                    fileStyle = QStringLiteral("color: #DD4040;");
                    fileIcon = QStringLiteral("🎬 "); // Video icon
                } else if (fileType.startsWith(QStringLiteral("audio/"))) {
                    fileStyle = QStringLiteral("color: #40AAAA;");
                    fileIcon = QStringLiteral("🎵 "); // Audio icon
                } else {
                    fileStyle = QStringLiteral("color: inherit;");
                    fileIcon = QStringLiteral("📁 "); // Generic file icon
                }
            }
            
            // Replace with formatted version
            QString entryRegex = QRegularExpression::escape(entry);
            processedLine.replace(
                QRegularExpression(QStringLiteral("\\b") + entryRegex + QStringLiteral("\\b")),
                QStringLiteral("<a href=\"file://%1\" style=\"%2 text-decoration: none;\">%3%4</a>")
                .arg(fullPath.toHtmlEscaped(), fileStyle, fileIcon, entry.toHtmlEscaped())
            );
        }
        
        htmlOutput += processedLine + QStringLiteral("\n");
    }
    
    htmlOutput += QStringLiteral("</pre>");
    return htmlOutput;
}

/**
 * Create a context menu for file operations
 * @param filePath Full path to the file
 * @param isDirectory Whether the item is a directory
 * @return Context menu for the item
 */
QMenu* WarpKateView::createFileContextMenu(const QString &filePath, bool isDirectory)
{
    QMenu *menu = new QMenu();
    QFileInfo fileInfo(filePath);
    
    // Check if file exists
    if (!fileInfo.exists()) {
        QAction *errorAction = menu->addAction(i18n("File not found"));
        errorAction->setEnabled(false);
        return menu;
    }
    
    // Open action (default)
    QAction *openAction = menu->addAction(QIcon::fromTheme(QStringLiteral("document-open")), i18n("Open"));
    connect(openAction, &QAction::triggered, this, [this, filePath, isDirectory]() {
        if (isDirectory) {
            openDirectory(filePath);
        } else {
            openFile(filePath);
        }
    });
    
    menu->addSeparator();
    
    if (!isDirectory) {
        // Open with Kate (for files only)
        QAction *openWithKateAction = menu->addAction(QIcon::fromTheme(QStringLiteral("kate")), i18n("Open with Kate"));
        connect(openWithKateAction, &QAction::triggered, this, [this, filePath]() {
            openFileInKate(filePath);
        });
        
        // Execute (for executable files only)
        if (isExecutable(filePath)) {
            menu->addSeparator();
            QAction *executeAction = menu->addAction(QIcon::fromTheme(QStringLiteral("system-run")), i18n("Execute"));
            connect(executeAction, &QAction::triggered, this, [this, filePath]() {
                executeFile(filePath);
            });
        }
    }
    
    menu->addSeparator();
    
    // Open containing folder
    QString dirPath = isDirectory ? filePath : QFileInfo(filePath).absolutePath();
    QAction *openFolderAction = menu->addAction(QIcon::fromTheme(QStringLiteral("folder-open")), i18n("Open Containing Folder"));
    connect(openFolderAction, &QAction::triggered, this, [this, dirPath]() {
        openDirectory(dirPath);
    });
    
    // Copy path to clipboard
    QAction *copyPathAction = menu->addAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy Path to Clipboard"));
    connect(copyPathAction, &QAction::triggered, this, [this, filePath]() {
        copyPathToClipboard(filePath);
    });
    
    return menu;
}

/**
 * Copy a file path to clipboard
 * @param filePath Full path to copy
 */
void WarpKateView::copyPathToClipboard(const QString &filePath)
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(filePath);
    
    // Log the action
    qDebug() << "WarpKate: Copied path to clipboard:" << filePath;
    
    // Add a notification to the conversation area
    QTextCursor cursor = m_conversationArea->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_conversationArea->setTextCursor(cursor);
    
    QTextCharFormat infoFormat;
    infoFormat.setForeground(QBrush(QColor(80, 80, 200))); // Blue
    
    cursor.insertBlock();
    cursor.setCharFormat(infoFormat);
    cursor.insertText(i18n("Path copied to clipboard: %1", filePath));
    cursor.setCharFormat(QTextCharFormat());
    
    m_conversationArea->ensureCursorVisible();
}

/**
 * Execute a file safely
 * @param filePath Full path to the file to execute
 */
void WarpKateView::executeFile(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    
    // Check if file exists and is executable
    if (!fileInfo.exists() || !fileInfo.isExecutable()) {
        qWarning() << "


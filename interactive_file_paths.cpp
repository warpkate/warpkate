/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
 * This file contains enhanced implementations of the functions needed for
 * clickable file paths in WarpKate's terminal output.
 * 
 * To use these implementations, copy the relevant functions to your warpkateview.cpp file
 * and add the isExecutable function declaration to warpkateview.h.
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

// Add this function declaration to WarpKateView.h
//
// /**
//  * Check if a file is executable
//  * @param filePath Full path to the file
//  * @return True if the file is executable
//  */
// bool isExecutable(const QString &filePath);
//
// /**
//  * Execute a file safely
//  * @param filePath Full path to the file to execute
//  */
// void executeFile(const QString &filePath);

/**
 * Check if a file is executable
 * @param filePath Full path to the file
 * @return True if the file is executable
 */
bool WarpKateView::isExecutable(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    
    // Check if file exists and has executable permissions
    if (fileInfo.exists() && fileInfo.isFile()) {
        return fileInfo.isExecutable();
    }
    
    return false;
}

/**
 * Process terminal output to make file paths interactive
 * Enhanced version with better file path detection and visual styling
 * 
 * @param output Terminal output to process
 * @return HTML-formatted output with clickable file paths
 */
QString WarpKateView::processTerminalOutputForInteractivity(const QString &output)
{
    // If output is empty, return early
    if (output.isEmpty()) {
        return output;
    }
    
    // Get the current working directory
    QString workingDir;
    if (m_terminalEmulator) {
        workingDir = m_terminalEmulator->currentWorkingDirectory();
    }
    
    // If we don't have a working directory, try to extract it from the output
    if (workingDir.isEmpty()) {
        // Attempt to extract working directory from PS1 prompt if present in output
        QRegularExpression cwdRegex(QStringLiteral("\\[(.*?)\\]\\$"));
        QRegularExpressionMatch cwdMatch = cwdRegex.match(output);
        if (cwdMatch.hasMatch()) {
            workingDir = cwdMatch.captured(1);
        } else {
            // Default to home directory if nothing else available
            workingDir = QDir::homePath();
        }
    }
    
    // Quick check if this output is unlikely to contain file paths
    if (!output.contains(QStringLiteral("/")) && 
        !output.contains(QStringLiteral(".")) && 
        !output.contains(QStringLiteral("drwx")) && 
        !output.contains(QStringLiteral("-rw-"))) {
        // Unlikely to have file paths, return as plain text
        return output;
    }
    
    // Check if this output is likely from an ls command
    bool isLsOutput = false;
    QStringList lines = output.split(QStringLiteral("\n"));
    int fileEntryCount = 0;
    
    // Look for patterns that suggest this is an ls command output
    QString firstLine = lines.isEmpty() ? QString() : lines.first().trimmed();
    QString lastLine = lines.isEmpty() ? QString() : lines.last().trimmed();
    
    // Check for ls command pattern in the first line
    if (firstLine.startsWith(QStringLiteral("total ")) || 
        output.contains(QStringLiteral("drwx")) || 
        output.contains(QStringLiteral("-rw-"))) {
        isLsOutput = true;
    }
    
    // Also check first few lines for ls-like patterns
    for (int i = 0; i < qMin(5, lines.size()); ++i) {
        if (lines[i].contains(QRegularExpression(QStringLiteral("^[d\\-][rwx\\-]{9}")))) {
            isLsOutput = true;
            break;
        }
    }
    
    // Specific check for `ls` (no flags) output which just lists filenames
    // Count how many plausible file entries we have
    for (const QString &line : lines) {
        if (!line.trimmed().isEmpty() && !line.startsWith(QStringLiteral("total "))) {
            // Split by whitespace to see if we have multiple entries per line (common in ls)
            QStringList entries = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
            fileEntryCount += entries.size();
        }
    }
    
    if (fileEntryCount > 3 && lines.size() < 10) {
        // Likely an `ls` command with just filenames
        isLsOutput = true;
    }
    
    // Process line by line to look for individual file listing patterns
    if (isLsOutput) {
        bool hasDetailedInfo = output.contains(QRegularExpression(QStringLiteral("^[d\\-][rwx\\-]{9}")));
        if (hasDetailedInfo) {
            // Use the detailed processing for ls -l like output
            return processDetailedListing(output, workingDir);
        } else {
            // Use simple processing for ls without details
            return processSimpleListing(output, workingDir);
        }
    }
    
    // Prepare HTML output
    QString htmlOutput = QStringLiteral("<pre style='margin: 0; padding: 0; white-space: pre-wrap;'>");
    
    // Count consecutive blank lines to avoid creating too much whitespace
    int consecutiveBlankLines = 0;
    
    // Process line by line for other command types (find, grep, etc.)
    for (const QString &line : lines) {
        // Skip excessive blank lines
        if (line.trimmed().isEmpty()) {
            consecutiveBlankLines++;
            if (consecutiveBlankLines <= 2) {
                htmlOutput += QStringLiteral("\n");
            }
            continue;
        }
        consecutiveBlankLines = 0;
        
        // Check if this line contains file paths
        QString processedLine = line.toHtmlEscaped();
        bool hasProcessedFilePaths = processFileListingLine(processedLine, workingDir);
        
        if (!hasProcessedFilePaths) {
            // If not a file listing, check for file paths in the text
            
            // Look for absolute paths - enhanced to handle more file naming patterns
            QRegularExpression absolutePathRegex(QStringLiteral("(?<![\\w-])/(?:[\\w.+\\-_~@]+/)*(?:[\\w.+\\-_~@]+)"));
            QRegularExpressionMatchIterator matches = absolutePathRegex.globalMatch(line);
            
            while (matches.hasNext()) {
                QRegularExpressionMatch match = matches.next();
                QString path = match.captured(0);
                
                // Skip if this appears to be a command option (e.g., --option=/some/path)
                if (line.contains(QStringLiteral("=") + path)) {
                    continue;
                }
                
                // Check if path exists
                QFileInfo fileInfo(path);
                if (fileInfo.exists()) {
                    bool isDir = fileInfo.isDir();
                    bool isExec = isExecutable(path);
                    
                    // Replace with appropriate HTML
                    QString pathRegex = QRegularExpression::escape(path);
                    QRegularExpression pathRe(QStringLiteral("\\b") + pathRegex + QStringLiteral("\\b"));
                    
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
                        QString fileType = detectFileType(path);
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
                    
                    processedLine.replace(pathRe, 
                        QStringLiteral("<a href=\"file://%1\" style=\"%2 text-decoration: none;\">%3%4</a>")
                        .arg(path.toHtmlEscaped(), fileStyle, fileIcon, path.toHtmlEscaped()));
                }
            }
            
            // Look for relative paths (if we have a working directory)
            if (!workingDir.isEmpty()) {
                // Enhanced regex for relative paths - handles more file naming patterns
                QRegularExpression relativePathRegex(QStringLiteral("\\b(?:\\./)?[\\w.+\\-_~@]+(?:/[\\w.+\\-_~@]+)*\\b"));
                matches = relativePathRegex.globalMatch(line);
                
                while (matches.hasNext()) {
                    QRegularExpressionMatch match = matches.next();
                    QString relativePath = match.captured(0);
                    
                    // Skip common command names and options that might match our pattern
                    if (relativePath.startsWith(QStringLiteral("./")) || 
                        (!relativePath.contains(QStringLiteral("/")) && relativePath.contains(QStringLiteral(".")))) {
                        // Skip if it's likely a command option
                        if (line.contains(QStringLiteral("-") + relativePath) || 
                            line.contains(QStringLiteral("--") + relativePath)) {
                            continue;
                        }
                        
                        // Create absolute path by combining with working directory
                        QString fullPath = workingDir;
                        if (!fullPath.endsWith(QStringLiteral("/"))) {
                            fullPath += QStringLiteral("/");
                        }
                        
                        if (relativePath.startsWith(QStringLiteral("./"))) {
                            fullPath += relativePath.mid(2); // Skip the ./ prefix
                        } else {
                            fullPath += relativePath;
                        }
                        
                        // Check if this path exists
                        QFileInfo fileInfo(fullPath);
                        if (fileInfo.exists()) {
                            bool isDir = fileInfo.isDir();
                            bool isExec = isExecutable(fullPath);
                            
                            // Replace with appropriate HTML
                            QString pathRegex = QRegularExpression::escape(relativePath);
                            QRegularExpression pathRe(QStringLiteral("\\b") + pathRegex + QStringLiteral("\\b"));
                            
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
                            
                            processedLine.replace(pathRe, 
                                QStringLiteral("<a href=\"file://%1\" style=\"%2 text-decoration: none;\">%3%4</a>")
                                .arg(fullPath.toHtmlEscaped(), fileStyle, fileIcon, relativePath.toHtmlEscaped()));
                        }
                    }
                }
            }
        }
        
        htmlOutput += processedLine +


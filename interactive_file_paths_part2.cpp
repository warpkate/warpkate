/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
 * This file continues the implementation of the processTerminalOutputForInteractivity function
 * from interactive_file_paths_part1.cpp
 * 
 * Part 2: Processing non-ls output to detect file paths and create HTML links
 */

/*
 * The function continues from the 'if (isLsOutput)' conditional in part1.
 * Here is the remaining implementation:
 */

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
        
        htmlOutput += processedLine + QStringLiteral("\n");
    }
    
    htmlOutput += QStringLiteral("</pre>");
    return htmlOutput;
}


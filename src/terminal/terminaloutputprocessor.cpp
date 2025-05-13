/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "terminaloutputprocessor.h"

#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QTextDocument>
#include <QRegularExpression>

TerminalOutputProcessor::TerminalOutputProcessor()
{
    // Initialize regular expressions for cleaning terminal output
    m_ansiEscapeRE = QRegularExpression(QStringLiteral("\033\\[[0-9;]*[A-Za-z]"));
    m_oscSequenceRE = QRegularExpression(QStringLiteral("\033\\][0-9].*;.*(\007|\033\\\\)"));
    m_termStatusRE = QRegularExpression(QStringLiteral("\\[\\?[0-9;]*[a-zA-Z]"));
    m_controlCharsRE = QRegularExpression(QStringLiteral("[\\x00-\\x08\\x0B\\x0C\\x0E-\\x1F]"));
    
    // Initialize list of words to ignore in file listings
    m_nonFileWords << QStringLiteral("total") 
                  << QStringLiteral("ls")
                  << QStringLiteral("cd")
                  << QStringLiteral("grep")
                  << QStringLiteral("find");
}

TerminalOutputProcessor::~TerminalOutputProcessor()
{
    // No specific cleanup needed
}

QString TerminalOutputProcessor::cleanTerminalOutput(const QString &rawOutput)
{
    if (rawOutput.isEmpty()) {
        return rawOutput;
    }
    
    // Additional regular expressions needed for cleaning
    static QRegularExpression termPromptRE(QStringLiteral("\\][0-9];[^\007]*"));
    static QRegularExpression bellRE(QStringLiteral("\007"));
    
    // Start cleaning the output
    QString cleaned = rawOutput;
    
    // Log the original and cleaned output for debugging
    qDebug() << "Original terminal output:" << (rawOutput.length() > 50 ? rawOutput.left(50) + QStringLiteral("...") : rawOutput);
    
    // Apply all the filters
    cleaned = cleaned.replace(m_ansiEscapeRE, QStringLiteral(""));
    cleaned = cleaned.replace(m_oscSequenceRE, QStringLiteral(""));
    cleaned = cleaned.replace(m_termStatusRE, QStringLiteral(""));
    cleaned = cleaned.replace(termPromptRE, QStringLiteral(""));
    cleaned = cleaned.replace(m_controlCharsRE, QStringLiteral(""));
    cleaned = cleaned.replace(bellRE, QStringLiteral(""));
    
    // Additional cleanup for common escape sequences in bash/zsh
    cleaned = cleaned.replace(QStringLiteral("\\]0;"), QStringLiteral(""));
    
    // Log the cleaned output for debugging
    qDebug() << "Cleaned terminal output:" << (cleaned.length() > 50 ? cleaned.left(50) + QStringLiteral("...") : cleaned);
    
    return cleaned;
}

QString TerminalOutputProcessor::processTerminalOutputForInteractivity(const QString &output, const QString &workingDir)
{
    // If output is empty, return early
    if (output.isEmpty()) {
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
    bool hasProcessedFileListing = false;
    QStringList processedLines;
    for (const QString &line : lines) {
        if (processFileListingLine(line, workingDir)) {
            hasProcessedFileListing = true;
            processedLines.append(line);
        }
    }
    
    // If we found and processed individual file listings, format them as HTML
    if (hasProcessedFileListing && !processedLines.isEmpty()) {
        QString htmlOutput = QStringLiteral("<pre>");
        for (const QString &line : processedLines) {
            htmlOutput += line + QStringLiteral("\n");
        }
        htmlOutput += QStringLiteral("</pre>");
        return htmlOutput;
    }
    
    // If we have an ls output, process it to add interactivity
    if (isLsOutput) {
        QString htmlOutput;
        
        // Process ls output with different formats based on detected style
        if (output.contains(QRegularExpression(QStringLiteral("^[d\\-][rwx\\-]{9}")))) {
            // Detailed listing (ls -l format)
            htmlOutput = processDetailedListing(output, workingDir);
        } else {
            // Simple listing (ls format)
            htmlOutput = processSimpleListing(output, workingDir);
        }
        
        return htmlOutput;
    }
    
    // Not an ls output, just add basic styling (make directories bold)
    QString htmlOutput = QStringLiteral("<pre>");
    
    // Process each line to detect and highlight directories
    for (const QString &line : lines) {
        QString processedLine = line;
        
        // Look for potential directory patterns (common shell prompts, cd commands)
        QRegularExpression dirRegex(QStringLiteral("(?:\\[|cd\\s+)([\\w\\.\\-/~]+)(?:\\]|$)"));
        QRegularExpressionMatch dirMatch = dirRegex.match(line);
        
        if (dirMatch.hasMatch()) {
            QString dirPath = dirMatch.captured(1);
            // Make the directory bold and clickable
            QString fullPath = dirPath;
            if (!fullPath.startsWith(QStringLiteral("/")) && !fullPath.startsWith(QStringLiteral("~"))) {
                fullPath = workingDir + QStringLiteral("/") + dirPath;
            }
            processedLine.replace(dirPath, QStringLiteral("<a href=\"file://%1\" style=\"color: inherit; text-decoration: none;\"><b>%2</b></a>")
                                 .arg(fullPath.toHtmlEscaped(), dirPath.toHtmlEscaped()));
        }
        
        htmlOutput += processedLine + QStringLiteral("\n");
    }
    
    htmlOutput += QStringLiteral("</pre>");
    return htmlOutput;
}

QString TerminalOutputProcessor::processDetailedListing(const QString &output, const QString &workingDir)
{
    QString htmlOutput = QStringLiteral("<pre>");
    QStringList lines = output.split(QStringLiteral("\n"));
    
    for (const QString &line : lines) {
        // Skip empty lines
        if (line.trimmed().isEmpty()) {
            htmlOutput += QStringLiteral("\n");
            continue;
        }
        
        // Skip the "total" line that appears in ls -l output
        if (line.startsWith(QStringLiteral("total "))) {
            htmlOutput += line + QStringLiteral("\n");
            continue;
        }
        
        // Check if this is a potential file/directory line (ls -l format)
        QRegularExpression fileRegex(QStringLiteral("^([d\\-])([rwx\\-]{9})\\s+\\d+\\s+\\w+\\s+\\w+\\s+\\d+\\s+\\w+\\s+\\d+\\s+[\\d:]+\\s+(.+)$"));
        QRegularExpressionMatch fileMatch = fileRegex.match(line);
        
        if (fileMatch.hasMatch()) {
            bool isDir = fileMatch.captured(1) == QStringLiteral("d");
            QString permissions = fileMatch.captured(2);
            QString filename = fileMatch.captured(3);
            
            // Build the full path
            QString fullPath = workingDir + QStringLiteral("/") + filename;
            
            // Format the line based on file type - make directories bold
            QString formattedLine;
            if (isDir) {
                // Make directory name bold and clickable
                formattedLine = line.left(line.length() - filename.length());
                formattedLine += QStringLiteral("<a href=\"file://%1\" style=\"color: inherit; text-decoration: none;\"><b>%2</b></a>")
                                .arg(fullPath.toHtmlEscaped(), filename.toHtmlEscaped());
            } else {
                // Make regular files clickable
                formattedLine = line.left(line.length() - filename.length());
                formattedLine += QStringLiteral("<a href=\"file://%1\" style=\"color: inherit; text-decoration: none;\">%2</a>")
                                .arg(fullPath.toHtmlEscaped(), filename.toHtmlEscaped());
            }
            
            htmlOutput += formattedLine + QStringLiteral("\n");
        } else {
            // If not matched, just add the line as is
            htmlOutput += line + QStringLiteral("\n");
        }
    }
    
    htmlOutput += QStringLiteral("</pre>");
    return htmlOutput;
}

QString TerminalOutputProcessor::processSimpleListing(const QString &output, const QString &workingDir)
{
    QString htmlOutput = QStringLiteral("<pre>");
    QStringList lines = output.split(QStringLiteral("\n"));
    
    for (const QString &line : lines) {
        // Skip empty lines
        if (line.trimmed().isEmpty()) {
            htmlOutput += QStringLiteral("\n");
            continue;
        }
        
        // Skip the "total" line that appears in ls -l output
        if (line.startsWith(QStringLiteral("total "))) {
            htmlOutput += line + QStringLiteral("\n");
            continue;
        }
        
        // Split the line by whitespace to process individual filenames
        QStringList entries = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        
        // Process each file/directory entry in the line
        QString processedLine = line;
        for (const QString &entry : entries) {
            // Skip if entry is empty or just dots
            if (entry.isEmpty() || entry == QStringLiteral(".") || entry == QStringLiteral("..")) {
                continue;
            }
            
            // Skip common non-file words from ls output
            if (entry == QStringLiteral("total") || 
                entry.startsWith(QStringLiteral("-")) || 
                entry == QStringLiteral("ls") || 
                entry.length() < 2) {
                continue;
            }
            
            // Check if this entry is a directory
            QString fullPath = workingDir + QStringLiteral("/") + entry;
            bool isDir = isDirectory(entry, output);
            if (!isDir) {
                // Double-check with filesystem if available
                QFileInfo fileInfo(fullPath);
                if (fileInfo.exists()) {
                    isDir = fileInfo.isDir();
                }
            }
            
            // Replace with formatted version
            if (isDir) {
                // Directory - make bold and clickable
                processedLine.replace(QRegularExpression(QStringLiteral("\\b") + QRegularExpression::escape(entry) + QStringLiteral("\\b")),
                                     QStringLiteral("<a href=\"file://%1\" style=\"color: inherit; text-decoration: none;\"><b>%2</b></a>")
                                     .arg(fullPath.toHtmlEscaped(), entry.toHtmlEscaped()));
            } else {
                // Regular file - make clickable
                processedLine.replace(QRegularExpression(QStringLiteral("\\b") + QRegularExpression::escape(entry) + QStringLiteral("\\b")),
                                     QStringLiteral("<a href=\"file://%1\" style=\"color: inherit; text-decoration: none;\">%2</a>")
                                     .arg(fullPath.toHtmlEscaped(), entry.toHtmlEscaped()));
            }
        }
        
        htmlOutput += processedLine + QStringLiteral("\n");
    }
    
    htmlOutput += QStringLiteral("</pre>");
    return htmlOutput;
}

bool TerminalOutputProcessor::processFileListingLine(const QString &line, const QString &workingDir)
{
    // Skip empty lines
    if (line.trimmed().isEmpty()) {
        return false;
    }
    
    // Skip the "total" line that appears in ls -l output
    if (line.startsWith(QStringLiteral("total "))) {
        return false;
    }
    
    // Check if this is a detailed listing line (ls -l format)
    QRegularExpression detailedRegex(QStringLiteral("^[d\\-][rwx\\-]{9}\\s+\\d+\\s+\\w+\\s+\\w+\\s+\\d+\\s+\\w+\\s+\\d+\\s+[\\d:]+\\s+(.+)$"));
    QRegularExpressionMatch detailedMatch = detailedRegex.match(line);
    
    if (detailedMatch.hasMatch()) {
        // This is a file listing in detailed format
        return true;
    }
    
    // Check if this is a simple file/directory name
    // Look for patterns that might indicate it's in a listing context
    QStringList entries = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    
    // If we have a series of what looks like filenames (non-command strings)
    if (entries.size() > 2) {
        bool allLikelyFilenames = true;
        for (const QString &entry : entries) {
            // Skip if it looks like a command flag
            if (entry.startsWith(QStringLiteral("-")) && entry.length() > 1 && !entry.at(1).isDigit()) {
                allLikelyFilenames = false;
                break;
            }
            
            // Skip if it's a common command
            if (entry == QStringLiteral("ls") || entry == QStringLiteral("cd") || 
                entry == QStringLiteral("grep") || entry == QStringLiteral("find")) {
                allLikelyFilenames = false;
                break;
            }
        }
        
        if (allLikelyFilenames) {
            return true;
        }
    }
    
    // Not a file listing line
    return false;
}

bool TerminalOutputProcessor::isDirectory(const QString &filename, const QString &output)
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
    
    // Check filesystem if we have a working directory
    if (!filename.isEmpty()) {
        // This would need to be handled differently without direct m_terminalEmulator access
        // For now, we'll just check if the file has no extension, which is a common directory trait
        if (!filename.contains(QStringLiteral(".")) && 
            (filename.length() > 2) && 
            !filename.contains(QRegularExpression(QStringLiteral("[\\(\\)\\[\\]\\{\\}\\<\\>\\|\\*\\&\\^\\%\\$\\#\\@\\!\\~\\`]"))) &&
            // Make sure filename doesn't consist of only digits
            !filename.contains(QRegularExpression(QStringLiteral("^\\d+$"))) &&
            // Exclude common terminal output words
            !m_nonFileWords.contains(filename)) {
            // Increase the chance that this is a directory, but not definite
            return true;
        }
    }
    
    // Default to false if we can't determine
    return false;
}


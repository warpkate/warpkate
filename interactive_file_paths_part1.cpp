/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
 * This file contains enhanced implementations of file path detection functions for WarpKate.
 * Part 1: isExecutable and processTerminalOutputForInteractivity functions
 */

#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QDebug>

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


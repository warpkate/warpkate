/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TERMINALOUTPUTPROCESSOR_H
#define TERMINALOUTPUTPROCESSOR_H

#include <QString>
#include <QRegularExpression>
#include <QStringList>
#include <QUrl>

/**
 * @brief The TerminalOutputProcessor class
 * 
 * This class is responsible for processing terminal output:
 * - Cleaning ANSI escape sequences and control characters
 * - Adding interactivity to file listings and paths
 * - Processing detailed and simple file listings for display
 * - Detecting directories and file types
 */
class TerminalOutputProcessor
{
public:
    /**
     * Constructor
     */
    TerminalOutputProcessor();
    
    /**
     * Destructor
     */
    ~TerminalOutputProcessor();
    
    /**
     * Clean terminal output by removing ANSI escape sequences and control characters
     * @param rawOutput The raw terminal output
     * @return Cleaned output string
     */
    QString cleanTerminalOutput(const QString &rawOutput);
    
    /**
     * Process terminal output to add interactivity for file listings, paths, etc.
     * @param output Cleaned terminal output
     * @param workingDir Current working directory for resolving relative paths
     * @return Processed output with interactivity added
     */
    QString processTerminalOutputForInteractivity(const QString &output, const QString &workingDir);
    
    /**
     * Process a detailed file listing (ls -l format) to add interactivity
     * @param output Terminal output containing a detailed listing
     * @param workingDir Current working directory for resolving paths
     * @return HTML-formatted output with interactive elements
     */
    QString processDetailedListing(const QString &output, const QString &workingDir);
    
    /**
     * Process a simple file listing (ls format) to add interactivity
     * @param output Terminal output containing a simple listing
     * @param workingDir Current working directory for resolving paths
     * @return HTML-formatted output with interactive elements
     */
    QString processSimpleListing(const QString &output, const QString &workingDir);
    
    /**
     * Process a single line from a file listing to detect files and directories
     * @param line Line from terminal output
     * @param workingDir Current working directory for resolving paths
     * @return True if the line was processed as a file listing
     */
    bool processFileListingLine(const QString &line, const QString &workingDir);
    
    /**
     * Determine if a filename represents a directory
     * @param filename Filename to check
     * @param output Complete terminal output for context
     * @return True if the filename represents a directory
     */
    bool isDirectory(const QString &filename, const QString &output);

private:
    // Regular expressions for parsing terminal output
    QRegularExpression m_ansiEscapeRE;
    QRegularExpression m_oscSequenceRE;
    QRegularExpression m_termStatusRE;
    QRegularExpression m_controlCharsRE;
    
    // List of common non-file words that appear in terminal output
    QStringList m_nonFileWords;
};

#endif // TERMINALOUTPUTPROCESSOR_H


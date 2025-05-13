/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "conversationview.h"

#include <QDebug>
#include <QTextDocument>
#include <QTextBlock>
#include <QScrollBar>
#include <QBrush>
#include <QColor>
#include <QRegularExpression>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QDesktopServices>
#include <QTimer>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>

ConversationView::ConversationView(QWidget *parent)
    : QTextBrowser(parent)
    , m_inCodeBlock(false)
    , m_inAIResponse(false)
{
    initialize();
}

ConversationView::~ConversationView()
{
    // No specific cleanup needed
}

void ConversationView::initialize()
{
    // Set up the text browser
    setReadOnly(true);
    setOpenExternalLinks(false);
    setUndoRedoEnabled(false);
    
    // Connect signals
    connect(this, &QTextBrowser::anchorClicked, this, &ConversationView::onLinkClicked);
    
    // Initialize text formats
    m_commandFormat.setFontWeight(QFont::Bold);
    m_commandFormat.setForeground(QBrush(QColor(0, 128, 255))); // Blue
    
    m_outputFormat.setFontFamily(QStringLiteral("Monospace"));
    
    m_aiQueryFormat.setFontWeight(QFont::Bold);
    m_aiQueryFormat.setForeground(QBrush(QColor(75, 0, 130))); // Indigo
    
    m_aiResponseFormat.setForeground(QBrush(QColor(0, 100, 0))); // Dark green
    
    m_codeBlockFormat.setFontFamily(QStringLiteral("Monospace"));
    m_codeBlockFormat.setBackground(QBrush(QColor(240, 240, 240))); // Light gray
}

void ConversationView::addCommand(const QString &command, int blockId)
{
    qDebug() << "ConversationView: Adding command" << command << "with block ID" << blockId;
    
    // Create a new conversation item
    ConversationItem item;
    item.type = ConversationItem::Command;
    item.text = command;
    item.timestamp = QDateTime::currentDateTime();
    item.blockId = blockId;
    
    // Add to history
    m_history.append(item);
    
    // Add to the display
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);
    
    // Insert a new block if not at the start
    if (!cursor.atStart() && !cursor.atBlockStart()) {
        cursor.insertBlock();
    }
    
    // Format and insert the command
    cursor.setCharFormat(m_commandFormat);
    cursor.insertText(QStringLiteral("> %1").arg(command));
    cursor.setCharFormat(QTextCharFormat()); // Reset format
    
    // Ensure visible
    ensureCursorVisible();
}

void ConversationView::addCommandOutput(const QString &output, int blockId, int exitCode)
{
    qDebug() << "ConversationView: Adding command output for block ID" << blockId;
    
    // Create a new conversation item
    ConversationItem item;
    item.type = ConversationItem::Output;
    item.text = output;
    item.timestamp = QDateTime::currentDateTime();
    item.blockId = blockId;
    item.exitCode = exitCode;
    
    // Add to history
    m_history.append(item);
    
    // Add to the display
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);
    
    // Insert a new block
    cursor.insertBlock();
    
    // Format and insert the output
    cursor.setCharFormat(m_outputFormat);
    
    // Check if output contains HTML
    if (output.contains(QStringLiteral("<")) && output.contains(QStringLiteral(">"))) {
        // Insert as HTML
        cursor.insertHtml(output);
    } else {
        cursor.insertText(output);
    }
    
    cursor.setCharFormat(QTextCharFormat()); // Reset format
    
    // Add exit code info if command failed
    if (exitCode != 0) {
        cursor.insertBlock();
        QTextCharFormat errorFormat;
        errorFormat.setForeground(QBrush(QColor(200, 0, 0))); // Red
        cursor.setCharFormat(errorFormat);
        cursor.insertText(QStringLiteral("Command exited with code %1").arg(exitCode));
        cursor.setCharFormat(QTextCharFormat()); // Reset format
    }
    
    // Ensure visible
    ensureCursorVisible();
}

void ConversationView::addAIQuery(const QString &query)
{
    qDebug() << "ConversationView: Adding AI query" << query;
    
    // Create a new conversation item
    ConversationItem item;
    item.type = ConversationItem::AIQuery;
    item.text = query;
    item.timestamp = QDateTime::currentDateTime();
    
    // Add to history
    m_history.append(item);
    
    // Add to the display
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);
    
    // Insert a new block if not at the start
    if (!cursor.atStart() && !cursor.atBlockStart()) {
        cursor.insertBlock();
    }
    
    // Format and insert the query
    cursor.setCharFormat(m_aiQueryFormat);
    cursor.insertText(QStringLiteral("? %1").arg(query));
    cursor.setCharFormat(QTextCharFormat()); // Reset format
    
    // Ensure visible
    ensureCursorVisible();
}

void ConversationView::addAIResponse(const QString &response, bool isFinal)
{
    qDebug() << "ConversationView: Adding AI response, isFinal=" << isFinal;
    
    if (isFinal) {
        // Create a new conversation item for the full response
        ConversationItem item;
        item.type = ConversationItem::AIResponse;
        item.text = response;
        item.timestamp = QDateTime::currentDateTime();
        
        // Add to history
        m_history.append(item);
    }
    
    // Handle displaying the response
    QTextCursor cursor = textCursor();
    
    // If this is the first part of the response, add a header
    if (!m_inAIResponse) {
        cursor.movePosition(QTextCursor::End);
        setTextCursor(cursor);
        
        // Insert a new block
        cursor.insertBlock();
        
        // Add AI response header
        QTextCharFormat headerFormat = m_aiResponseFormat;
        headerFormat.setFontWeight(QFont::Bold);
        cursor.setCharFormat(headerFormat);
        cursor.insertText(QStringLiteral("AI Response:"));
        cursor.setCharFormat(QTextCharFormat()); // Reset format
        
        cursor.insertBlock();
        m_inAIResponse = true;
    }
    
    // Process the response for code blocks, etc.
    QString formattedResponse = processCodeBlocks(response);
    
    // Insert the formatted response
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);
    
    if (formattedResponse.contains(QStringLiteral("<")) && formattedResponse.contains(QStringLiteral(">"))) {
        // Insert as HTML
        cursor.insertHtml(formattedResponse);
    } else {
        // Normal text with appropriate formatting
        if (m_inCodeBlock) {
            cursor.setCharFormat(m_codeBlockFormat);
        } else {
            cursor.setCharFormat(m_aiResponseFormat);
        }
        cursor.insertText(formattedResponse);
    }
    
    // If this is the final part, end the response
    if (isFinal) {
        m_inAIResponse = false;
        m_inCodeBlock = false;
        
        // Add a blank line after the response
        cursor.insertBlock();
        
        // Add usage hint for first-time users
        static bool firstResponse = true;
        if (firstResponse) {
            cursor.insertBlock();
            QTextCharFormat hintFormat;
            hintFormat.setFontItalic(true);
            hintFormat.setForeground(QBrush(QColor(100, 100, 100))); // Gray
            cursor.setCharFormat(hintFormat);
            cursor.insertText(QStringLiteral("Tip: Select text in the response and use 'Insert to Editor' to paste it into your document."));
            cursor.setCharFormat(QTextCharFormat());
            firstResponse = false;
        }
    }
    
    // Ensure visible
    ensureCursorVisible();
}

void ConversationView::clearConversation()
{
    qDebug() << "ConversationView: Clearing conversation";
    
    // Clear the text browser
    clear();
    
    // Clear history
    m_history.clear();
    
    // Reset state
    m_inCodeBlock = false;
    m_inAIResponse = false;
    m_currentCodeBlockLanguage.clear();
}

bool ConversationView::saveToObsidian(const QString &vaultPath, const QString &filename)
{
    qDebug() << "ConversationView: Saving to Obsidian, vault path=" << vaultPath;
    
    // Check if vault path exists
    QDir vaultDir(vaultPath);
    if (!vaultDir.exists()) {
        qWarning() << "ConversationView: Obsidian vault path does not exist:" << vaultPath;
        return false;
    }
    
    // Generate filename if not provided
    QString actualFilename = filename;
    if (actualFilename.isEmpty()) {
        // Use default pattern: WarpKate-Chat-{date}
        actualFilename = QStringLiteral("WarpKate-Chat-%1")
                           .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd")));
    }
    
    // Ensure .md extension
    if (!actualFilename.endsWith(QStringLiteral(".md"))) {
        actualFilename += QStringLiteral(".md");
    }
    
    // Create full path
    QString filePath = vaultDir.filePath(actualFilename);
    
    // Generate markdown content
    QString markdown = toMarkdown();
    
    // Write to file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "ConversationView: Failed to open file for writing:" << filePath;
        return false;
    }
    
    QTextStream out(&file);
    out << markdown;
    file.close();
    
    qDebug() << "ConversationView: Saved to" << filePath;
    return true;
}

QString ConversationView::toMarkdown() const
{
    qDebug() << "ConversationView: Converting to markdown";
    
    QString markdown;
    markdown += QStringLiteral("# WarpKate Conversation\n\n");
    markdown += QStringLiteral("Date: %1\n\n").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
    
    // Process each history item
    for (const ConversationItem &item : m_history) {
        markdown += QStringLiteral("---\n\n");
        
        switch (item.type) {
            case ConversationItem::Command:
                markdown += QStringLiteral("### Command\n\n");
                markdown += QStringLiteral("```bash\n%1\n```\n\n").arg(item.text);
                break;
                
            case ConversationItem::Output:
                markdown += QStringLiteral("### Output\n\n");
                if (item.exitCode != 0) {
                    markdown += QStringLiteral("*Exit code: %1*\n\n").arg(item.exitCode);
                }
                markdown += QStringLiteral("```\n%1\n```\n\n").arg(item.text);
                break;
                
            case ConversationItem::AIQuery:
                markdown += QStringLiteral("### Query\n\n");
                markdown += QStringLiteral("%1\n\n").arg(item.text);
                break;
                
            case ConversationItem::AIResponse:
                markdown += QStringLiteral("### Response\n\n");
                markdown += item.text + QStringLiteral("\n\n");
                break;
        }
    }
    
    return markdown;
}

QList<ConversationItem> ConversationView::conversationHistory() const
{
    return m_history;
}

QString ConversationView::formatText(const QString &text)
{
    // Simple implementation - in a real version, this would process the text
    // more extensively for formatting
    return text;
}

QString ConversationView::processCodeBlocks(const QString &text)
{
    // Check for code block markers
    if (text.contains(QStringLiteral("```"))) {
        // Toggle code block state
        m_inCodeBlock = !m_inCodeBlock;
        
        // If we're entering a code block, look for language specification
        if (m_inCodeBlock) {
            // Extract language specification if present
            QRegularExpression langRe(QStringLiteral("```(\\w+)"));
            QRegularExpressionMatch match = langRe.match(text);
            if (match.hasMatch()) {
                m_currentCodeBlockLanguage = match.captured(1);
            } else {
                m_currentCodeBlockLanguage.clear();
            }
        }
    }
    
    // For now, just return the original text
    return text;
}

QTextCharFormat ConversationView::formatForItemType(ConversationItem::Type type)
{
    switch (type) {
        case ConversationItem::Command:
            return m_commandFormat;
        case ConversationItem::Output:
            return m_outputFormat;
        case ConversationItem::AIQuery:
            return m_aiQueryFormat;
        case ConversationItem::AIResponse:
            return m_aiResponseFormat;
        default:
            return QTextCharFormat();
    }
}

void ConversationView::onLinkClicked(const QUrl &url)
{
    qDebug() << "ConversationView: Link clicked:" << url.toString();
    
    // Handle file:// URLs differently
    if (url.scheme() == QStringLiteral("file")) {
        QString filePath = url.toLocalFile();
        QFileInfo fileInfo(filePath);
        
        if (!fileInfo.exists()) {
            qWarning() << "ConversationView: File does not exist:" << filePath;
            Q_EMIT operationComplete(QStringLiteral("File does not exist: %1").arg(filePath), false);
            return;
        }
        
        // Emit the filePathClicked signal for handling by the main application
        Q_EMIT filePathClicked(filePath, fileInfo.isDir());
        
        // Prevent the default URL opening behavior
        return;
    } 
    else if (url.scheme() == QStringLiteral("command")) {
        // Handle command:// URLs (for executing terminal commands)
        QString command = url.path();
        if (!command.isEmpty()) {
            Q_EMIT commandRequested(command);
        }
        return;
    }
    else if (url.scheme() == QStringLiteral("kate")) {
        // Handle kate:// URLs (for Kate editor actions)
        Q_EMIT kateActionRequested(url.toString());
        return;
    }
    else {
        // For other URLs, use the default handler
        QDesktopServices::openUrl(url);
    }
}


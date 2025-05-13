/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CONVERSATIONVIEW_H
#define CONVERSATIONVIEW_H

#include <QTextBrowser>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QDateTime>
#include <QUrl>

/**
 * @brief The ConversationItem struct
 * 
 * Represents a single item in the conversation (command, output, or AI response)
 */
struct ConversationItem {
    enum Type {
        Command,    ///< Terminal command
        Output,     ///< Command output
        AIQuery,    ///< Query to AI assistant
        AIResponse  ///< Response from AI assistant
    };
    
    Type type;                  ///< Type of the conversation item
    QString text;               ///< Text content
    QDateTime timestamp;        ///< When the item was created
    int blockId;                ///< ID of the associated command block (if applicable)
    int exitCode;               ///< Exit code of command (if applicable)
};

/**
 * @brief The ConversationView class
 * 
 * This class manages the display of command and AI interactions in the terminal view.
 * It handles formatting, styling, and managing the conversation history.
 */
class ConversationView : public QTextBrowser
{
    Q_OBJECT

public:
    /**
     * Constructor
     * @param parent Parent widget
     */
    explicit ConversationView(QWidget *parent = nullptr);
    
    /**
     * Destructor
     */
    ~ConversationView();
    
    /**
     * Add a command to the conversation
     * @param command Command text
     * @param blockId Associated block ID
     */
    void addCommand(const QString &command, int blockId = -1);
    
    /**
     * Add command output to the conversation
     * @param output Output text
     * @param blockId Associated block ID
     * @param exitCode Command exit code
     */
    void addCommandOutput(const QString &output, int blockId = -1, int exitCode = 0);
    
    /**
     * Add an AI query to the conversation
     * @param query Query text
     */
    void addAIQuery(const QString &query);
    
    /**
     * Add an AI response to the conversation
     * @param response Response text
     * @param isFinal Whether this is the final (complete) response
     */
    void addAIResponse(const QString &response, bool isFinal = true);
    
    /**
     * Clear the conversation view
     */
    void clearConversation();
    
    /**
     * Save the conversation to an Obsidian markdown file
     * @param vaultPath Path to the Obsidian vault
     * @param filename Optional filename (without extension)
     * @return True if the operation was successful
     */
    bool saveToObsidian(const QString &vaultPath, const QString &filename = QString());
    
    /**
     * Get the current conversation as markdown text
     * @return Markdown formatted conversation
     */
    QString toMarkdown() const;
    
    /**
     * Get the conversation history items
     * @return List of conversation items
     */
    QList<ConversationItem> conversationHistory() const;

public Q_SLOTS:
    /**
     * Handle link clicked event in the conversation view
     * @param url The URL that was clicked
     */
    void onLinkClicked(const QUrl &url);
    
Q_SIGNALS:
    /**
     * Signal emitted when a file path is clicked
     * @param filePath Path to the file
     * @param isDirectory Whether the path is a directory
     */
    void filePathClicked(const QString &filePath, bool isDirectory);
    
    /**
     * Signal emitted when a command execution is requested
     * @param command The command to execute
     */
    void commandRequested(const QString &command);
    
    /**
     * Signal emitted when a Kate editor action is requested
     * @param action The action to perform
     */
    void kateActionRequested(const QString &action);
    
    /**
     * Signal emitted when an operation is completed
     * @param message Message describing the result
     * @param success Whether the operation was successful
     */
    void operationComplete(const QString &message, bool success);
    
private:
    /**
     * Initialize the component
     */
    void initialize();
    
    /**
     * Format special elements in text (code blocks, links, etc.)
     * @param text Input text to format
     * @return Formatted text
     */
    QString formatText(const QString &text);
    
    /**
     * Process code blocks in text
     * @param text Text containing code blocks
     * @return Processed text with formatted code blocks
     */
    QString processCodeBlocks(const QString &text);
    
    /**
     * Create a character format for a specific item type
     * @param type Type of conversation item
     * @return Formatting for the item
     */
    QTextCharFormat formatForItemType(ConversationItem::Type type);
    
    // Store conversation history
    QList<ConversationItem> m_history;
    
    // Formatting properties
    QTextCharFormat m_commandFormat;
    QTextCharFormat m_outputFormat;
    QTextCharFormat m_aiQueryFormat;
    QTextCharFormat m_aiResponseFormat;
    QTextCharFormat m_codeBlockFormat;
    
    // Current state
    bool m_inCodeBlock;
    bool m_inAIResponse;
    QString m_currentCodeBlockLanguage;
};

#endif // CONVERSATIONVIEW_H


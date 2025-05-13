/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WARPKATEVIEW_H
#define WARPKATEVIEW_H

#include <KTextEditor/MainWindow>
#include <KTextEditor/View>
#include <KTextEditor/Document>
#include <KXMLGUIClient>

#include <QObject>
#include <QWidget>
#include <QDockWidget>
#include <QTextEdit>
#include <QTextBrowser>
#include <QLineEdit>
#include <QToolBar>
#include <QLabel>
#include <QToolButton>
#include <QMenu>
#include <QMimeDatabase>
#include <QDesktopServices>
#include <QApplication>
#include <QClipboard>
#include "aiservice.h"

class WarpKatePlugin;
class TerminalEmulator;
class BlockModel;
// We don't use TerminalBlockView in the simplified interface
class QAction;

/**
 * WarpKateView - Plugin view for the WarpKate plugin
 * This class represents the view of the plugin within a Kate main window
 */
class WarpKateView : public QObject, public KXMLGUIClient
{
    Q_OBJECT

public:
    /**
     * Constructor
     */
    WarpKateView(WarpKatePlugin *plugin, KTextEditor::MainWindow *mainWindow);
    
    /**
     * Destructor
     */
    ~WarpKateView() override;

public Q_SLOTS:
    /**
     * Show the terminal view
     */
    void showTerminal();
    
    /**
     * Hide the terminal view
     */
    void hideTerminal();
    
    /**
     * Toggle the terminal view
     */
    void toggleTerminal();
    
    /**
     * Execute the current line or selection as a command
     */
    void executeCurrentText();
    
    /**
     * Clear the terminal
     */
    void clearTerminal();
    
    /**
     * Navigate to the previous command block
     */
    void previousBlock();
    
    /**
     * Navigate to the next command block
     */
    void nextBlock();
    
    /**
     * Handle current document changed
     */
    void onDocumentChanged(KTextEditor::Document *document);
    
private:
    /**
     * Initialize the UI components
     */
    void setupUI();
    
    /**
     * Initialize the actions
     */
    void setupActions();
    
    /**
     * Create and set up the terminal widget
     */
    void setupTerminal();
    
    /**
     * Execute a command in the terminal
     * @param command Command to execute
     */
    void executeCommand(const QString &command);
    
    /**
     * Handle AI query input
     * @param query The query text
     */
    void handleAIQuery(const QString &query);
    
    /**
     * Insert selected text into editor
     */
    void insertToEditor();
    
    /**
     * Save current conversation to Obsidian
     */
    void saveToObsidian();
    
    /**
     * Check code in current editor
     */
    void checkCode();
    
    /**
     * Show preferences dialog
     */
    void showPreferences();
    
    /**
     * Handle input text submission
     */
    void submitInput();
    
    /**
     * Get the current text from the editor (current line or selection)
     * @return Current text
     */
    QString getCurrentText();
    
    /**
     * Get context information from the current document and environment
     * @return Context information as text
     */
    QString getContextInformation();
    
    /**
     * Generate an AI response to a query
     * @param query The user's query
     * @param contextInfo Additional context information
     */
    void generateAIResponse(const QString &query, const QString &contextInfo);
    
    /**
     * Handle AI response from the service
     * @param response The AI response text
     * @param isFinal Whether this is the final (complete) response
     */
    void handleAIResponse(const QString &response, bool isFinal);
    
    /**
     * Set up the AI service
     */
    void setupAIService();
    
    /**
     * Clean and filter terminal output for display
     * Removes ANSI escape sequences and control characters
     * @param rawOutput The raw terminal output
     * @return Cleaned output ready for display
     */
    QString cleanTerminalOutput(const QString &rawOutput);
    
    /**
     * Process terminal output for interactive features
     * - Detects file and directory listings
     * - Adds formatting and interactivity
     * @param output Terminal output to process
     * @return Formatted output with interactive features
     */
    QString processTerminalOutputForInteractivity(const QString &output);
    
    /**
     * Detect and parse file listings in terminal output
     * @param line Line of terminal output to parse
     * @param workingDir Current working directory
     * @return True if the line was processed as a file listing
     */
    bool processFileListingLine(const QString &line, const QString &workingDir);
    
    /**
     * Create a context menu for interactive file/directory items
     * @param filePath Full path to the file/directory
     * @param isDirectory Whether the item is a directory
     * @return Context menu for the item
     */
    QMenu* createFileContextMenu(const QString &filePath, bool isDirectory);
    
    /**
     * Handle clicking on an interactive file/directory item
     * @param filePath Full path to the file/directory
     * @param isDirectory Whether the item is a directory
     */
    void handleFileItemClicked(const QString &filePath, bool isDirectory);
    
    /**
     * Open a file with the appropriate application
     * @param filePath Full path to the file
     */
    void openFile(const QString &filePath);
    
    /**
     * Open a directory in the file manager
     * @param dirPath Full path to the directory
     */
    void openDirectory(const QString &dirPath);
    
    /**
     * Open a file in Kate editor
     * @param filePath Full path to the file
     */
    void openFileInKate(const QString &filePath);
    
    /**
     * Copy a file path to clipboard
     * @param filePath Full path to copy
     */
    void copyPathToClipboard(const QString &filePath);
    
    /**
     * Execute a file safely
     * @param filePath Full path to the file to execute
     */
    void executeFile(const QString &filePath);
    
    /**
     * Check if a file is executable
     * @param filePath Full path to the file
     * @return True if the file is executable
     */
    bool isExecutable(const QString &filePath);
    
    /**
     * Check if a filename represents a directory
     * @param filename Filename to check
     * @param output Full ls command output for reference
     * @return True if the filename represents a directory
     */
    bool isDirectory(const QString &filename, const QString &output);
    
    /**
     * Detect file type based on extension
     * @param filename Filename to check
     * @return Type of file (text, image, etc.)
     */
    QString detectFileType(const QString &filename);

    /**
     * Process detailed file listing (ls -l format) to add interactivity
     * @param output Terminal output to process
     * @param workingDir Current working directory
     * @return Formatted HTML output with interactive elements
     */
    QString processDetailedListing(const QString &output, const QString &workingDir);

    /**
     * Process simple file listing (ls format) to add interactivity
     * @param output Terminal output to process
     * @param workingDir Current working directory
     * @return Formatted HTML output with interactive elements
     */
    QString processSimpleListing(const QString &output, const QString &workingDir);

    /**
     * Handle clicking on links in the terminal output
     * @param url The URL that was clicked
     */
    void onLinkClicked(const QUrl &url);

    /**
     * Refresh UI elements from current settings
     */
    void refreshUIFromSettings();
    
private Q_SLOTS:
    /**
     * Handle terminal output
     * @param output Terminal output text
     */
    void onTerminalOutput(const QString &output);
    
    /**
     * Handle command execution completion
     * @param command The executed command
     * @param output Command output
     * @param exitCode Command exit code
     */
    void onCommandExecuted(const QString &command, const QString &output, int exitCode);
    
    /**
     * Handle command detection
     * @param command Detected command text
     */
    void onCommandDetected(const QString &command);
    
    /**
     * Handle working directory changes
     * @param directory New working directory
     */
    void onWorkingDirectoryChanged(const QString &directory);
    
    /**
     * Handle shell process termination
     * @param exitCode Shell exit code
     */
    void onShellFinished(int exitCode);
    
protected:
    /**
     * Event filter for handling keyboard events in the input area
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    // For interactive element highlighting
    int m_currentFocusIndex = -1;
    QList<QTextEdit::ExtraSelection> m_interactiveElements;
    QTimer *m_clickFeedbackTimer = nullptr;
    int m_lastClickedIndex = -1;
    
    // Methods for interactive element handling
    void updateInteractiveElements();
    void focusNextInteractiveElement();
    void focusPreviousInteractiveElement();
    void applyInteractiveElementStyles();
    void flashClickFeedback(int elementIndex);
    void navigateCommandHistory(int direction);
    WarpKatePlugin *m_plugin;
    KTextEditor::MainWindow *m_mainWindow;
    
    // UI components
    QWidget *m_toolView;
    QWidget *m_terminalWidget;
    QTextBrowser *m_conversationArea;
    QTextEdit *m_promptInput;
    QToolBar *m_toolbar;
    QLabel *m_inputModeLabel;
    QToolButton *m_inputModeToggle;
    QToolButton *m_aiModeToggle;
    QLabel *m_modeIconLabel; // For displaying the mode icon
    QIcon m_terminalIcon;    // Terminal mode icon
    QIcon m_aiIcon;          // AI mode icon
    
    // Terminal components
    TerminalEmulator *m_terminalEmulator;
    BlockModel *m_blockModel;
    
    // Actions
    QAction *m_showTerminalAction;
    QAction *m_executeAction;
    QAction *m_clearAction;
    QAction *m_insertToEditorAction;
    QAction *m_saveToObsidianAction;
    QAction *m_checkCodeAction;
    
    // State variables
    int m_currentBlockId;
    
    // State
    bool m_terminalVisible;
    
    // AI service
    AIService *m_aiService;
    
    // Command history variables
    int m_historyIndex;
    QString m_savedPartialCommand;
    
private Q_SLOTS:
    /**
     * Handle input mode toggle changes
     */
    void onInputModeToggled(bool aiMode);
    void onModeButtonClicked(bool aiMode);
};

#endif // WARPKATEVIEW_H



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
#include <QLineEdit>
#include <QToolBar>
#include <QLabel>
#include <QToolButton>

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
    WarpKatePlugin *m_plugin;
    KTextEditor::MainWindow *m_mainWindow;
    
    // UI components
    QWidget *m_toolView;
    QWidget *m_terminalWidget;
    QTextEdit *m_conversationArea;
    QTextEdit *m_promptInput;
    QToolBar *m_toolbar;
    QLabel *m_inputModeLabel;
    QToolButton *m_inputModeToggle;
    
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
    
private Q_SLOTS:
    /**
     * Handle input mode toggle changes
     */
    void onInputModeToggled(bool aiMode);
};

#endif // WARPKATEVIEW_H


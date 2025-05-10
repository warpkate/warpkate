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

class WarpKatePlugin;
class TerminalEmulator;
class BlockModel;
class QAction;
class QToolButton;

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

private:
    WarpKatePlugin *m_plugin;
    KTextEditor::MainWindow *m_mainWindow;
    
    // UI components
    QDockWidget *m_dockWidget;
    QWidget *m_terminalWidget;
    
    // Terminal components
    TerminalEmulator *m_terminalEmulator;
    BlockModel *m_blockModel;
    
    // Actions
    QAction *m_showTerminalAction;
    QAction *m_executeAction;
    QAction *m_clearAction;
    QAction *m_prevBlockAction;
    QAction *m_nextBlockAction;
    
    // State
    bool m_terminalVisible;
};

#endif // WARPKATEVIEW_H


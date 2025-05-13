/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TERMINALBLOCKVIEW_H
#define TERMINALBLOCKVIEW_H

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHash>
#include <QPair>

class QLabel;
class QTextEdit;
class QLineEdit;
class QPushButton;
class QMenu;
class QAction;
class QTimer;
class QResizeEvent;
class QKeyEvent;
class QFocusEvent;
class QContextMenuEvent;

class TerminalEmulator;
class BlockModel;
class CommandBlock;

/**
 * Terminal block view widget
 * 
 * This widget displays terminal content in a block-based view, similar to
 * Warp Terminal. It integrates with BlockModel and TerminalEmulator classes
 * to display command blocks and terminal output.
 */
class TerminalBlockView : public QWidget
{
    Q_OBJECT
    
public:
    /**
     * Constructor
     * @param parent Parent widget
     */
    explicit TerminalBlockView(QWidget *parent = nullptr);
    
    /**
     * Destructor
     */
    ~TerminalBlockView() override;
    
    /**
     * Set the terminal emulator to use
     * @param terminal Terminal emulator
     */
    void setTerminalEmulator(TerminalEmulator *terminal);
    
    /**
     * Set the block model to use
     * @param model Block model
     */
    void setBlockModel(BlockModel *model);
    
    /**
     * Get the terminal emulator
     * @return Terminal emulator
     */
    TerminalEmulator *terminalEmulator() const;
    
    /**
     * Get the block model
     * @return Block model
     */
    BlockModel *blockModel() const;
    
    /**
     * Execute a command
     * @param command Command to execute
     */
    void executeCommand(const QString &command);
    
    /**
     * Navigate to a specific block
     * @param blockId Block ID to navigate to
     */
    void navigateToBlock(int blockId);
    
    /**
     * Navigate to the next block
     */
    void navigateToNextBlock();
    
    /**
     * Navigate to the previous block
     */
    void navigateToPreviousBlock();
    
    /**
     * Find text in blocks
     * @param text Text to find
     * @param searchForward Whether to search forward
     * @return True if text was found
     */
    bool findText(const QString &text, bool searchForward = true);
    
    /**
     * Set focus to the command input
     */
    void focusCommandInput();
    
    /**
     * Clear the terminal view
     */
    void clear();
    
protected:
    /**
     * Handle resize events
     * @param event Resize event
     */
    void resizeEvent(QResizeEvent *event) override;
    
    /**
     * Handle key press events
     * @param event Key event
     */
    void keyPressEvent(QKeyEvent *event) override;
    
    /**
     * Handle focus events
     * @param event Focus event
     */
    void focusInEvent(QFocusEvent *event) override;
    
    /**
     * Handle context menu events
     * @param event Context menu event
     */
    void contextMenuEvent(QContextMenuEvent *event) override;
    
private:
    /**
     * Initialize the UI components
     */
    void setupUI();
    
    /**
     * Create a block widget for a command block
     * @param block Command block
     * @return Block widget
     */
    QWidget *createBlockWidget(const CommandBlock &block);
    
    /**
     * Update a block widget for a command block
     * @param blockId Block ID
     */
    void updateBlockWidget(int blockId);
    
    /**
     * Remove a block widget
     * @param blockId Block ID
     */
    void removeBlockWidget(int blockId);
    
    /**
     * Create the context menu
     * @return Context menu
     */
    QMenu *createContextMenu();

private Q_SLOTS:
    /**
     * Handle current block changed in the model
     * @param blockId New current block ID
     */
    void onCurrentBlockChanged(int blockId);
    
    /**
     * Handle block created in the model
     * @param blockId New block ID
     */
    void onBlockCreated(int blockId);
    
    /**
     * Handle block state changed in the model
     * @param blockId Block ID
     * @param state New state
     */
    void onBlockStateChanged(int blockId, int state);
    
    /**
     * Handle block changed in the model
     * @param blockId Block ID
     */
    void onBlockChanged(int blockId);
    
    /**
     * Handle command input return pressed
     */
    void onCommandInputReturnPressed();
    
    /**
     * Handle execute button clicked
     */
    void onExecuteButtonClicked();
    
    /**
     * Handle terminal redraw required
     */
    void onTerminalRedrawRequired();
    
    /**
     * Handle cursor blink timer
     */
    void onCursorBlinkTimer();
    
    /**
     * Handle copy action
     */
    void onCopyAction();
    
    /**
     * Handle paste action
     */
    void onPasteAction();
    
    /**
     * Handle clear action
     */
    void onClearAction();
    
Q_SIGNALS:
    /**
     * Emitted when a command is executed
     * @param command Command text
     */
    void commandExecuted(const QString &command);
    
    /**
     * Emitted when a block is selected
     * @param blockId Block ID
     */
    void blockSelected(int blockId);
    
    /**
     * Emitted when the view needs to be scrolled
     * @param position Scroll position
     */
    void scrollPositionChanged(int position);
    
private:
    TerminalEmulator *m_terminal;                         ///< Terminal emulator
    BlockModel *m_model;                                  ///< Block model
    
    QScrollArea *m_scrollArea;                           ///< Scroll area for blocks
    QWidget *m_blockContainer;                           ///< Container for block widgets
    QVBoxLayout *m_blockLayout;                          ///< Layout for block widgets
    QLineEdit *m_commandInput;                           ///< Command input field
    QPushButton *m_executeButton;                        ///< Execute button
    
    QHash<int, QWidget*> m_blockWidgets;                 ///< Map of block ID to block widget
    QHash<int, QPair<QLabel*, QTextEdit*>> m_blockParts; ///< Map of block ID to block parts
    
    QTimer *m_cursorBlinkTimer;                          ///< Timer for cursor blinking
    bool m_cursorVisible;                                ///< Whether the cursor is visible
    
    int m_currentBlockId;                                ///< Current block ID
    bool m_isInitialized;                                ///< Whether the view is initialized
};

#endif // TERMINALBLOCKVIEW_H


/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "terminalblockview.h"
#include "blockmodel.h"
#include "terminalemulator.h"

#include <QMessageBox>

#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QContextMenuEvent>
#include <QScrollBar>
#include <QClipboard>
#include <QApplication>
#include <QDateTime>
#include <QStyle>
#include <QStyleOption>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QDebug>

// Block widget styles
static const QString COMMAND_STYLE = 
    "QLabel { "
    "   color: #eee; "
    "   background-color: #444; "
    "   padding: 4px 8px; "
    "   border-top-left-radius: 4px; "
    "   border-top-right-radius: 4px; "
    "   font-family: monospace; "
    "   font-weight: bold; "
    "}";

static const QString OUTPUT_STYLE_BASE = 
    "QTextEdit { "
    "   color: #ddd; "
    "   background-color: #333; "
    "   border: none; "
    "   padding: 8px; "
    "   font-family: monospace; "
    "}";

static const QString OUTPUT_STYLE_COMPLETED = 
    "QTextEdit { "
    "   color: #ddd; "
    "   background-color: #333; "
    "   border: none; "
    "   padding: 8px; "
    "   font-family: monospace; "
    "}";

static const QString OUTPUT_STYLE_FAILED = 
    "QTextEdit { "
    "   color: #ddd; "
    "   background-color: #3a2a2a; "
    "   border: none; "
    "   padding: 8px; "
    "   font-family: monospace; "
    "}";

static const QString COMMAND_INPUT_STYLE = 
    "QLineEdit { "
    "   color: #eee; "
    "   background-color: #444; "
    "   border: 1px solid #555; "
    "   border-radius: 4px; "
    "   padding: 4px 8px; "
    "   font-family: monospace; "
    "}";

static const QString EXECUTE_BUTTON_STYLE = 
    "QPushButton { "
    "   color: #eee; "
    "   background-color: #455; "
    "   border: none; "
    "   border-radius: 4px; "
    "   padding: 4px 8px; "
    "   font-weight: bold; "
    "} "
    "QPushButton:hover { "
    "   background-color: #566; "
    "} "
    "QPushButton:pressed { "
    "   background-color: #677; "
    "}";

TerminalBlockView::TerminalBlockView(QWidget *parent)
    : QWidget(parent)
    , m_terminal(nullptr)
    , m_model(nullptr)
    , m_scrollArea(nullptr)
    , m_blockContainer(nullptr)
    , m_blockLayout(nullptr)
    , m_commandInput(nullptr)
    , m_executeButton(nullptr)
    , m_cursorBlinkTimer(nullptr)
    , m_cursorVisible(true)
    , m_currentBlockId(-1)
    , m_isInitialized(false)
{
    setupUI();
    
    // Create cursor blink timer
    m_cursorBlinkTimer = new QTimer(this);
    connect(m_cursorBlinkTimer, &QTimer::timeout, this, &TerminalBlockView::onCursorBlinkTimer);
    m_cursorBlinkTimer->start(500);
    
    m_isInitialized = true;
}

TerminalBlockView::~TerminalBlockView()
{
    // Clean up block widgets
    qDeleteAll(m_blockWidgets);
    m_blockWidgets.clear();
    m_blockParts.clear();
}

void TerminalBlockView::setTerminalEmulator(TerminalEmulator *terminal)
{
    if (m_terminal == terminal) {
        return;
    }
    
    // Disconnect from old terminal if any
    if (m_terminal) {
        disconnect(m_terminal, nullptr, this, nullptr);
    }
    
    m_terminal = terminal;
    
    if (m_terminal) {
        // Connect signals from terminal
        connect(m_terminal, &TerminalEmulator::redrawRequired, this, &TerminalBlockView::onTerminalRedrawRequired);
    }
}

void TerminalBlockView::setBlockModel(BlockModel *model)
{
    if (m_model == model) {
        return;
    }
    
    // Disconnect from old model if any
    if (m_model) {
        disconnect(m_model, nullptr, this, nullptr);
        
        // Clear existing block widgets
        qDeleteAll(m_blockWidgets);
        m_blockWidgets.clear();
        m_blockParts.clear();
    }
    
    m_model = model;
    
    if (m_model) {
        // Connect signals from model
        connect(m_model, &BlockModel::currentBlockChanged, this, &TerminalBlockView::onCurrentBlockChanged);
        connect(m_model, &BlockModel::blockCreated, this, &TerminalBlockView::onBlockCreated);
        connect(m_model, &BlockModel::blockStateChanged, this, &TerminalBlockView::onBlockStateChanged);
        connect(m_model, &BlockModel::blockChanged, this, &TerminalBlockView::onBlockChanged);
        
        // Create widgets for existing blocks
        QList<CommandBlock> blocks = m_model->blocks();
        for (const CommandBlock &block : blocks) {
            QWidget *blockWidget = createBlockWidget(block);
            m_blockLayout->addWidget(blockWidget);
            m_blockWidgets.insert(block.id, blockWidget);
        }
        
        // Update current block
        int currentBlockId = m_model->currentBlockId();
        if (currentBlockId >= 0) {
            onCurrentBlockChanged(currentBlockId);
        }
    }
}

TerminalEmulator *TerminalBlockView::terminalEmulator() const
{
    return m_terminal;
}

BlockModel *TerminalBlockView::blockModel() const
{
    return m_model;
}

void TerminalBlockView::executeCommand(const QString &command)
{
    if (!m_terminal || !m_model || command.isEmpty()) {
        return;
    }
    
    // Execute the command through the model (which will use the terminal)
    m_model->executeCommand(command);
    
    // Clear command input
    m_commandInput->clear();
    
    // Emit signal
    emit commandExecuted(command);
    
    // Scroll to bottom
    QTimer::singleShot(100, this, [this]() {
        m_scrollArea->verticalScrollBar()->setValue(m_scrollArea->verticalScrollBar()->maximum());
    });
}

void TerminalBlockView::navigateToBlock(int blockId)
{
    if (!m_model) {
        return;
    }
    
    // Set current block in model
    if (m_model->setCurrentBlock(blockId)) {
        // Scroll to block widget
        QWidget *blockWidget = m_blockWidgets.value(blockId);
        if (blockWidget) {
            m_scrollArea->ensureWidgetVisible(blockWidget);
        }
        
        // Emit signal
        emit blockSelected(blockId);
    }
}

void TerminalBlockView::navigateToNextBlock()
{
    if (!m_model) {
        return;
    }
    
    // Navigate to next block in model
    if (m_model->navigateToNextBlock()) {
        // Scroll to current block
        QWidget *blockWidget = m_blockWidgets.value(m_model->currentBlockId());
        if (blockWidget) {
            m_scrollArea->ensureWidgetVisible(blockWidget);
        }
    }
}

void TerminalBlockView::navigateToPreviousBlock()
{
    if (!m_model) {
        return;
    }
    
    // Navigate to previous block in model
    if (m_model->navigateToPreviousBlock()) {
        // Scroll to current block
        QWidget *blockWidget = m_blockWidgets.value(m_model->currentBlockId());
        if (blockWidget) {
            m_scrollArea->ensureWidgetVisible(blockWidget);
        }
    }
}

bool TerminalBlockView::findText(const QString &text, bool searchForward)
{
    if (!m_model || text.isEmpty()) {
        return false;
    }
    
    // Find text in model
    int blockId = m_model->findText(text, m_model->currentBlockId(), searchForward);
    if (blockId >= 0) {
        // Navigate to found block
        navigateToBlock(blockId);
        return true;
    }
    
    return false;
}

void TerminalBlockView::focusCommandInput()
{
    if (m_commandInput) {
        m_commandInput->setFocus();
    }
}

void TerminalBlockView::clear()
{
    if (!m_model) {
        return;
    }
    
    // Clear model
    m_model->clear();
    
    // Clear block widgets
    qDeleteAll(m_blockWidgets);
    m_blockWidgets.clear();
    m_blockParts.clear();
}

void TerminalBlockView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    
    // Adjust block widgets width
    for (QWidget *blockWidget : m_blockWidgets) {
        blockWidget->setMinimumWidth(m_scrollArea->viewport()->width() - 20);
    }
}

void TerminalBlockView::keyPressEvent(QKeyEvent *event)
{
    // Handle navigation keys
    switch (event->key()) {
        case Qt::Key_Up:
            if (event->modifiers() & Qt::AltModifier) {
                navigateToPreviousBlock();
                event->accept();
                return;
            }
            break;
            
        case Qt::Key_Down:
            if (event->modifiers() & Qt::AltModifier) {
                navigateToNextBlock();
                event->accept();
                return;
            }
            break;
            
        case Qt::Key_F:
            if (event->modifiers() & Qt::ControlModifier) {
                // TODO: Implement find dialog
                event->accept();
                return;
            }
            break;
            
        case Qt::Key_C:
            if (event->modifiers() & Qt::ControlModifier) {
                onCopyAction();
                event->accept();
                return;
            }
            break;
            
        case Qt::Key_V:
            if (event->modifiers() & Qt::ControlModifier) {
                onPasteAction();
                event->accept();
                return;
            }
            break;
            
        case Qt::Key_L:
            if (event->modifiers() & Qt::ControlModifier) {
                onClearAction();
                event->accept();
                return;
            }
            break;
    }
    
    // Pass unhandled keys to parent
    QWidget::keyPressEvent(event);
}

void TerminalBlockView::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    
    // Focus command input when the view gets focus
    focusCommandInput();
}

void TerminalBlockView::contextMenuEvent(QContextMenuEvent *event)
{
    // Show context menu
    QMenu *menu = createContextMenu();
    menu->exec(event->globalPos());
    delete menu;
    
    event->accept();
}

void TerminalBlockView::setupUI()
{
    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Scroll area for blocks
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    // Container for blocks
    m_blockContainer = new QWidget(m_scrollArea);
    m_blockContainer->setObjectName("blockContainer");
    m_blockContainer->setStyleSheet("#blockContainer { background-color: #2a2a2a; }");
    
    // Layout for blocks
    m_blockLayout = new QVBoxLayout(m_blockContainer);
    m_blockLayout->setContentsMargins(8, 8, 8, 8);
    m_blockLayout->setSpacing(12);
    m_blockLayout->addStretch();
    
    m_scrollArea->setWidget(m_blockContainer);
    mainLayout->addWidget(m_scrollArea);
    
    // Input area
    QWidget *inputWidget = new QWidget(this);
    QHBoxLayout *inputLayout = new QHBoxLayout(inputWidget);
    inputLayout->setContentsMargins(8, 4, 8, 8);
    inputLayout->setSpacing(8);
    
    // Command prompt
    QLabel *promptLabel = new QLabel("$", inputWidget);
    promptLabel->setStyleSheet("QLabel { color: #6a6; font-family: monospace; font-weight: bold; }");
    
    // Command input
    m_commandInput = new QLineEdit(inputWidget);
    m_commandInput->setStyleSheet(COMMAND_INPUT_STYLE);
    m_commandInput->setPlaceholderText(tr("Enter command..."));
    connect(m_commandInput, &QLineEdit::returnPressed, this, &TerminalBlockView::onCommandInputReturnPressed);
    
    // Execute button
    m_executeButton = new QPushButton(tr("Run"), inputWidget);
    m_executeButton->setStyleSheet(EXECUTE_BUTTON_STYLE);
    connect(m_executeButton, &QPushButton::clicked, this, &TerminalBlockView::onExecuteButtonClicked);
    
    inputLayout->addWidget(promptLabel);
    inputLayout->addWidget(m_commandInput, 1);
    inputLayout->addWidget(m_executeButton);
    
    mainLayout->addWidget(inputWidget);
    
    // Set focus policy
    setFocusPolicy(Qt::StrongFocus);
    
    // Initial focus to command input
    m_commandInput->setFocus();
}

QWidget *TerminalBlockView::createBlockWidget(const CommandBlock &block)
{
    // Create container widget for the block
    QWidget *blockWidget = new QWidget(m_blockContainer);
    blockWidget->setObjectName(QString("block_%1").arg(block.id));
    
    // Create layout for the block
    QVBoxLayout *blockLayout = new QVBoxLayout(blockWidget);
    blockLayout->setContentsMargins(0, 0, 0, 0);
    blockLayout->setSpacing(0);
    
    // Create command label (prompt + command text)
    QString commandLabelText = QString("$ %1").arg(block.command);
    QLabel *commandLabel = new QLabel(commandLabelText, blockWidget);
    commandLabel->setStyleSheet(COMMAND_STYLE);
    commandLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    
    // Create output text edit
    QTextEdit *outputTextEdit = new QTextEdit(blockWidget);
    outputTextEdit->setReadOnly(true);
    outputTextEdit->setLineWrapMode(QTextEdit::WidgetWidth);
    outputTextEdit->setStyleSheet(OUTPUT_STYLE_BASE);
    outputTextEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    outputTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    // Set output text
    outputTextEdit->setPlainText(block.output);
    
    // Adjust text edit height to content
    QFontMetrics metrics(outputTextEdit->font());
    int lineCount = block.output.count('\n') + 1;
    int contentHeight = metrics.height() * lineCount + 20;
    outputTextEdit->setFixedHeight(qMin(contentHeight, 400));
    
    // Apply style based on block state
    switch (block.state) {
        case CommandBlock::Running:
            outputTextEdit->setStyleSheet(OUTPUT_STYLE_BASE);
            break;
        case CommandBlock::Completed:
            outputTextEdit->setStyleSheet(OUTPUT_STYLE_COMPLETED);
            break;
        case CommandBlock::Failed:
            outputTextEdit->setStyleSheet(OUTPUT_STYLE_FAILED);
            break;
    }
    
    // Add widgets to layout
    blockLayout->addWidget(commandLabel);
    blockLayout->addWidget(outputTextEdit);
    
    // Store block parts for later updates
    m_blockParts.insert(block.id, qMakePair(commandLabel, outputTextEdit));
    
    // Set minimum width
    blockWidget->setMinimumWidth(m_scrollArea->viewport()->width() - 20);
    
    // Add highlight for current block
    if (block.id == m_currentBlockId) {
        blockWidget->setStyleSheet("QWidget { border-left: 3px solid #6a6; }");
    }
    
    return blockWidget;
}

void TerminalBlockView::updateBlockWidget(int blockId)
{
    if (!m_model || !m_blockWidgets.contains(blockId)) {
        return;
    }
    
    // Get block
    CommandBlock block = m_model->block(blockId);
    
    // Get block parts
    QPair<QLabel*, QTextEdit*> parts = m_blockParts.value(blockId);
    QLabel *commandLabel = parts.first;
    QTextEdit *outputTextEdit = parts.second;
    
    // Update command text
    commandLabel->setText(QString("$ %1").arg(block.command));
    
    // Update output text
    outputTextEdit->setPlainText(block.output);
    
    // Adjust text edit height to content
    QFontMetrics metrics(outputTextEdit->font());
    int lineCount = block.output.count('\n') + 1;
    int contentHeight = metrics.height() * lineCount + 20;
    outputTextEdit->setFixedHeight(qMin(contentHeight, 400));
    
    // Apply style based on block state
    switch (block.state) {
        case CommandBlock::Running:
            outputTextEdit->setStyleSheet(OUTPUT_STYLE_BASE);
            break;
        case CommandBlock::Completed:
            outputTextEdit->setStyleSheet(OUTPUT_STYLE_COMPLETED);
            break;
        case CommandBlock::Failed:
            outputTextEdit->setStyleSheet(OUTPUT_STYLE_FAILED);
            break;
    }
    
    // Update highlight for current block
    if (blockId == m_currentBlockId) {
        m_blockWidgets[blockId]->setStyleSheet("QWidget { border-left: 3px solid #6a6; }");
    } else {
        m_blockWidgets[blockId]->setStyleSheet("");
    }
}

void TerminalBlockView::removeBlockWidget(int blockId)
{
    if (!m_blockWidgets.contains(blockId)) {
        return;
    }
    
    // Remove widget from layout and delete it
    QWidget *blockWidget = m_blockWidgets.take(blockId);
    m_blockLayout->removeWidget(blockWidget);
    
    // Remove block parts
    m_blockParts.remove(blockId);
    
    // Delete widget
    delete blockWidget;
}

QMenu *TerminalBlockView::createContextMenu()
{
    QMenu *menu = new QMenu(this);
    
    // Create actions
    QAction *copyAction = menu->addAction(tr("Copy"));
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, this, &TerminalBlockView::onCopyAction);
    
    QAction *pasteAction = menu->addAction(tr("Paste"));
    pasteAction->setShortcut(QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, this, &TerminalBlockView::onPasteAction);
    
    menu->addSeparator();
    
    QAction *clearAction = menu->addAction(tr("Clear Terminal"));
    clearAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
    connect(clearAction, &QAction::triggered, this, &TerminalBlockView::onClearAction);
    
    menu->addSeparator();
    
    // Block-specific actions
    if (m_model && m_currentBlockId >= 0) {
        QAction *rerunAction = menu->addAction(tr("Re-run Command"));
        connect(rerunAction, &QAction::triggered, this, [this]() {
            if (m_model && m_currentBlockId >= 0) {
                CommandBlock block = m_model->block(m_currentBlockId);
                executeCommand(block.command);
            }
        });
    }
    
    return menu;
}

void TerminalBlockView::onCurrentBlockChanged(int blockId)
{
    // Clear highlight on previous current block
    if (m_currentBlockId >= 0 && m_blockWidgets.contains(m_currentBlockId)) {
        m_blockWidgets[m_currentBlockId]->setStyleSheet("");
    }
    
    m_currentBlockId = blockId;
    
    // Set highlight on new current block
    if (m_currentBlockId >= 0 && m_blockWidgets.contains(m_currentBlockId)) {
        m_blockWidgets[m_currentBlockId]->setStyleSheet("QWidget { border-left: 3px solid #6a6; }");
        m_scrollArea->ensureWidgetVisible(m_blockWidgets[m_currentBlockId]);
    }
    
    // Emit signal
    emit blockSelected(blockId);
}

void TerminalBlockView::onBlockCreated(int blockId)
{
    if (!m_model) {
        return;
    }
    
    // Get block
    CommandBlock block = m_model->block(blockId);
    
    // Create block widget
    QWidget *blockWidget = createBlockWidget(block);
    
    // Add widget to layout before the stretch
    int layoutIndex = m_blockLayout->count() - 1;
    m_blockLayout->insertWidget(layoutIndex, blockWidget);
    
    // Add to map
    m_blockWidgets.insert(blockId, blockWidget);
    
    // Scroll to new block
    m_scrollArea->ensureWidgetVisible(blockWidget);
}

void TerminalBlockView::onBlockStateChanged(int blockId, int state)
{
    if (!m_model || !m_blockWidgets.contains(blockId)) {
        return;
    }
    
    // Get block parts
    QPair<QLabel*, QTextEdit*> parts = m_blockParts.value(blockId);
    QTextEdit *outputTextEdit = parts.second;
    
    // Apply style based on block state
    switch (state) {
        case CommandBlock::Running:
            outputTextEdit->setStyleSheet(OUTPUT_STYLE_BASE);
            break;
        case CommandBlock::Completed:
            outputTextEdit->setStyleSheet(OUTPUT_STYLE_COMPLETED);
            break;
        case CommandBlock::Failed:
            outputTextEdit->setStyleSheet(OUTPUT_STYLE_FAILED);
            break;
    }
}

void TerminalBlockView::onBlockChanged(int blockId)
{
    updateBlockWidget(blockId);
}

void TerminalBlockView::onCommandInputReturnPressed()
{
    QString command = m_commandInput->text().trimmed();
    if (!command.isEmpty()) {
        executeCommand(command);
    }
}

void TerminalBlockView::onExecuteButtonClicked()
{
    QString command = m_commandInput->text().trimmed();
    if (!command.isEmpty()) {
        executeCommand(command);
    }
}

void TerminalBlockView::onTerminalRedrawRequired()
{
    // Update current block output
    if (m_model && m_terminal && m_model->currentBlockId() >= 0) {
        // The model should have already updated the block with terminal output
        updateBlockWidget(m_model->currentBlockId());
    }
}

void TerminalBlockView::onCursorBlinkTimer()
{
    // Toggle cursor visibility
    m_cursorVisible = !m_cursorVisible;
    
    // Update current block if it's running
    if (m_model && m_model->currentBlockId() >= 0) {
        CommandBlock block = m_model->block(m_model->currentBlockId());
        if (block.state == CommandBlock::Running) {
            // Update the cursor in the output
            QPair<QLabel*, QTextEdit*> parts = m_blockParts.value(block.id);
            QTextEdit *outputTextEdit = parts.second;
            
            QString output = block.output;
            if (m_cursorVisible) {
                output += "█"; // Visible cursor
            }
            
            outputTextEdit->setPlainText(output);
        }
    }
}

void TerminalBlockView::onCopyAction()
{
    // Copy selected text to clipboard
    QString selectedText;
    
    // Check if any text is selected in current block
    if (m_model && m_currentBlockId >= 0 && m_blockParts.contains(m_currentBlockId)) {
        QPair<QLabel*, QTextEdit*> parts = m_blockParts.value(m_currentBlockId);
        QTextEdit *outputTextEdit = parts.second;
        
        selectedText = outputTextEdit->textCursor().selectedText();
    }
    
    if (!selectedText.isEmpty()) {
        QApplication::clipboard()->setText(selectedText);
    } else if (m_model && m_currentBlockId >= 0) {
        // If no text is selected, copy entire output of current block
        CommandBlock block = m_model->block(m_currentBlockId);
        QApplication::clipboard()->setText(block.output);
    }
}

void TerminalBlockView::onPasteAction()
{
    // Check if command input has focus
    if (m_commandInput && m_commandInput->hasFocus()) {
        // Paste clipboard text into command input
        QString clipboardText = QApplication::clipboard()->text();
        if (!clipboardText.isEmpty()) {
            // Remove any newlines to avoid unexpected execution
            clipboardText = clipboardText.split('\n').first();
            
            // Insert text at cursor position
            m_commandInput->insert(clipboardText);
        }
    } else if (m_model && m_currentBlockId >= 0) {
        // If command input doesn't have focus, try to use clipboard text as a new command
        QString clipboardText = QApplication::clipboard()->text();
        
        if (!clipboardText.isEmpty()) {
            // Split by lines and use first line as command
            QString command = clipboardText.split('\n').first().trimmed();
            
            if (!command.isEmpty()) {
                m_commandInput->setText(command);
                m_commandInput->setFocus();
            }
        }
    }
}

void TerminalBlockView::onClearAction()
{
    // Clear the terminal blocks
    if (m_model) {
        // Ask for confirmation
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, tr("Clear Terminal"),
            tr("Are you sure you want to clear all terminal blocks?"),
            QMessageBox::Yes | QMessageBox::No);
            
        if (reply == QMessageBox::Yes) {
            clear();
        }
    }
}

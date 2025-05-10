/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "warpkateview.h"
#include "warpkateplugin.h"
// These will be implemented later:
// #include "terminalemulator.h"
// #include "blockmodel.h"

#include <KLocalizedString>
#include <KActionCollection>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KXMLGUIFactory>

#include <QAction>
#include <QVBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>

WarpKateView::WarpKateView(WarpKatePlugin *plugin, KTextEditor::MainWindow *mainWindow)
    : QObject(mainWindow)
    , KXMLGUIClient()
    , m_plugin(plugin)
    , m_mainWindow(mainWindow)
    , m_dockWidget(nullptr)
    , m_terminalWidget(nullptr)
    , m_terminalEmulator(nullptr)
    , m_blockModel(nullptr)
    , m_terminalVisible(false)
{
    setComponentName(QStringLiteral("warpkate"), i18n("WarpKate"));
    
    // Initialize UI components
    setupUI();
    
    // Initialize actions
    setupActions();
    
    // Initialize terminal (placeholder for now)
    setupTerminal();
    
    // Connect to document changes
    connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, [this](KTextEditor::View *view) {
        if (view) {
            onDocumentChanged(view->document());
        }
    });
    
    // Add to XML GUI
    mainWindow->guiFactory()->addClient(this);
}

WarpKateView::~WarpKateView()
{
    m_mainWindow->guiFactory()->removeClient(this);
    
    if (m_dockWidget) {
        delete m_dockWidget;
    }
}

void WarpKateView::setupUI()
{
    // Create dock widget
    m_dockWidget = new QDockWidget(i18n("WarpKate Terminal"), m_mainWindow->window());
    m_dockWidget->setObjectName(QStringLiteral("warpkate_terminal"));
    m_dockWidget->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    
    // Create placeholder widget
    m_terminalWidget = new QWidget(m_dockWidget);
    QVBoxLayout *layout = new QVBoxLayout(m_terminalWidget);
    
    // Add placeholder label (will be replaced with actual terminal)
    QLabel *placeholderLabel = new QLabel(i18n("WarpKate Terminal (Placeholder)"), m_terminalWidget);
    layout->addWidget(placeholderLabel);
    
    m_dockWidget->setWidget(m_terminalWidget);
    m_mainWindow->addDockWidget(Qt::BottomDockWidgetArea, m_dockWidget);
    
    // Hide initially
    m_dockWidget->hide();
}

void WarpKateView::setupActions()
{
    KActionCollection *actions = actionCollection();
    
    // Show/hide terminal action
    m_showTerminalAction = actions->addAction(QStringLiteral("warpkate_show_terminal"), this, &WarpKateView::toggleTerminal);
    m_showTerminalAction->setText(i18n("Show WarpKate Terminal"));
    m_showTerminalAction->setIcon(QIcon::fromTheme(QStringLiteral("konsole")));
    m_showTerminalAction->setCheckable(true);
    actions->setDefaultShortcut(m_showTerminalAction, Qt::Key_F8);
    
    // Execute current text action
    m_executeAction = actions->addAction(QStringLiteral("warpkate_execute"), this, &WarpKateView::executeCurrentText);
    m_executeAction->setText(i18n("Execute in Terminal"));
    m_executeAction->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
    actions->setDefaultShortcut(m_executeAction, Qt::CTRL | Qt::Key_Return);
    
    // Clear terminal action
    m_clearAction = actions->addAction(QStringLiteral("warpkate_clear"), this, &WarpKateView::clearTerminal);
    m_clearAction->setText(i18n("Clear Terminal"));
    m_clearAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear")));
    
    // Navigation actions
    m_prevBlockAction = actions->addAction(QStringLiteral("warpkate_prev_block"), this, &WarpKateView::previousBlock);
    m_prevBlockAction->setText(i18n("Previous Command Block"));
    m_prevBlockAction->setIcon(QIcon::fromTheme(QStringLiteral("go-up")));
    actions->setDefaultShortcut(m_prevBlockAction, Qt::ALT | Qt::Key_Up);
    
    m_nextBlockAction = actions->addAction(QStringLiteral("warpkate_next_block"), this, &WarpKateView::nextBlock);
    m_nextBlockAction->setText(i18n("Next Command Block"));
    m_nextBlockAction->setIcon(QIcon::fromTheme(QStringLiteral("go-down")));
    actions->setDefaultShortcut(m_nextBlockAction, Qt::ALT | Qt::Key_Down);
}

void WarpKateView::setupTerminal()
{
    // This is a placeholder for now
    // In the future, we'll initialize the actual terminal emulator here
    qDebug() << "WarpKate: Setting up terminal emulation (placeholder)";
    
    // Terminal components will be initialized here:
    // m_terminalEmulator = new TerminalEmulator(m_terminalWidget);
    // m_blockModel = new BlockModel(this);
}

void WarpKateView::showTerminal()
{
    if (!m_terminalVisible) {
        m_dockWidget->show();
        m_terminalVisible = true;
        m_showTerminalAction->setChecked(true);
    }
}

void WarpKateView::hideTerminal()
{
    if (m_terminalVisible) {
        m_dockWidget->hide();
        m_terminalVisible = false;
        m_showTerminalAction->setChecked(false);
    }
}

void WarpKateView::toggleTerminal()
{
    if (m_terminalVisible) {
        hideTerminal();
    } else {
        showTerminal();
    }
}

void WarpKateView::executeCurrentText()
{
    KTextEditor::View *view = m_mainWindow->activeView();
    if (!view) {
        return;
    }
    
    QString command;
    if (view->selection()) {
        command = view->selectionText();
    } else {
        KTextEditor::Document *doc = view->document();
        int line = view->cursorPosition().line();
        command = doc->line(line);
    }
    
    if (command.isEmpty()) {
        return;
    }
    
    // Make terminal visible if it's not
    showTerminal();
    
    qDebug() << "WarpKate: Executing command (placeholder):" << command;
    
    // This will be implemented when the terminal emulator is ready:
    // if (m_terminalEmulator) {
    //     m_terminalEmulator->executeCommand(command);
    // }
}

void WarpKateView::clearTerminal()
{
    qDebug() << "WarpKate: Clearing terminal (placeholder)";
    
    // This will be implemented when the terminal emulator is ready:
    // if (m_terminalEmulator) {
    //     m_terminalEmulator->clear();
    // }
}

void WarpKateView::previousBlock()
{
    qDebug() << "WarpKate: Navigating to previous block (placeholder)";
    
    // This will be implemented when the block model is ready:
    // if (m_blockModel) {
    //     m_blockModel->navigateToPreviousBlock();
    // }
}

void WarpKateView::nextBlock()
{
    qDebug() << "WarpKate: Navigating to next block (placeholder)";
    
    // This will be implemented when the block model is ready:
    // if (m_blockModel) {
    //     m_blockModel->navigateToNextBlock();
    // }
}

void WarpKateView::onDocumentChanged(KTextEditor::Document *document)
{
    if (!document) {
        return;
    }
    
    // Handle document change events
    // For now, this is just a placeholder
    qDebug() << "WarpKate: Document changed:" << document->documentName();
    
    // We could potentially update terminal context based on document
    // or set working directory to document's path
}

#include "moc_warpkateview.cpp"


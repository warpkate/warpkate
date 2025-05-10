/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "warpkateview.h"
#include "warpkateplugin.h"
#include "terminalemulator.h"
#include "blockmodel.h"
#include "terminalblockview.h"

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
    
    // Clean up components
    delete m_blockModel;
    delete m_terminalEmulator;
    
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
    qDebug() << "WarpKate: Setting up terminal components";
    
    // Clear existing layout
    if (m_terminalWidget->layout()) {
        QLayoutItem *item;
        while ((item = m_terminalWidget->layout()->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete m_terminalWidget->layout();
    }
    
    // Create new layout
    QVBoxLayout *layout = new QVBoxLayout(m_terminalWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    // Initialize terminal components
    m_terminalEmulator = new TerminalEmulator(m_terminalWidget);
    m_blockModel = new BlockModel(this);
    m_terminalBlockView = new TerminalBlockView(m_terminalWidget);
    
    // Connect components
    m_terminalBlockView->setTerminalEmulator(m_terminalEmulator);
    m_terminalBlockView->setBlockModel(m_blockModel);
    m_blockModel->setTerminalEmulator(m_terminalEmulator);
    
    // Add terminal view to layout
    layout->addWidget(m_terminalBlockView);
    
    // Connect command execution signal from terminal view
    connect(m_terminalBlockView, &TerminalBlockView::commandExecuted, 
            this, [this](const QString &cmd) {
                qDebug() << "WarpKate: Command executed:" << cmd;
            });
    
    // Connect block selection signal from terminal view
    connect(m_terminalBlockView, &TerminalBlockView::blockSelected,
            this, [this](int blockId) {
                qDebug() << "WarpKate: Block selected:" << blockId;
            });
    
    // Initialize and start terminal
    // Use reasonable default size for the terminal (80 columns x 24 rows)
    m_terminalEmulator->initialize(24, 80);
    
    // Start shell with current document directory if available
    QString initialWorkingDir;
    KTextEditor::View *view = m_mainWindow->activeView();
    if (view && view->document() && view->document()->url().isLocalFile()) {
        QFileInfo fileInfo(view->document()->url().toLocalFile());
        if (fileInfo.exists()) {
            initialWorkingDir = fileInfo.dir().absolutePath();
        }
    }
    
    // Default to home directory if no document is open
    if (initialWorkingDir.isEmpty()) {
        initialWorkingDir = QDir::homePath();
    }
    
    m_terminalEmulator->startShell(QString(), initialWorkingDir);
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

QString WarpKateView::getCurrentText()
{
    KTextEditor::View *view = m_mainWindow->activeView();
    if (!view) {
        return QString();
    }
    
    QString text;
    if (view->selection()) {
        text = view->selectionText();
    } else {
        KTextEditor::Document *doc = view->document();
        int line = view->cursorPosition().line();
        text = doc->line(line);
    }
    
    return text;
}

void WarpKateView::executeCommand(const QString &command)
{
    if (command.isEmpty()) {
        return;
    }
    
    // Make terminal visible if it's not
    showTerminal();
    
    qDebug() << "WarpKate: Executing command:" << command;
    
    if (m_terminalBlockView) {
        m_terminalBlockView->executeCommand(command);
    }
}

void WarpKateView::executeCurrentText()
{
    QString command = getCurrentText();
    executeCommand(command);
}

void WarpKateView::clearTerminal()
{
    qDebug() << "WarpKate: Clearing terminal";
    
    if (m_terminalBlockView) {
        m_terminalBlockView->clear();
    }
}

void WarpKateView::previousBlock()
{
    qDebug() << "WarpKate: Navigating to previous block";
    
    if (m_terminalBlockView) {
        m_terminalBlockView->navigateToPreviousBlock();
    }
}

void WarpKateView::nextBlock()
{
    qDebug() << "WarpKate: Navigating to next block";
    
    if (m_terminalBlockView) {
        m_terminalBlockView->navigateToNextBlock();
    }
}

void WarpKateView::onDocumentChanged(KTextEditor::Document *document)
{
    if (!document) {
        return;
    }
    
    // Handle document change events
    qDebug() << "WarpKate: Document changed:" << document->documentName();
    
    // Set working directory to document's path if available
    if (m_terminalEmulator && document->url().isLocalFile()) {
        QFileInfo fileInfo(document->url().toLocalFile());
        if (fileInfo.exists()) {
            QString dir = fileInfo.dir().absolutePath();
            // Execute cd command to change working directory
            m_terminalEmulator->executeCommand(QString("cd \"%1\"").arg(dir));
            qDebug() << "WarpKate: Setting working directory to:" << dir;
        }
    }
}

#include "moc_warpkateview.cpp"


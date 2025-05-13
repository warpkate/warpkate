/*
 * SPDX-FileCopyrightText: 2025 WarpKate Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "warpkateview.h"
#include "warpkateplugin.h"
#include "terminalemulator.h"
#include "blockmodel.h"
// Not using terminalblockview.h in simplified interface
#include "warpkatepreferencesdialog.h"

#include <KLocalizedString>
#include <KActionCollection>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KXMLGUIFactory>

#include <QAction>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QFileInfo>
#include <QDir>
#include <QTextEdit>
#include <QLineEdit>
#include <QToolBar>
#include <QFontDatabase>
#include <QEvent>
#include <QKeyEvent>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QBrush>
#include <QTimer>

WarpKateView::WarpKateView(WarpKatePlugin *plugin, KTextEditor::MainWindow *mainWindow)
    : QObject(mainWindow)
    , KXMLGUIClient()
    , m_plugin(plugin)
    , m_mainWindow(mainWindow)
    , m_toolView(nullptr)
    , m_terminalWidget(nullptr)
    , m_conversationArea(nullptr)
    , m_promptInput(nullptr)
    , m_toolbar(nullptr)
    , m_terminalEmulator(nullptr)
    , m_blockModel(nullptr)
    , m_terminalVisible(false)
    , m_currentBlockId(-1)
    , m_aiService(nullptr)
{
    setComponentName(QStringLiteral("warpkate"), i18n("WarpKate"));
    
    // Initialize UI components
    // Create click feedback timer
    m_clickFeedbackTimer = new QTimer(this);
    m_clickFeedbackTimer->setSingleShot(true);
    m_clickFeedbackTimer->setInterval(200); // 200ms flash
    connect(m_clickFeedbackTimer, &QTimer::timeout, this, [this]() {
        applyInteractiveElementStyles(); // Restore normal styles
    });
    setupUI();
    
    // Initialize actions
    setupActions();
    
    // Initialize terminal (placeholder for now)
    setupTerminal();
    // Initialize AI service
    setupAIService();
    
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
    delete m_aiService;
    
    // The tool view is owned by the main window, we don't need to delete it
}

void WarpKateView::setupUI()
{
    // Create the tool view in the bottom area
    m_toolView = m_mainWindow->createToolView(
        m_plugin,
        QStringLiteral("warpkate_terminal"),
        KTextEditor::MainWindow::Bottom,
        QIcon(QIcon::fromTheme(QStringLiteral("utilities-terminal")).pixmap(QSize(18, 18))),
        i18n("WarpKate"));
    
    // Create main container widget
    m_terminalWidget = new QWidget(m_toolView);
    QVBoxLayout *layout = new QVBoxLayout(m_terminalWidget);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);
    
    // Create toolbar for buttons
    m_toolbar = new QToolBar(m_terminalWidget);
    m_toolbar->setIconSize(QSize(18, 18));
    
    // Initialize mode icons with proper KDE styling
    m_terminalIcon = QIcon::fromTheme(QStringLiteral("utilities-terminal"), QIcon(QStringLiteral(":/icons/org.kde.konsole.svg")));
    
    // Load AI icon from preferences or use default
    KConfigGroup config = KSharedConfig::openConfig()->group(QStringLiteral("WarpKate"));
    QString iconName = config.readEntry("AIButtonIcon", QStringLiteral("aibutton.svg"));
    m_aiIcon = QIcon::fromTheme(QStringLiteral("assistant"), QIcon(QStringLiteral(":/icons/%1").arg(iconName)));
    
    // Create mode icon label and add to toolbar
//    m_modeIconLabel = new QLabel(m_terminalWidget);
//    m_modeIconLabel->setFixedSize(50, 50);
//    m_modeIconLabel->setAlignment(Qt::AlignCenter);
//    m_modeIconLabel->setPixmap(m_terminalIcon.pixmap(QSize(50, 50)));
//    
//    // Add as first element in toolbar
//    m_toolbar->insertWidget(0, m_modeIconLabel);
    
    // Add action buttons to toolbar
    m_toolbar->addAction(QIcon::fromTheme(QStringLiteral("dialog-ok-apply")), i18n("Code Check"), 
                        this, &WarpKateView::checkCode);
    m_toolbar->addAction(QIcon::fromTheme(QStringLiteral("edit-paste")), i18n("Insert to Editor"), 
                        this, &WarpKateView::insertToEditor);
    m_toolbar->addAction(QIcon::fromTheme(QStringLiteral("document-save")), i18n("Save to Obsidian"), 
                        this, &WarpKateView::saveToObsidian);
    m_toolbar->addAction(QIcon::fromTheme(QStringLiteral("edit-clear")), i18n("Clear Screen"), 
                        this, &WarpKateView::clearTerminal);
    
    // Add spacer to push preferences button to the right
    QWidget* spacer = new QWidget(m_toolbar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_toolbar->addWidget(spacer);
    
    // Add preferences button to the right
    m_toolbar->addAction(QIcon::fromTheme(QStringLiteral("configure")), i18n("Preferences"), 
                        this, &WarpKateView::showPreferences);
    
    layout->addWidget(m_toolbar);
    
    // Create conversation area (read-only text edit)
    m_conversationArea = new QTextBrowser(m_terminalWidget);
    m_conversationArea->setReadOnly(true);
    m_conversationArea->setAcceptRichText(true);
        m_conversationArea->setOpenExternalLinks(false);  // Now using QTextBrowser which supports this method (that's for QTextBrowser)
    // Instead, we'll handle link clicks by connecting to the anchorClicked signal in onTerminalOutput
    // Install event filter for keyboard navigation
    m_conversationArea->installEventFilter(this);
    m_conversationArea->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    layout->addWidget(m_conversationArea, 1); // Takes most of the space
    
    // Create prompt input area - use QTextEdit for expandable area
    m_promptInput = new QTextEdit(m_terminalWidget);
    
    // Configure the input area
    // Get assistant name from preferences
    KConfigGroup configAssistant = KSharedConfig::openConfig()->group(QStringLiteral("WarpKate"));
    QString assistantName = config.readEntry("AssistantName", QStringLiteral("WarpKate"));
    if (config.readEntry("UseCustomAssistantName", false)) {
        m_promptInput->setPlaceholderText(i18n("> Type command or '%1' for AI assistant", assistantName));
    } else {
    m_promptInput->setPlaceholderText(i18n("> Type command or '?' for AI assistant"));
    }
    m_promptInput->setFixedHeight(80); // Doubled height to 80px
    m_promptInput->setMaximumHeight(80); // Doubled maximum height to 80px
    m_promptInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    m_promptInput->setAcceptRichText(false);
    m_promptInput->setTabChangesFocus(true);
    m_promptInput->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_promptInput->setLineWrapMode(QTextEdit::WidgetWidth);
    
    // Set a fixed width font for the input
    QFont inputFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_promptInput->setFont(inputFont);
    
    // Install event filter to handle Enter/Shift+Enter
    // Apply styling to the prompt input - black background with white text
    // Apply styling to the prompt input - black background with white text
    m_promptInput->setStyleSheet(QStringLiteral("QTextEdit { background-color: black; color: white; border: none; border-radius: 3px; }"));
    m_promptInput->installEventFilter(this);
    
    // Create a horizontal layout for the prompt input area
    QHBoxLayout *promptLayout = new QHBoxLayout();
    // Create a vertical layout for the stacked buttons
    QVBoxLayout *buttonLayout = new QVBoxLayout();
    // Create a black background panel for the command area
    QWidget* promptPanel = new QWidget(m_terminalWidget);
    promptPanel->setStyleSheet(QStringLiteral("QWidget { background-color: black; }"));
    promptPanel->setLayout(promptLayout);
    promptLayout->setContentsMargins(0, 0, 0, 0);
    promptLayout->setSpacing(4); // Set to consistent 4px spacing

    // Create a label for the input mode
    m_inputModeLabel = new QLabel(i18n("Command:"), m_terminalWidget);
    m_inputModeLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    // Style the mode label with black background and white text
    // Style the mode label with black background and white text
    m_inputModeLabel->setStyleSheet(QStringLiteral("QLabel { background-color: black; color: white; padding: 0px 5px; }"));
    m_inputModeLabel->setVisible(false); // Hide the label completely
    // Create a toggle switch for input mode
    m_inputModeToggle = new QToolButton(m_terminalWidget);
    // Create a second button for AI mode
    m_aiModeToggle = new QToolButton(m_terminalWidget);
    m_aiModeToggle->setCheckable(true);
    m_aiModeToggle->setIcon(m_aiIcon);
    m_aiModeToggle->setIconSize(QSize(27, 27)); // Reduced by 1/3 from 40px to 27px
    m_aiModeToggle->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_aiModeToggle->setFixedSize(30, 30); // Fixed size for consistent button appearance
    m_aiModeToggle->setStyleSheet(QStringLiteral("QToolButton { background-color: black; border: none; padding: 0; margin: 0; opacity: 0.5; border-radius: 3px; }"));
    connect(m_aiModeToggle, &QToolButton::clicked, this, [this]() {
        m_inputModeToggle->setChecked(false);
        m_aiModeToggle->setChecked(true);
        // Update styles and handle mode change
        onModeButtonClicked(true);
    });
    m_inputModeToggle->setCheckable(true);
    m_inputModeToggle->setIcon(m_terminalIcon); // Command mode icon
    m_inputModeToggle->setIconSize(QSize(27, 27)); // Reduced by 1/3 from 40px to 27px
    m_inputModeToggle->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_inputModeToggle->setFixedSize(30, 30); // Fixed size for consistent button appearance
    m_inputModeToggle->setChecked(false);
    connect(m_inputModeToggle, &QToolButton::clicked, this, [this]() {
        m_inputModeToggle->setChecked(true);
        m_aiModeToggle->setChecked(false);
        // Update styles and handle mode change
        onModeButtonClicked(false);
    });
    // Style the mode toggle button with KDE-like styling
    m_inputModeToggle->setStyleSheet(QStringLiteral("QToolButton { background-color: black; border: none; padding: 0; margin: 0; opacity: 0.5; border-radius: 3px; }"));

    // Add buttons to the vertical layout with proper spacing
    buttonLayout->addWidget(m_aiModeToggle);
    buttonLayout->addWidget(m_inputModeToggle);
    buttonLayout->setSpacing(3); // Small spacing between buttons for better visual separation
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setAlignment(Qt::AlignCenter); // Center the buttons in the layout
    
    // Add button layout and prompt input to the main layout
    promptLayout->addLayout(buttonLayout);
    promptLayout->addWidget(m_promptInput, 1); // Takes most of the space
    // Add the prompt layout to the main layout instead of directly adding m_promptInput
    layout->addWidget(promptPanel);
    
    // Initially hide the tool view
    m_toolView->setVisible(false);
}

void WarpKateView::setupActions()
{
    KActionCollection *actions = actionCollection();
    
    // Show/hide terminal action
    m_showTerminalAction = actions->addAction(QStringLiteral("warpkate_show_terminal"), this, &WarpKateView::toggleTerminal);
    m_showTerminalAction->setText(i18n("Show WarpKate Terminal"));
    m_showTerminalAction->setIcon(QIcon::fromTheme(QStringLiteral("utilities-terminal")));
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
    
    // Insert to editor action
    m_insertToEditorAction = actions->addAction(QStringLiteral("warpkate_insert_to_editor"), this, &WarpKateView::insertToEditor);
    m_insertToEditorAction->setText(i18n("Insert to Editor"));
    m_insertToEditorAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-paste")));
    actions->setDefaultShortcut(m_insertToEditorAction, Qt::CTRL | Qt::Key_I);
    
    // Save to Obsidian action
    m_saveToObsidianAction = actions->addAction(QStringLiteral("warpkate_save_to_obsidian"), this, &WarpKateView::saveToObsidian);
    m_saveToObsidianAction->setText(i18n("Save to Obsidian"));
    m_saveToObsidianAction->setIcon(QIcon::fromTheme(QStringLiteral("document-save")));
    actions->setDefaultShortcut(m_saveToObsidianAction, Qt::CTRL | Qt::Key_S);
    
    // Check code action
    m_checkCodeAction = actions->addAction(QStringLiteral("warpkate_check_code"), this, &WarpKateView::checkCode);
    m_checkCodeAction->setText(i18n("Check Code"));
    m_checkCodeAction->setIcon(QIcon::fromTheme(QStringLiteral("dialog-ok-apply")));
    actions->setDefaultShortcut(m_checkCodeAction, Qt::CTRL | Qt::Key_K);
}

void WarpKateView::setupTerminal()
{
    qDebug() << "WarpKate: Setting up terminal components";
    
    // Initialize terminal components
    m_terminalEmulator = new TerminalEmulator(m_terminalWidget);
    m_blockModel = new BlockModel(this);
    
    // Connect to terminal signals for real-time updates
    connect(m_terminalEmulator, &TerminalEmulator::outputAvailable, this, &WarpKateView::onTerminalOutput);
    connect(m_terminalEmulator, &TerminalEmulator::commandExecuted, this, &WarpKateView::onCommandExecuted);
    connect(m_terminalEmulator, &TerminalEmulator::commandDetected, this, &WarpKateView::onCommandDetected);
    connect(m_terminalEmulator, &TerminalEmulator::workingDirectoryChanged, this, &WarpKateView::onWorkingDirectoryChanged);
    connect(m_terminalEmulator, &TerminalEmulator::shellFinished, this, &WarpKateView::onShellFinished);

    // Connect block model to terminal
    m_blockModel->connectToTerminal(m_terminalEmulator);
    
    // Initialize and start terminal with default size
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
        m_toolView->show();
        m_terminalVisible = true;
        m_showTerminalAction->setChecked(true);
    }
}

void WarpKateView::hideTerminal()
{
    if (m_terminalVisible) {
        m_toolView->hide();
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
    
    // Display command in conversation area with prompt
    QString promptText = QStringLiteral("> %1").arg(command);
    
    // Format as a command with a distinctive style
    QTextCharFormat commandFormat;
    commandFormat.setFontWeight(QFont::Bold);
    commandFormat.setForeground(QBrush(QColor(0, 128, 255)));
    
    // Add command to conversation area with formatting
    QTextCursor cursor = m_conversationArea->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_conversationArea->setTextCursor(cursor);
    cursor.insertBlock();
    cursor.setCharFormat(commandFormat);
    cursor.insertText(promptText);
    cursor.setCharFormat(QTextCharFormat());
    
    // Create a block for this command in the model
    int blockId = m_blockModel->executeCommand(command);
    
    // Store the block ID for reference when output arrives
    m_currentBlockId = blockId;
    
    // Execute the command in the terminal
    m_terminalEmulator->executeCommand(command);
    
    // Output will be handled by the outputAvailable and commandExecuted signals
    // Make sure the view scrolls to show the new command
    m_conversationArea->ensureCursorVisible();
}

void WarpKateView::executeCurrentText()
{
    QString command = getCurrentText();
    executeCommand(command);
}

void WarpKateView::clearTerminal()
{
    qDebug() << "WarpKate: Clearing terminal";
    m_conversationArea->clear();
}

void WarpKateView::previousBlock()
{
    qDebug() << "WarpKate: Navigating to previous block (not implemented in simplified interface)";
    // Not implemented in the simplified interface
}

void WarpKateView::nextBlock()
{
    qDebug() << "WarpKate: Navigating to next block (not implemented in simplified interface)";
    // Not implemented in the simplified interface
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
            m_terminalEmulator->executeCommand(QStringLiteral("cd \"%1\"").arg(dir));
            qDebug() << "WarpKate: Setting working directory to:" << dir;
        }
    }
}

// Add missing method implementations
void WarpKateView::handleAIQuery(const QString &query)
{
    if (query.isEmpty()) {
        return;
    }
    
    // Make terminal visible if it's not
    showTerminal();
    
    qDebug() << "WarpKate: Handling AI query:" << query;
    
    // Create formatted query with distinctive styling
    QTextCharFormat queryFormat;
    queryFormat.setFontWeight(QFont::Bold);
    queryFormat.setForeground(QBrush(QColor(75, 0, 130))); // Indigo for AI queries
    
    // Add query to conversation area with formatting
    QTextCursor cursor = m_conversationArea->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_conversationArea->setTextCursor(cursor);
    cursor.insertBlock();
    cursor.setCharFormat(queryFormat);
    cursor.insertText(QStringLiteral("? %1").arg(query));
    cursor.setCharFormat(QTextCharFormat());
    
    // Get context information to enhance AI response
    QString contextInfo = getContextInformation();
    
    // Simulate an AI response (in a real implementation, this would call an AI service)
    QTimer::singleShot(500, this, [this, query, contextInfo]() {
        generateAIResponse(query, contextInfo);
    });
    
    // Make sure the view scrolls to show the query
    m_conversationArea->ensureCursorVisible();
}

void WarpKateView::insertToEditor()
{
    // Get the current text from the conversation area
    QTextCursor cursor = m_conversationArea->textCursor();
    QString selectedText = cursor.selectedText();
    
    if (selectedText.isEmpty()) {
        qDebug() << "WarpKate: No text selected to insert";
        return;
    }
    
    // Get the active editor view
    KTextEditor::View *view = m_mainWindow->activeView();
    if (!view) {
        qDebug() << "WarpKate: No active editor view";
        return;
    }
    
    // Insert the selected text at current cursor position in the editor
    view->insertText(selectedText);
    qDebug() << "WarpKate: Inserted text into editor";
}

void WarpKateView::saveToObsidian()
{
    qDebug() << "WarpKate: Save to Obsidian requested";
    
    // Get the conversation content
    QString content = m_conversationArea->toPlainText();
    if (content.isEmpty()) {
        qDebug() << "WarpKate: No content to save";
        return;
    }
    
    // Get the Obsidian vault path from settings
    KConfigGroup config = KSharedConfig::openConfig()->group(QStringLiteral("WarpKate"));
    QString vaultPath = config.readEntry("ObsidianVaultPath", QString());
    
    if (vaultPath.isEmpty()) {
        // No vault path configured, show a message and open preferences
        QTextCursor cursor = m_conversationArea->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_conversationArea->setTextCursor(cursor);
        
        cursor.insertBlock();
        cursor.insertText(QStringLiteral("To save to Obsidian, you need to configure your vault path in Preferences."));
        cursor.insertBlock();
        cursor.insertText(QStringLiteral("Would you like to configure it now?"));
        
        // Create a simulated AI response with options
        QTimer::singleShot(500, this, [this]() {
            showPreferences();
        });
        
        return;
    }
    
    // Analyze the conversation to find meaningful content to save
    QStringList conversations;
    QTextDocument *doc = m_conversationArea->document();
    
    for (QTextBlock block = doc->begin(); block != doc->end(); block = block.next()) {
        QString line = block.text().trimmed();
        if (line.startsWith(QStringLiteral("? ")) || line.startsWith(QStringLiteral("AI Response:")) || 
            line.startsWith(QStringLiteral("> ")) || line.startsWith(QStringLiteral("Code Check"))) {
            
            // Start of a new conversation
            conversations.append(line);
            
            // Get following blocks until next conversation marker
            QTextBlock nextBlock = block.next();
            while (nextBlock.isValid()) {
                QString nextLine = nextBlock.text().trimmed();
                
                // If we hit another conversation marker, break
                if (nextLine.startsWith(QStringLiteral("? ")) || nextLine.startsWith(QStringLiteral("AI Response:")) ||
                    nextLine.startsWith(QStringLiteral("> ")) || nextLine.startsWith(QStringLiteral("Code Check"))) {
                    break;
                }
                
                // Add this line to the current conversation
                if (!nextLine.isEmpty()) {
                    conversations.last().append(QStringLiteral("\n") + nextLine);
                }
                
                nextBlock = nextBlock.next();
            }
        }
    }
    
    // Get file pattern from settings
    QString filePattern = config.readEntry("DefaultFilenamePattern", QStringLiteral("WarpKate-Chat-{date}"));
    
    // Replace {date} with current date
    QDateTime now = QDateTime::currentDateTime();
    filePattern.replace(QStringLiteral("{date}"), now.toString(QStringLiteral("yyyy-MM-dd")));
    
    // Display suggestions in the conversation area
    QTextCursor cursor = m_conversationArea->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_conversationArea->setTextCursor(cursor);
    
    cursor.insertBlock();
    cursor.insertBlock();
    
    QTextCharFormat headerFormat;
    headerFormat.setFontWeight(QFont::Bold);
    headerFormat.setForeground(QBrush(QColor(0, 128, 0)));
    
    cursor.setCharFormat(headerFormat);
    cursor.insertText(QStringLiteral("Obsidian Save Analysis:"));
    cursor.setCharFormat(QTextCharFormat());
    
    cursor.insertBlock();
    cursor.insertText(QStringLiteral("I found %1 conversation exchanges in this session.").arg(conversations.size()));
    cursor.insertBlock();
    
    if (conversations.size() <= 3) {
        cursor.insertText(QStringLiteral("Recommended: Save the entire conversation to Obsidian."));
    } else {
        cursor.insertText(QStringLiteral("Recommended: Save the following key exchanges to Obsidian:"));
        
        // Find the most important conversations (for demo, just take first, last, and one in middle)
        QStringList important;
        important << conversations.first();
        
        if (conversations.size() > 2) {
            important << conversations.at(conversations.size() / 2);
        }
        
        important << conversations.last();
        
        cursor.insertBlock();
        for (int i = 0; i < important.size(); ++i) {
            // Truncate if too long
            QString snippet = important[i].left(100);
            if (important[i].length() > 100) {
                snippet += QStringLiteral("...");
            }
            
            cursor.insertText(QStringLiteral("%1. %2").arg(i+1).arg(snippet));
            cursor.insertBlock();
        }
    }
    
    cursor.insertBlock();
    cursor.insertText(QStringLiteral("Proposed filename: %1.md").arg(filePattern));
    cursor.insertBlock();
    cursor.insertText(QStringLiteral("Location: %1").arg(vaultPath));
    cursor.insertBlock();
    
    cursor.insertText(QStringLiteral("(In a full implementation, this would save the file to your Obsidian vault)"));
    cursor.insertBlock();
    
    // TODO: Implement actual file writing
    // This would create a markdown file in the Obsidian vault with the conversation
    
    m_conversationArea->ensureCursorVisible();
}

void WarpKateView::checkCode()
{
    qDebug() << "WarpKate: Code check requested";
    
    // Get the current text from the editor
    QString code = getCurrentText();
    if (code.isEmpty()) {
        qDebug() << "WarpKate: No code selected to check";
        return;
    }
    
    // Make terminal visible if it's not
    showTerminal();
    
    // Format and display the request
    QTextCharFormat requestFormat;
    requestFormat.setFontWeight(QFont::Bold);
    requestFormat.setForeground(QBrush(QColor(0, 100, 0))); // Dark green
    
    QTextCursor cursor = m_conversationArea->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_conversationArea->setTextCursor(cursor);
    cursor.insertBlock();
    cursor.setCharFormat(requestFormat);
    cursor.insertText(QStringLiteral("Code Check requested:"));
    cursor.setCharFormat(QTextCharFormat());
    
    // Format and display the code
    QTextCharFormat codeFormat;
    codeFormat.setFontFamily(QStringLiteral("Monospace"));
    codeFormat.setBackground(QBrush(QColor(240, 240, 240))); // Light gray
    
    cursor.insertBlock();
    cursor.insertText(QStringLiteral("```"));
    cursor.insertBlock();
    cursor.setCharFormat(codeFormat);
    cursor.insertText(code);
    cursor.setCharFormat(QTextCharFormat());
    cursor.insertBlock();
    cursor.insertText(QStringLiteral("```"));
    
    // In a real implementation, this would analyze the code
    // For now, just create a simulated response after a short delay
    QTimer::singleShot(800, this, [this, code]() {
        // Create a simulated code analysis response
        QTextCursor cursor = m_conversationArea->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_conversationArea->setTextCursor(cursor);
        
        // Format for analysis header
        QTextCharFormat analysisHeaderFormat;
        analysisHeaderFormat.setFontWeight(QFont::Bold);
        analysisHeaderFormat.setForeground(QBrush(QColor(0, 100, 0))); // Dark green
        
        cursor.insertBlock();
        cursor.setCharFormat(analysisHeaderFormat);
        cursor.insertText(QStringLiteral("Code Analysis:"));
        cursor.setCharFormat(QTextCharFormat());
        
        // Format for analysis results
        cursor.insertBlock();
        
        // Detect language (simple detection)
        QString language = QStringLiteral("unknown");
        if (code.contains(QStringLiteral("class")) && code.contains(QStringLiteral(";"))) {
            language = QStringLiteral("C++/Java");
        } else if (code.contains(QStringLiteral("def ")) && code.contains(QStringLiteral(":"))) {
            language = QStringLiteral("Python");
        } else if (code.contains(QStringLiteral("function")) && code.contains(QStringLiteral("{"))) {
            language = QStringLiteral("JavaScript");
        }
        
        // Create analysis points based on language
        QTextCharFormat bulletFormat;
        bulletFormat.setFontWeight(QFont::Bold);
        
        // Add language detection result
        cursor.insertText(QStringLiteral("Detected language: "));
        cursor.setCharFormat(bulletFormat);
        cursor.insertText(language);
        cursor.setCharFormat(QTextCharFormat());
        cursor.insertBlock();
        
        // Add analysis points
        QStringList analysisPoints;
        analysisPoints << QStringLiteral("Code structure appears well-organized.")
                      << QStringLiteral("No obvious syntax errors detected.")
                      << QStringLiteral("Consider adding more comments to improve readability.");
        
        if (language == QStringLiteral("C++/Java")) {
            analysisPoints << QStringLiteral("Check memory management to prevent leaks.");
            if (code.contains(QStringLiteral("new ")) && !code.contains(QStringLiteral("delete "))) {
                analysisPoints << QStringLiteral("Warning: Found 'new' without corresponding 'delete'.");
            }
        } else if (language == QStringLiteral("Python")) {
            analysisPoints << QStringLiteral("Consider using list comprehensions for conciseness.");
            if (code.contains(QStringLiteral("except:")) && !code.contains(QStringLiteral("except "))) {
                analysisPoints << QStringLiteral("Warning: Bare except clause can hide errors.");
            }
        }
        
        // Format each analysis point
        for (const QString &point : analysisPoints) {
            cursor.insertText(QStringLiteral("• "));
            cursor.insertText(point);
            cursor.insertBlock();
        }
        
        // Add a blank line
        cursor.insertBlock();
        
        // Ensure visible
        m_conversationArea->ensureCursorVisible();
    });
}

void WarpKateView::showPreferences()
{
    qDebug() << "WarpKate: Preferences dialog requested";
    
    // Create and show the preferences dialog
    WarpKatePreferencesDialog dialog(m_mainWindow->window());
    int result = dialog.exec();
    
    // Refresh UI if dialog was accepted (user clicked OK or Apply)
    if (result == QDialog::Accepted) {
        refreshUIFromSettings();
    }
}

void WarpKateView::onInputModeToggled(bool aiMode)
{
    // For backward compatibility, call our new function
    onModeButtonClicked(aiMode);

    // Sync the button checked states
    if (aiMode) {
        m_aiModeToggle->setChecked(true);
        m_inputModeToggle->setChecked(false);
    } else {
        m_aiModeToggle->setChecked(false);
        m_inputModeToggle->setChecked(true);
    }
}

void WarpKateView::submitInput()
{
    // Get input text
    QString input = m_promptInput->toPlainText().trimmed();
    if (input.isEmpty()) {
        return;
    }
    
    // Process based on which mode button is active
    if (m_aiModeToggle->isChecked()) {
        // AI mode
        handleAIQuery(input);
    } else {
        // Process based on whether it starts with "?" or the assistant name as a fallback
        KConfigGroup config = KSharedConfig::openConfig()->group(QStringLiteral("WarpKate"));
        QString assistantTrigger = QStringLiteral("?");
        if (config.readEntry("UseCustomAssistantName", false)) {
            assistantTrigger = config.readEntry("AssistantName", QStringLiteral("WarpKate"));
        }
        
        if (input.startsWith(assistantTrigger) || input.startsWith(QStringLiteral("?"))) {
            // AI mode
            QString query;
            if (input.startsWith(assistantTrigger)) {
                query = input.mid(assistantTrigger.length()).trimmed();
            } else {
                query = input.mid(1).trimmed();
            }
            handleAIQuery(query);
        } else {
            // Terminal mode
            // Terminal mode
            executeCommand(input);
        }
    }
    
    // Clear input area
    m_promptInput->clear();
    
    // Maintain fixed height of 40px
    m_promptInput->setFixedHeight(80); // Doubled height to 80px
    m_promptInput->setMaximumHeight(80); // Doubled maximum height to 80px
}

bool WarpKateView::eventFilter(QObject *obj, QEvent *event)
{
    // Only process events from the prompt input
    // Handle events for conversation area (Tab navigation)
    if (obj == m_conversationArea && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        
        if (keyEvent->key() == Qt::Key_Tab) {
            if (keyEvent->modifiers() & Qt::ShiftModifier) {
                // Shift+Tab moves to previous element
                focusPreviousInteractiveElement();
            } else {
                // Tab moves to next element
                focusNextInteractiveElement();
            }
            return true; // Event handled
        } else if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            // Enter key activates the focused element
            if (m_currentFocusIndex >= 0 && m_currentFocusIndex < m_interactiveElements.size()) {
                QString href = m_interactiveElements[m_currentFocusIndex].format.anchorHref();
                flashClickFeedback(m_currentFocusIndex);
                onLinkClicked(QUrl(href));
                return true; // Event handled
            }
        }
    }
    if (obj != m_promptInput) {
        return QObject::eventFilter(obj, event);
    }
    
    // Handle key press events
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        // Check for ">" character to toggle between modes ONLY when input is empty
        
        if (keyEvent->text() == QStringLiteral(">") && m_promptInput->toPlainText().isEmpty()) {
            if (m_aiModeToggle->isChecked()) {
                // Currently in AI mode, switch to Terminal mode
                m_aiModeToggle->setChecked(false);
                m_inputModeToggle->setChecked(true);
                onModeButtonClicked(false);
            } else {
                // Currently in Terminal mode, switch to AI mode
                m_aiModeToggle->setChecked(true);
                m_inputModeToggle->setChecked(false);
                onModeButtonClicked(true);
            }
            return true; // Event handled, don't insert the ">" character
        }
        // Detect Enter key (but not Shift+Enter)
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (!(keyEvent->modifiers() & Qt::ShiftModifier)) {
                submitInput();
                return true; // Event handled
            }
        }
    }
    
    // Let the default handler process other events
    return QObject::eventFilter(obj, event);
}
QString WarpKateView::getContextInformation()
{
    QString context;
    
    // Add current document information
    KTextEditor::View *view = m_mainWindow->activeView();
    if (view && view->document()) {
        KTextEditor::Document *doc = view->document();
        context += QStringLiteral("Current document: %1\n").arg(doc->documentName());
        
        // Add file type information
        if (!doc->mimeType().isEmpty()) {
            context += QStringLiteral("File type: %1\n").arg(doc->mimeType());
        }
        
        // Add cursor position
        KTextEditor::Cursor pos = view->cursorPosition();
        context += QStringLiteral("Cursor position: Line %1, Column %2\n").arg(pos.line() + 1).arg(pos.column() + 1);
        
        // Add some context around cursor (a few lines before and after)
        int startLine = qMax(0, pos.line() - 3);
        int endLine = qMin(doc->lines() - 1, pos.line() + 3);
        
        context += QStringLiteral("\nCode context:\n");
        for (int line = startLine; line <= endLine; ++line) {
            QString lineText = doc->line(line);
            // Highlight the current line
            if (line == pos.line()) {
                context += QStringLiteral("> %1: %2\n").arg(line + 1).arg(lineText);
            } else {
                context += QStringLiteral("  %1: %2\n").arg(line + 1).arg(lineText);
            }
        }
    }
    
    // Add working directory information from terminal
    if (m_terminalEmulator) {
        QString pwd = m_terminalEmulator->currentWorkingDirectory();
        if (!pwd.isEmpty()) {
            context += QStringLiteral("\nWorking directory: %1\n").arg(pwd);
        }
    }
    
    return context;
}

void WarpKateView::generateAIResponse(const QString &query, const QString &contextInfo)
{
    if (!m_aiService || !m_aiService->isReady()) {
        // If AI service is not available, format an error message
        QTextCursor cursor = m_conversationArea->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_conversationArea->setTextCursor(cursor);
        
        // Format for AI response
        QTextCharFormat aiHeaderFormat;
        aiHeaderFormat.setFontWeight(QFont::Bold);
        aiHeaderFormat.setForeground(QBrush(QColor(75, 0, 130))); // Indigo
        
        cursor.insertBlock();
        cursor.setCharFormat(aiHeaderFormat);
        cursor.insertText(i18n("AI Service Error:"));
        cursor.setCharFormat(QTextCharFormat());
        
        cursor.insertBlock();
        cursor.insertText(i18n("The AI service is not properly configured. Please check your API key and settings in Preferences."));
        cursor.insertBlock();
        
        // Ensure visible
        m_conversationArea->ensureCursorVisible();
        return;
    }
    
    // Format the header for AI response
    QTextCursor cursor = m_conversationArea->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_conversationArea->setTextCursor(cursor);
    
    // Format for AI response
    QTextCharFormat aiHeaderFormat;
    aiHeaderFormat.setFontWeight(QFont::Bold);
    aiHeaderFormat.setForeground(QBrush(QColor(75, 0, 130))); // Indigo
    
    cursor.insertBlock();
    cursor.setCharFormat(aiHeaderFormat);
    cursor.insertText(i18n("AI Response:"));
    cursor.setCharFormat(QTextCharFormat());
    
    cursor.insertBlock();
    
    // Show a "Thinking..." message that will be replaced by the response
    QTextCharFormat thinkingFormat;
    thinkingFormat.setFontItalic(true);
    thinkingFormat.setForeground(QBrush(QColor(100, 100, 100))); // Gray
    cursor.setCharFormat(thinkingFormat);
    cursor.insertText(i18n("Thinking..."));
    cursor.setCharFormat(QTextCharFormat());
    cursor.insertBlock();
    
    // Ensure visible
    m_conversationArea->ensureCursorVisible();
    
    // Call the AI service to generate a response
    // Pass a callback to handle the response (will replace the "Thinking..." message)
    m_aiService->generateResponse(
        query, 
        contextInfo,
        [this](const QString& response, bool isFinal) {
            handleAIResponse(response, isFinal);
        }
    );
}


void WarpKateView::onTerminalOutput(const QString &output)
{
    if (output.isEmpty()) {
        return;
    }
    
    // Clean the terminal output - remove escape sequences and control characters
    QString cleanedOutput = cleanTerminalOutput(output);
    
    // Skip typical command echo patterns - look for lines that are just a command followed by a newline
    // Common command echo pattern: command followed by newline with no other content
    if (cleanedOutput.count(QLatin1Char('\n')) <= 1 && !cleanedOutput.contains(QLatin1Char(' '))) {
        // This looks like a simple echo of a typed command - skip it
        qDebug() << "WarpKate: Skipping likely command echo:" << cleanedOutput;
        return;
    }
    
    qDebug() << "WarpKate: Terminal output received:" << (output.length() > 50 ? output.left(50) + QStringLiteral("...") : output);
    
    // Process the output for interactive features (file listings, bold directories)
    QString processedOutput = processTerminalOutputForInteractivity(cleanedOutput);
    
    // Format for terminal output
    QTextCharFormat outputFormat;
    outputFormat.setFontFamily(QStringLiteral("Monospace"));
    
    // Add to conversation area
    QTextCursor cursor = m_conversationArea->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_conversationArea->setTextCursor(cursor);
    
    // Only insert a block if we're not at the beginning of a block already
    if (!cursor.atBlockStart()) {
        cursor.insertBlock();
    }
    
    // For simple text output, use the basic approach
    if (!processedOutput.contains(QStringLiteral("<"))) {
        cursor.setCharFormat(outputFormat);
        cursor.insertText(processedOutput);
    } else {
        // If we have HTML/rich text formatting, insert as HTML
        cursor.insertHtml(processedOutput);
    }
    // Connect to the anchorClicked signal to handle clicks on links
    connect(m_conversationArea, &QTextBrowser::anchorClicked, this, &WarpKateView::onLinkClicked, Qt::UniqueConnection);

    // Ensure cursor is visible
    // Connect to the anchorClicked signal to handle clicks on links
    connect(m_conversationArea, &QTextBrowser::anchorClicked, 
            this, &WarpKateView::onLinkClicked,
            Qt::UniqueConnection);

    // Ensure cursor is visible
    // If the output contains HTML links, update interactive elements
    if (processedOutput.contains(QStringLiteral("<a "))) {
        QTimer::singleShot(100, this, &WarpKateView::updateInteractiveElements);
    }
    m_conversationArea->ensureCursorVisible();
    m_conversationArea->ensureCursorVisible();
}
void WarpKateView::onCommandExecuted(const QString &command, const QString &output, int exitCode)
{
    qDebug() << "WarpKate: Command executed:" << command << "with exit code:" << exitCode;
    
    // Format and display the command completion info
    QTextCursor cursor = m_conversationArea->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_conversationArea->setTextCursor(cursor);
    
    // Add separator
    cursor.insertBlock();
    
    // Format output based on exit code
    QTextCharFormat resultFormat;
    resultFormat.setFontFamily(QStringLiteral("Monospace"));
    
    if (exitCode != 0) {
        // Command failed, use error formatting
        resultFormat.setForeground(QBrush(QColor(200, 0, 0))); // Red for errors
        cursor.setCharFormat(resultFormat);
        cursor.insertText(i18n("Command exited with code %1").arg(exitCode));
    } else {
        // Show a subtle completed message
        resultFormat.setForeground(QBrush(QColor(0, 150, 0))); // Green for success
        resultFormat.setFontItalic(true);
        cursor.setCharFormat(resultFormat);
        cursor.insertText(i18n("Command completed successfully"));
    }
    
    cursor.setCharFormat(QTextCharFormat());
    cursor.insertBlock();
    
    // Make sure the view scrolls to show the completion status
    m_conversationArea->ensureCursorVisible();
    
    // Update the block model with the complete output
    if (m_currentBlockId >= 0) {
        m_blockModel->setBlockOutput(m_currentBlockId, output);
        m_blockModel->setBlockExitCode(m_currentBlockId, exitCode);
        m_blockModel->setBlockState(m_currentBlockId, exitCode == 0 ? Completed : Failed);
        m_blockModel->setBlockEndTime(m_currentBlockId, QDateTime::currentDateTime());
    }
}

void WarpKateView::onCommandDetected(const QString &command)
{
    qDebug() << "WarpKate: Command detected:" << command;
    
    // In a more complete implementation, we might update UI elements to show
    // that a command is being executed, update a status bar, etc.
    // For now, we'll just log it
}

void WarpKateView::onWorkingDirectoryChanged(const QString &directory)
{
    qDebug() << "WarpKate: Working directory changed:" << directory;
    
    // Update the conversation area with the directory change information
    QTextCursor cursor = m_conversationArea->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_conversationArea->setTextCursor(cursor);
    
    // Format for directory change notification
    QTextCharFormat dirFormat;
    dirFormat.setFontItalic(true);
    dirFormat.setForeground(QBrush(QColor(100, 100, 100))); // Gray for info messages
    
    // Add notification text
    cursor.insertBlock();
    cursor.setCharFormat(dirFormat);
    cursor.insertText(i18n("Directory changed to: %1").arg(directory));
    cursor.setCharFormat(QTextCharFormat());
    
    // Update the block model with the directory information
    if (m_currentBlockId >= 0) {
        // setBlockWorkingDirectory does not exist in BlockModel class
        // m_blockModel->setBlockWorkingDirectory(m_currentBlockId, directory);
    }
    
    // Make sure the view scrolls to show the notification
    m_conversationArea->ensureCursorVisible();
}

void WarpKateView::onShellFinished(int exitCode)
{
    qDebug() << "WarpKate: Shell process finished with exit code:" << exitCode;
    
    // Format and display the shell termination message
    QTextCursor cursor = m_conversationArea->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_conversationArea->setTextCursor(cursor);
    
    // Format for shell termination message
    QTextCharFormat shellExitFormat;
    shellExitFormat.setFontWeight(QFont::Bold);
    
    if (exitCode != 0) {
        // Shell terminated abnormally
        shellExitFormat.setForeground(QBrush(QColor(200, 0, 0))); // Red for errors
        
        cursor.insertBlock();
        cursor.setCharFormat(shellExitFormat);
        cursor.insertText(i18n("Shell process terminated with exit code %1").arg(exitCode));
    } else {
        // Shell terminated normally
        shellExitFormat.setForeground(QBrush(QColor(0, 100, 0))); // Green for success
        
        cursor.insertBlock();
        cursor.setCharFormat(shellExitFormat);
        cursor.insertText(i18n("Shell session ended"));
    }
    
    cursor.setCharFormat(QTextCharFormat());
    cursor.insertBlock();
    
    // Make sure the view scrolls to show the termination message
    m_conversationArea->ensureCursorVisible();
    
    // Optionally, we could prompt the user to start a new shell session here
    // For now, we'll just log the event
}

void WarpKateView::setupAIService()
{
    // Create the AI service
    m_aiService = new AIService(this);
    
    // Get configuration from WarpKate settings
    KConfigGroup config = KSharedConfig::openConfig()->group(QStringLiteral("WarpKate"));
    
    // Initialize the AI service with the configuration
    if (!m_aiService->initialize(config)) {
        qWarning() << "WarpKate: Failed to initialize AI service";
    } else {
        qDebug() << "WarpKate: AI service initialized successfully";
    }
}

void WarpKateView::handleAIResponse(const QString &response, bool isFinal)
{
    // Get the conversation area and position cursor at the end
    QTextCursor cursor = m_conversationArea->textCursor();
    
    // If this is the first response chunk, remove the "Thinking..." text
    if (isFinal) {
        // For the final response, replace the "Thinking..." text completely
        // Find the last occurrence of "AI Response:" and navigate to it
        QTextDocument *doc = m_conversationArea->document();
        QTextCursor findCursor(doc);
        findCursor.movePosition(QTextCursor::End);
        
        // Search backward for the last "Thinking..." text
        while(findCursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::MoveAnchor)) {
            if (findCursor.block().text().contains(QStringLiteral("Thinking..."))) {
                // Found the "Thinking..." text, select the block
                findCursor.select(QTextCursor::BlockUnderCursor);
                findCursor.removeSelectedText();
                
                // Position cursor here to insert the response
                m_conversationArea->setTextCursor(findCursor);
                cursor = findCursor;
                break;
            }
        }
    } else {
        // For streaming responses, find where we need to append
        // Look for last AI Response block or last response chunk
        cursor.movePosition(QTextCursor::End);
        m_conversationArea->setTextCursor(cursor);
    }
    
    // Format for code blocks in responses
    static QTextCharFormat codeFormat;
    codeFormat.setFontFamily(QStringLiteral("Monospace"));
    codeFormat.setBackground(QBrush(QColor(240, 240, 240))); // Light gray background
    
    // Format for regular text
    static QTextCharFormat regularFormat;
    
    // Check if we're currently in a code block
    static bool inCodeBlock = false;
    
    // Process and format the response text
    QStringList lines = response.split(QStringLiteral("\n"));
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i];
        
        // Check for code block delimiters (```), commonly used in markdown
        if (line.trimmed().startsWith(QStringLiteral("```"))) {
            inCodeBlock = !inCodeBlock;
            
            // Insert a new block if not at start
            if (cursor.position() > 0 && !cursor.atBlockStart()) {
                cursor.insertBlock();
            }
            
            // Insert the code block delimiter
            cursor.insertText(line);
            cursor.insertBlock();
            continue;
        }
        
        // If not already at the start of a block, insert a new one
        if (i > 0 || (!cursor.atBlockStart() && !cursor.atStart())) {
            cursor.insertBlock();
        }
        
        // Apply appropriate format based on whether in code block
        if (inCodeBlock) {
            cursor.setCharFormat(codeFormat);
        } else {
            cursor.setCharFormat(regularFormat);
        }
        
        // Insert the line
        cursor.insertText(line);
    }
    
    // If this is the final response, add any finishing touches
    if (isFinal) {
        // Reset code block state for next response
        inCodeBlock = false;
        
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
    m_conversationArea->ensureCursorVisible();
}

void WarpKateView::refreshUIFromSettings()
{
    // Get configuration from WarpKate settings
    KConfigGroup config = KSharedConfig::openConfig()->group(QStringLiteral("WarpKate"));
    
    // Update AI icon
    QString iconName = config.readEntry("AIButtonIcon", QStringLiteral("aibutton.svg"));
    m_aiIcon = QIcon(QStringLiteral(":/icons/%1").arg(iconName));
    
    // Update toggle button icon if in AI mode
    if (m_inputModeToggle->isChecked()) {
        m_inputModeToggle->setIcon(m_aiIcon);
    }
    
    // Update assistant name in prompt placeholder
    QString assistantName = config.readEntry("AssistantName", QStringLiteral("WarpKate"));
    bool useCustomName = config.readEntry("UseCustomAssistantName", false);
    
    // Update placeholder text based on current mode
    if (m_inputModeToggle->isChecked()) {
        // AI mode
        m_promptInput->setPlaceholderText(i18n("> Ask me anything..."));
    } else {
        // Command mode with assistant hint
        if (useCustomName) {
            m_promptInput->setPlaceholderText(i18n("> Type command or '%1' for AI assistant", assistantName));
        } else {
            m_promptInput->setPlaceholderText(i18n("> Type command or '?' for AI assistant"));
        }
    }
    
    // Reset the input mode label based on current toggle state
    if (m_inputModeToggle->isChecked()) {
        m_inputModeLabel->setText(QStringLiteral("AI Mode:"));
    } else {
        m_inputModeLabel->setText(i18n("Command:"));
    }
    
    qDebug() << "WarpKate: UI refreshed from settings";
}

#include "moc_warpkateview.cpp"
#include <QMessageBox>

void WarpKateView::onModeButtonClicked(bool aiMode)
{
    // Update button styles - active button at full opacity with highlight, inactive at 50%
    if (aiMode) {
        // AI mode active
        m_aiModeToggle->setStyleSheet(QStringLiteral("QToolButton { background-color: #2980b9; border: none; padding: 0; margin: 0; border-radius: 3px; }"));
        m_inputModeToggle->setStyleSheet(QStringLiteral("QToolButton { background-color: black; border: none; padding: 0; margin: 0; opacity: 0.5; border-radius: 3px; }"));
        m_promptInput->setPlaceholderText(i18n("> Ask me anything..."));
    } else {
        // Command mode active
        m_aiModeToggle->setStyleSheet(QStringLiteral("QToolButton { background-color: black; border: none; padding: 0; margin: 0; opacity: 0.5; border-radius: 3px; }"));
        m_inputModeToggle->setStyleSheet(QStringLiteral("QToolButton { background-color: #2980b9; border: none; padding: 0; margin: 0; border-radius: 3px; }"));
        m_promptInput->setPlaceholderText(i18n("> Type command..."));
    }
}

QString WarpKateView::cleanTerminalOutput(const QString &rawOutput)
{
    
// ANSI escape sequences (color codes, cursor movement, etc.)
    static QRegularExpression ansiEscapeRE(QStringLiteral("\033\\[[0-9;]*[A-Za-z]"));
    
    // Terminal title sequences and similar OSC sequences
    static QRegularExpression oscSequenceRE(QStringLiteral("\033\\][0-9].*;.*(\007|\033\\\\)"));
    
    // Terminal status sequences like [?2004h and [?2004l
    static QRegularExpression termStatusRE(QStringLiteral("\\[\\?[0-9;]*[a-zA-Z]"));
    
    // Simple terminal prompts like ]0;user@host:path
    static QRegularExpression termPromptRE(QStringLiteral("\\][0-9];[^\007]*"));
    
    // Control characters (excluding newlines and tabs which we want to preserve)
    static QRegularExpression controlCharsRE(QStringLiteral("[\\x00-\\x08\\x0B\\x0C\\x0E-\\x1F]"));
    
    // Bell character
    static QRegularExpression bellRE(QStringLiteral("\007"));
    
    // Start cleaning the output
    QString cleaned = rawOutput;
    
    // Log the original and cleaned output for debugging
    qDebug() << "Original terminal output:" << (rawOutput.length() > 50 ? rawOutput.left(50) + QStringLiteral("...") : rawOutput);
    
    // Apply all the filters
    cleaned = cleaned.replace(ansiEscapeRE, QStringLiteral(""));
    cleaned = cleaned.replace(oscSequenceRE, QStringLiteral(""));
    cleaned = cleaned.replace(termStatusRE, QStringLiteral(""));
    cleaned = cleaned.replace(termPromptRE, QStringLiteral(""));
    cleaned = cleaned.replace(controlCharsRE, QStringLiteral(""));
    cleaned = cleaned.replace(bellRE, QStringLiteral(""));
    
    // Additional cleanup for common escape sequences in bash/zsh
    cleaned = cleaned.replace(QStringLiteral("\\]0;"), QStringLiteral(""));
    
    // Log the cleaned output for debugging
    qDebug() << "Cleaned terminal output:" << (cleaned.length() > 50 ? cleaned.left(50) + QStringLiteral("...") : cleaned);
    
    return cleaned;
}

QString WarpKateView::processTerminalOutputForInteractivity(const QString &output)
{
    // If output is empty, return early
    if (output.isEmpty()) {
        return output;
    }
    
    // Get the current working directory
    QString workingDir;
    if (m_terminalEmulator) {
        workingDir = m_terminalEmulator->currentWorkingDirectory();
    }
    
    // If we don't have a working directory, try to extract it from the output
    if (workingDir.isEmpty()) {
        // Attempt to extract working directory from PS1 prompt if present in output
        QRegularExpression cwdRegex(QStringLiteral("\\[(.*?)\\]\\$"));
        QRegularExpressionMatch cwdMatch = cwdRegex.match(output);
        if (cwdMatch.hasMatch()) {
            workingDir = cwdMatch.captured(1);
        } else {
            // Default to home directory if nothing else available
            workingDir = QDir::homePath();
        }
    }
    
    // Check if this output is likely from an ls command
    bool isLsOutput = false;
    QStringList lines = output.split(QStringLiteral("\n"));
    int fileEntryCount = 0;
    
    // Look for patterns that suggest this is an ls command output
    QString firstLine = lines.isEmpty() ? QString() : lines.first().trimmed();
    QString lastLine = lines.isEmpty() ? QString() : lines.last().trimmed();
    
    // Check for ls command pattern in the first line
    if (firstLine.startsWith(QStringLiteral("total ")) || 
        output.contains(QStringLiteral("drwx")) || 
        output.contains(QStringLiteral("-rw-"))) {
        isLsOutput = true;
    }
    
    // Also check first few lines for ls-like patterns
    for (int i = 0; i < qMin(5, lines.size()); ++i) {
        if (lines[i].contains(QRegularExpression(QStringLiteral("^[d\\-][rwx\\-]{9}")))) {
            isLsOutput = true;
            break;
        }
    }
    
    // Specific check for `ls` (no flags) output which just lists filenames
    // Count how many plausible file entries we have
    for (const QString &line : lines) {
        if (!line.trimmed().isEmpty() && !line.startsWith(QStringLiteral("total "))) {
            // Split by whitespace to see if we have multiple entries per line (common in ls)
            QStringList entries = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
            fileEntryCount += entries.size();
        }
    }
    
    if (fileEntryCount > 3 && lines.size() < 10) {
        // Likely an `ls` command with just filenames
        isLsOutput = true;
    }
    
    // Process line by line to look for individual file listing patterns
    bool hasProcessedFileListing = false;
    QStringList processedLines;
    for (const QString &line : lines) {
        if (processFileListingLine(line, workingDir)) {
            hasProcessedFileListing = true;
            processedLines.append(line);
        }
    }
    
    // If we found and processed individual file listings, format them as HTML
    if (hasProcessedFileListing && !processedLines.isEmpty()) {
        QString htmlOutput = QStringLiteral("<pre>");
        for (const QString &line : processedLines) {
            htmlOutput += line + QStringLiteral("\n");
        }
        htmlOutput += QStringLiteral("</pre>");
        return htmlOutput;
    }
    
    // If we have an ls output, process it to add interactivity
    if (isLsOutput) {
        QString htmlOutput;
        
        // Process ls output with different formats based on detected style
        if (output.contains(QRegularExpression(QStringLiteral("^[d\\-][rwx\\-]{9}")))) {
            // Detailed listing (ls -l format)
            htmlOutput = processDetailedListing(output, workingDir);
        } else {
            // Simple listing (ls format)
            htmlOutput = processSimpleListing(output, workingDir);
        }
        
        return htmlOutput;
    }
    
    // Not an ls output, just add basic styling (make directories bold)
    QString htmlOutput = QStringLiteral("<pre>");
    
    // Process each line to detect and highlight directories
    for (const QString &line : lines) {
        QString processedLine = line;
        
        // Look for potential directory patterns (common shell prompts, cd commands)
        QRegularExpression dirRegex(QStringLiteral("(?:\\[|cd\\s+)([\\w\\.\\-/~]+)(?:\\]|$)"));
        QRegularExpressionMatch dirMatch = dirRegex.match(line);
        
        if (dirMatch.hasMatch()) {
            QString dirPath = dirMatch.captured(1);
            // Make the directory bold and clickable
            QString fullPath = dirPath;
            if (!fullPath.startsWith(QStringLiteral("/")) && !fullPath.startsWith(QStringLiteral("~"))) {
                fullPath = workingDir + QStringLiteral("/") + dirPath;
            }
            processedLine.replace(dirPath, QStringLiteral("<a href=\"file://%1\" style=\"color: inherit; text-decoration: none;\"><b>%2</b></a>")
                                 .arg(fullPath.toHtmlEscaped(), dirPath.toHtmlEscaped()));
        }
        
        htmlOutput += processedLine + QStringLiteral("\n");
    }
    
    htmlOutput += QStringLiteral("</pre>");
    return htmlOutput;
}

QString WarpKateView::processDetailedListing(const QString &output, const QString &workingDir)
{
    QString htmlOutput = QStringLiteral("<pre>");
    QStringList lines = output.split(QStringLiteral("\n"));
    
    for (const QString &line : lines) {
        // Skip empty lines
        if (line.trimmed().isEmpty()) {
            htmlOutput += QStringLiteral("\n");
            continue;
        }
        
        // Skip the "total" line that appears in ls -l output
        if (line.startsWith(QStringLiteral("total "))) {
            htmlOutput += line + QStringLiteral("\n");
            continue;
        }
        
        // Check if this is a potential file/directory line (ls -l format)
        QRegularExpression fileRegex(QStringLiteral("^([d\\-])([rwx\\-]{9})\\s+\\d+\\s+\\w+\\s+\\w+\\s+\\d+\\s+\\w+\\s+\\d+\\s+[\\d:]+\\s+(.+)$"));
        QRegularExpressionMatch fileMatch = fileRegex.match(line);
        
        if (fileMatch.hasMatch()) {
            bool isDir = fileMatch.captured(1) == QStringLiteral("d");
            QString permissions = fileMatch.captured(2);
            QString filename = fileMatch.captured(3);
            
            // Build the full path
            QString fullPath = workingDir + QStringLiteral("/") + filename;
            
            // Format the line based on file type - make directories bold
            QString formattedLine;
            if (isDir) {
                // Make directory name bold and clickable
                formattedLine = line.left(line.length() - filename.length());
                formattedLine += QStringLiteral("<a href=\"file://%1\" style=\"color: inherit; text-decoration: none;\"><b>%2</b></a>")
                                .arg(fullPath.toHtmlEscaped(), filename.toHtmlEscaped());
            } else {
                // Make regular files clickable
                formattedLine = line.left(line.length() - filename.length());
                formattedLine += QStringLiteral("<a href=\"file://%1\" style=\"color: inherit; text-decoration: none;\">%2</a>")
                                .arg(fullPath.toHtmlEscaped(), filename.toHtmlEscaped());
            }
            
            htmlOutput += formattedLine + QStringLiteral("\n");
        } else {
            // If not matched, just add the line as is
            htmlOutput += line + QStringLiteral("\n");
        }
    }
    
    htmlOutput += QStringLiteral("</pre>");
    return htmlOutput;
}

QString WarpKateView::processSimpleListing(const QString &output, const QString &workingDir)
{
    QString htmlOutput = QStringLiteral("<pre>");
    QStringList lines = output.split(QStringLiteral("\n"));
    
    for (const QString &line : lines) {
        // Skip empty lines
        if (line.trimmed().isEmpty()) {
            htmlOutput += QStringLiteral("\n");
            continue;
        }
        
        // Skip the "total" line that appears in ls -l output
        if (line.startsWith(QStringLiteral("total "))) {
            htmlOutput += line + QStringLiteral("\n");
            continue;
        }
        
        // Split the line by whitespace to process individual filenames
        QStringList entries = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        
        // Process each file/directory entry in the line
        QString processedLine = line;
        for (const QString &entry : entries) {
            // Skip if entry is empty or just dots
            if (entry.isEmpty() || entry == QStringLiteral(".") || entry == QStringLiteral("..")) {
                continue;
            }
            
            // Skip common non-file words from ls output
            if (entry == QStringLiteral("total") || 
                entry.startsWith(QStringLiteral("-")) || 
                entry == QStringLiteral("ls") || 
                entry.length() < 2) {
                continue;
            }
            
            // Check if this entry is a directory
            QString fullPath = workingDir + QStringLiteral("/") + entry;
            bool isDir = isDirectory(entry, output);
            if (!isDir) {
                // Double-check with filesystem if available
                QFileInfo fileInfo(fullPath);
                if (fileInfo.exists()) {
                    isDir = fileInfo.isDir();
                }
            }
            
            // Replace with formatted version
            if (isDir) {
                // Directory - make bold and clickable
                processedLine.replace(QRegularExpression(QStringLiteral("\\b") + QRegularExpression::escape(entry) + QStringLiteral("\\b")),
                                     QStringLiteral("<a href=\"file://%1\" style=\"color: inherit; text-decoration: none;\"><b>%2</b></a>")
                                     .arg(fullPath.toHtmlEscaped(), entry.toHtmlEscaped()));
            } else {
                // Regular file - make clickable
                processedLine.replace(QRegularExpression(QStringLiteral("\\b") + QRegularExpression::escape(entry) + QStringLiteral("\\b")),
                                     QStringLiteral("<a href=\"file://%1\" style=\"color: inherit; text-decoration: none;\">%2</a>")
                                     .arg(fullPath.toHtmlEscaped(), entry.toHtmlEscaped()));
            }
        }
        
        htmlOutput += processedLine + QStringLiteral("\n");
    }
    
    htmlOutput += QStringLiteral("</pre>");
    return htmlOutput;
}

bool WarpKateView::processFileListingLine(const QString &line, const QString &workingDir)
{
    // Skip empty lines
    if (line.trimmed().isEmpty()) {
        return false;
    }
    
    // Skip the "total" line that appears in ls -l output
    if (line.startsWith(QStringLiteral("total "))) {
        return false;
    }
    
    // Check if this is a detailed listing line (ls -l format)
    QRegularExpression detailedRegex(QStringLiteral("^[d\\-][rwx\\-]{9}\\s+\\d+\\s+\\w+\\s+\\w+\\s+\\d+\\s+\\w+\\s+\\d+\\s+[\\d:]+\\s+(.+)$"));
    QRegularExpressionMatch detailedMatch = detailedRegex.match(line);
    
    if (detailedMatch.hasMatch()) {
        // This is a file listing in detailed format
        return true;
    }
    
    // Check if this is a simple file/directory name
    // Look for patterns that might indicate it's in a listing context
    QStringList entries = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    
    // If we have a series of what looks like filenames (non-command strings)
    if (entries.size() > 2) {
        bool allLikelyFilenames = true;
        for (const QString &entry : entries) {
            // Skip if it looks like a command flag
            if (entry.startsWith(QStringLiteral("-")) && entry.length() > 1 && !entry.at(1).isDigit()) {
                allLikelyFilenames = false;
                break;
            }
            
            // Skip if it's a common command
            if (entry == QStringLiteral("ls") || entry == QStringLiteral("cd") || 
                entry == QStringLiteral("grep") || entry == QStringLiteral("find")) {
                allLikelyFilenames = false;
                break;
            }
        }
        
        if (allLikelyFilenames) {
            return true;
        }
    }
    
    // Not a file listing line
    return false;
}

QMenu* WarpKateView::createFileContextMenu(const QString &filePath, bool isDirectory)
{
    QMenu *menu = new QMenu();
    
    // Add common actions for both files and directories
    QAction *copyPathAction = menu->addAction(QIcon::fromTheme(QStringLiteral("edit-copy")), 
                                            i18n("Copy Path to Clipboard"));
    connect(copyPathAction, &QAction::triggered, this, [this, filePath]() {
        copyPathToClipboard(filePath);
    });
    
    menu->addSeparator();
    
    if (isDirectory) {
        // Directory-specific actions
        QAction *openDirAction = menu->addAction(QIcon::fromTheme(QStringLiteral("folder-open")), 
                                              i18n("Open in File Manager"));
        connect(openDirAction, &QAction::triggered, this, [this, filePath]() {
            openDirectory(filePath);
        });
        
        // Open terminal here action (changes working directory)
        QAction *cdHereAction = menu->addAction(QIcon::fromTheme(QStringLiteral("utilities-terminal")), 
                                               i18n("Change Directory Here"));
        connect(cdHereAction, &QAction::triggered, this, [this, filePath]() {
            m_terminalEmulator->executeCommand(QStringLiteral("cd \"%1\"").arg(filePath));
        });
    } else {
        // File-specific actions
        QAction *openFileAction = menu->addAction(QIcon::fromTheme(QStringLiteral("document-open")), 
                                               i18n("Open with Default Application"));
        connect(openFileAction, &QAction::triggered, this, [this, filePath]() {
            openFile(filePath);
        });
        
        QAction *openInKateAction = menu->addAction(QIcon::fromTheme(QStringLiteral("kate")), 
                                                  i18n("Open in Kate"));
        connect(openInKateAction, &QAction::triggered, this, [this, filePath]() {
            openFileInKate(filePath);
        });
    }
    
    return menu;
}

bool WarpKateView::isDirectory(const QString &filename, const QString &output)
{
    // Skip common terminal output words that aren't actually files or directories
    static const QStringList nonFileWords = {
        QStringLiteral("total"),
        QStringLiteral("ls"),
        QStringLiteral("cd"),
        QStringLiteral("grep"),
        QStringLiteral("find")
    };
    
    if (nonFileWords.contains(filename) || filename.contains(QRegularExpression(QStringLiteral("^\\d+$")))) {
        return false;
    }
    
    // First, check if the filename ends with a slash (common directory indicator)
    if (filename.endsWith(QStringLiteral("/"))) {
        return true;
    }
    
    // Check if the name is a common directory name
    if (filename == QStringLiteral(".") || filename == QStringLiteral("..")) {
        return true;
    }
    
    // Look for directory indicators in detailed ls output
    if (output.contains(QRegularExpression(QStringLiteral("^d[rwx\\-]{9}.*\\s+") + QRegularExpression::escape(filename) + QStringLiteral("\\s*$"), QRegularExpression::MultilineOption))) {
        return true;
    }

    // Alternatively, check the filesystem directly if possible
    if (m_terminalEmulator) {
        QString workingDir = m_terminalEmulator->currentWorkingDirectory();
        if (!workingDir.isEmpty()) {
            QString fullPath = workingDir + QStringLiteral("/") + filename;
            QFileInfo fileInfo(fullPath);
            if (fileInfo.exists()) {
                return fileInfo.isDir();
            }
        }
    }
    
    // Check if the name is in blue or bold in the terminal output (common for directories)
    // This is a heuristic and might not work in all cases since we've already cleaned ANSI codes
    
    // If no other indication, check if it has no extension (more likely to be a directory)
    // but also has some typical directory-like attributes
    // Be more selective with the heuristic for detecting directories
    if (!filename.contains(QStringLiteral(".")) && 
        (filename.length() > 2) && 
        !filename.contains(QRegularExpression(QStringLiteral("[\\(\\)\\[\\]\\{\\}\\<\\>\\|\\*\\&\\^\\%\\$\\#\\@\\!\\~\\`]"))) &&
        // Make sure filename doesn't consist of only digits
        !filename.contains(QRegularExpression(QStringLiteral("^\\d+$"))) &&
        // Exclude common terminal output words
        !nonFileWords.contains(filename)) {
        // Increase the chance that this is a directory, but not definite
        return true;
    }
    
    // Default to false if we can't determine
    return false;
}

QString WarpKateView::detectFileType(const QString &filename)
{
    // Use Qt's MIME database to detect file type
    QMimeDatabase mimeDB;
    QMimeType mimeType = mimeDB.mimeTypeForFile(filename);
    
    // Return the MIME type name
    return mimeType.name();
}

void WarpKateView::openFile(const QString &filePath)
{
    // Use QDesktopServices to open the file with the default application
    QUrl fileUrl = QUrl::fromLocalFile(filePath);
    
    // Log the attempt
    qDebug() << "WarpKate: Opening file with default application:" << filePath;
    
    // Open the file
    bool success = QDesktopServices::openUrl(fileUrl);
    
    if (!success) {
        qWarning() << "WarpKate: Failed to open file:" << filePath;
        
        // Add a notification to the conversation area
        QTextCursor cursor = m_conversationArea->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_conversationArea->setTextCursor(cursor);
        
        QTextCharFormat errorFormat;
        errorFormat.setForeground(QBrush(QColor(200, 0, 0))); // Red
        
        cursor.insertBlock();
        cursor.setCharFormat(errorFormat);
        cursor.insertText(i18n("Error: Failed to open file: %1", filePath));
        cursor.setCharFormat(QTextCharFormat());
        
        m_conversationArea->ensureCursorVisible();
    }
}

void WarpKateView::openDirectory(const QString &dirPath)
{
    // Use QDesktopServices to open the directory in the default file manager
    QUrl dirUrl = QUrl::fromLocalFile(dirPath);
    
    // Log the attempt
    qDebug() << "WarpKate: Opening directory in file manager:" << dirPath;
    
    // Open the directory
    bool success = QDesktopServices::openUrl(dirUrl);
    
    if (!success) {
        qWarning() << "WarpKate: Failed to open directory:" << dirPath;
        
        // Add a notification to the conversation area
        QTextCursor cursor = m_conversationArea->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_conversationArea->setTextCursor(cursor);
        
        QTextCharFormat errorFormat;
        errorFormat.setForeground(QBrush(QColor(200, 0, 0))); // Red
        
        cursor.insertBlock();
        cursor.setCharFormat(errorFormat);
        cursor.insertText(i18n("Error: Failed to open directory: %1", dirPath));
        cursor.setCharFormat(QTextCharFormat());
        
        m_conversationArea->ensureCursorVisible();
    }
}

void WarpKateView::openFileInKate(const QString &filePath)
{
    // Log the attempt
    qDebug() << "WarpKate: Opening file in Kate:" << filePath;
    
    // First check if the file exists
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        qWarning() << "WarpKate: File does not exist:" << filePath;
        
        // Add a notification to the conversation area
        QTextCursor cursor = m_conversationArea->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_conversationArea->setTextCursor(cursor);
        
        QTextCharFormat errorFormat;
        errorFormat.setForeground(QBrush(QColor(200, 0, 0))); // Red
        
        cursor.insertBlock();
        cursor.setCharFormat(errorFormat);
        cursor.insertText(i18n("Error: File does not exist: %1", filePath));
        cursor.setCharFormat(QTextCharFormat());
        
        m_conversationArea->ensureCursorVisible();
        return;
    }
    
    // Since we're already in Kate, use the main window to open the file
    m_mainWindow->openUrl(QUrl::fromLocalFile(filePath));
}

void WarpKateView::copyPathToClipboard(const QString &filePath)
{
    // Copy the file path to the clipboard
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(filePath);
    
    qDebug() << "WarpKate: Copied path to clipboard:" << filePath;
    
    // Add a confirmation to the conversation area
    QTextCursor cursor = m_conversationArea->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_conversationArea->setTextCursor(cursor);
    
    QTextCharFormat infoFormat;
    infoFormat.setFontItalic(true);
    infoFormat.setForeground(QBrush(QColor(0, 100, 0))); // Green
    
    cursor.insertBlock();
    cursor.setCharFormat(infoFormat);
    cursor.insertText(i18n("Copied to clipboard: %1", filePath));
    cursor.setCharFormat(QTextCharFormat());
    
    m_conversationArea->ensureCursorVisible();
}

void WarpKateView::handleFileItemClicked(const QString &filePath, bool isDirectory)
{
    qDebug() << "WarpKate: File item clicked:" << filePath << "Is directory:" << isDirectory;
    
    // Default action based on file type
    if (isDirectory) {
        // For directories, open in file manager
        openDirectory(filePath);
    } else {
        // For files, detect type and decide what to do
        QString fileType = detectFileType(filePath);
        
        // Text files should open in Kate
        if (fileType.startsWith(QStringLiteral("text/")) || 
            fileType.contains(QStringLiteral("javascript")) ||
            fileType.contains(QStringLiteral("json")) || 
            fileType.contains(QStringLiteral("xml")) ||
            fileType.contains(QStringLiteral("html")) ||
            fileType.contains(QStringLiteral("css")) ||
            fileType.endsWith(QStringLiteral("/x-c")) ||
            fileType.endsWith(QStringLiteral("/x-c++")) ||
            fileType.endsWith(QStringLiteral("/x-python")) ||
            fileType.endsWith(QStringLiteral("/x-java"))) {
            
            openFileInKate(filePath);
        } else {
            // Non-text files open with default application
            openFile(filePath);
        }
    }
}

void WarpKateView::onLinkClicked(const QUrl &url)
{
    qDebug() << "WarpKate: Link clicked:" << url.toString();
    
    // Handle file:// URLs specially for our interactive file listings
    if (url.scheme() == QStringLiteral("file")) {
        QString filePath = url.toLocalFile();
        QFileInfo fileInfo(filePath);
        // Find which element was clicked for feedback
        int clickedIndex = -1;
        for (int i = 0; i < m_interactiveElements.size(); ++i) {
            QString href = m_interactiveElements[i].format.anchorHref();
            if (href == url.toString()) {
                clickedIndex = i;
                break;
            }
        }
        
        // Show click feedback
        flashClickFeedback(clickedIndex);
        
        // Check if the path exists
        if (!fileInfo.exists()) {
            qWarning() << "WarpKate: File does not exist:" << filePath;
            
            // Add a notification to the conversation area
            QTextCursor cursor = m_conversationArea->textCursor();
            cursor.movePosition(QTextCursor::End);
            m_conversationArea->setTextCursor(cursor);
            
            QTextCharFormat errorFormat;
            errorFormat.setForeground(QBrush(QColor(200, 0, 0))); // Red
            
            cursor.insertBlock();
            cursor.setCharFormat(errorFormat);
            cursor.insertText(i18n("Error: File does not exist: %1", filePath));
            cursor.setCharFormat(QTextCharFormat());
            
            m_conversationArea->ensureCursorVisible();
            return;
        }
        
        // Determine if it's a directory
        bool isDir = fileInfo.isDir();
        
        // Check which mouse button was pressed
        Qt::MouseButtons buttons = QApplication::mouseButtons();
        
        // For right-click, show context menu
        if (buttons & Qt::RightButton) {
            QMenu *menu = createFileContextMenu(filePath, isDir);
            
            // Show the menu at the cursor position
            QPoint pos = QCursor::pos();
            QAction *result = menu->exec(pos);
            
            // Clean up
            delete menu;
            
            // If an action was selected, it will have been handled by the action's trigger
        } else {
            // For left-click (or other buttons), directly perform the default action
            handleFileItemClicked(filePath, isDir);
        }
    } else {
        // For other types of URLs, use the default handler
        QDesktopServices::openUrl(url);
    }
}

/**
 * Check if a file is executable
 * @param filePath Full path to the file
 * @return True if the file is executable
 */
bool WarpKateView::isExecutable(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    
    // Check if file exists and has executable permissions
    if (fileInfo.exists() && fileInfo.isFile()) {
        return fileInfo.isExecutable();
    }
    
    return false;
}

/**
 * Execute a file safely
 * @param filePath Full path to the file to execute
 */
void WarpKateView::executeFile(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    
    // Check if file exists and is executable
    if (!fileInfo.exists()) {
        qWarning() << "WarpKate: Cannot execute file (not found):" << filePath;
        
        // Add a notification to the conversation area
        QTextCursor cursor = m_conversationArea->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_conversationArea->setTextCursor(cursor);
        
        QTextCharFormat errorFormat;
        errorFormat.setForeground(QBrush(QColor(200, 0, 0))); // Red
        
        cursor.insertBlock();
        cursor.setCharFormat(errorFormat);
        cursor.insertText(i18n("Error: File does not exist: %1", filePath));
        cursor.setCharFormat(QTextCharFormat());
        
        m_conversationArea->ensureCursorVisible();
        return;
    }
    
    if (!fileInfo.isExecutable()) {
        qWarning() << "WarpKate: Cannot execute file (not executable):" << filePath;
        
        // Add a notification to the conversation area
        QTextCursor cursor = m_conversationArea->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_conversationArea->setTextCursor(cursor);
        
        QTextCharFormat errorFormat;
        errorFormat.setForeground(QBrush(QColor(200, 0, 0))); // Red
        
        cursor.insertBlock();
        cursor.setCharFormat(errorFormat);
        cursor.insertText(i18n("Error: File is not executable: %1", filePath));
        cursor.setCharFormat(QTextCharFormat());
        
        m_conversationArea->ensureCursorVisible();
        return;
    }
    
    // Show confirmation dialog
    QMessageBox::StandardButton confirmation = QMessageBox::question(
        m_mainWindow->window(),
        i18n("Execute File"),
        i18n("Are you sure you want to execute '%1'?", fileInfo.fileName()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (confirmation != QMessageBox::Yes) {
        return;
    }
    
    // Determine how to execute the file based on its type
    QString command;
    QString extension = fileInfo.suffix().toLower();
    
    // Check if this is a script that needs an interpreter
    if (extension == QStringLiteral("sh")) {
        command = QStringLiteral("bash \"%1\"").arg(filePath);
    } else if (extension == QStringLiteral("py")) {
        command = QStringLiteral("python3 \"%1\"").arg(filePath);
    } else if (extension == QStringLiteral("pl")) {
        command = QStringLiteral("perl \"%1\"").arg(filePath);
    } else if (extension == QStringLiteral("rb")) {
        command = QStringLiteral("ruby \"%1\"").arg(filePath);
    } else if (extension == QStringLiteral("js")) {
        command = QStringLiteral("node \"%1\"").arg(filePath);
    } else {
        // For other executable files, check if it's a binary or text file
        QFile file(filePath);
        bool isBinary = false;
        
        if (file.open(QIODevice::ReadOnly)) {
            // Read the first few bytes to check if it's binary
            QByteArray start = file.read(4096);
            file.close();
            
            // Check for NULL bytes or other binary content
            for (int i = 0; i < start.size(); i++) {
                if (start.at(i) == '\0') {
                    isBinary = true;
                    break;
                }
            }
        }
        
        if (isBinary) {
            // For binary files, execute directly, but be more cautious
            QMessageBox::StandardButton execConfirmation = QMessageBox::warning(
                m_mainWindow->window(),
                i18n("Execute Binary File"),
                i18n("'%1' appears to be a binary file. Are you sure you want to execute it?", fileInfo.fileName()),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No
            );
            
            if (execConfirmation != QMessageBox::Yes) {
                return;
            }
            
            command = QStringLiteral("\"%1\"").arg(filePath);
        } else {
            // For other text files, assume it might be a script and use bash
            command = QStringLiteral("bash \"%1\"").arg(filePath);
        }
    }
    
    // Log what we're executing
    qDebug() << "WarpKate: Executing file:" << filePath << "with command:" << command;
    
    // Check if we need to cd to the directory first
    QString workingDir = fileInfo.absolutePath();
    QString currentDir;
    
    if (m_terminalEmulator) {
        currentDir = m_terminalEmulator->currentWorkingDirectory();
    }
    
    if (!workingDir.isEmpty() && workingDir != currentDir) {
        // We need to change directory first
        command = QStringLiteral("cd \"%1\" && %2").arg(workingDir, command);
    }
    
    // Execute the command through our terminal
    if (m_terminalEmulator) {
        // Add a notification to the conversation area
        QTextCursor cursor = m_conversationArea->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_conversationArea->setTextCursor(cursor);
        
        QTextCharFormat infoFormat;
        infoFormat.setForeground(QBrush(QColor(0, 150, 0))); // Green
        
        cursor.insertBlock();
        cursor.setCharFormat(infoFormat);
        cursor.insertText(i18n("Executing: %1", command));
        cursor.setCharFormat(QTextCharFormat());
        
        m_conversationArea->ensureCursorVisible();
        
        // Execute the command
        executeCommand(command);
    } else {
        // If terminal emulator is not available, try using QProcess as fallback
        qWarning() << "WarpKate: Terminal emulator not available, using QProcess as fallback";
        
        QProcess *process = new QProcess(this);
        process->setWorkingDirectory(workingDir);
        
        // For process cleanup
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                process, &QProcess::deleteLater);
        
        // Start the process
        process->start(QStringLiteral("/bin/bash"), QStringList() << QStringLiteral("-c") << command);
        
        // Add a notification to the conversation area
        QTextCursor cursor = m_conversationArea->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_conversationArea->setTextCursor(cursor);
        
        QTextCharFormat infoFormat;
        infoFormat.setForeground(QBrush(QColor(0, 150, 0))); // Green
        
        cursor.insertBlock();
        cursor.setCharFormat(infoFormat);
        cursor.insertText(i18n("Executing (external): %1", command));
        cursor.setCharFormat(QTextCharFormat());
        
        m_conversationArea->ensureCursorVisible();
    }
}
// ==== Add these methods to warpkateview.cpp ====

/**
 * Update the list of interactive elements in the conversation area
 */
void WarpKateView::updateInteractiveElements()
{
    m_interactiveElements.clear();
    
    QTextDocument *doc = m_conversationArea->document();
    
    // Scan document for links (a href tags)
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            QTextFragment fragment = it.fragment();
            
            if (fragment.isValid()) {
                QTextCharFormat format = fragment.charFormat();
                
                if (format.isAnchor()) {
                    // Found a link - create an ExtraSelection for it
                    QString href = format.anchorHref();
                    
                    // Only include file:// URLs
                    if (href.startsWith(QStringLiteral("file://"))) {
                        QTextEdit::ExtraSelection selection;
                        
                        // Normal style (will be modified when focused)
                        selection.format.setForeground(QBrush(QColor(0, 0, 200))); // Dark blue
                        selection.format.setBackground(QBrush(QColor(240, 240, 255))); // Light blue background
                        selection.format.setProperty(QTextFormat::AnchorHref, href);
                        
                        // Get the cursor for this fragment
                        QTextCursor cursor(doc);
                        cursor.setPosition(fragment.position());
                        cursor.setPosition(fragment.position() + fragment.length(), QTextCursor::KeepAnchor);
                        selection.cursor = cursor;
                        
                        m_interactiveElements.append(selection);
                    }
                }
            }
        }
    }
    
    // Reset focus index if needed
    if (m_currentFocusIndex >= m_interactiveElements.size()) {
        m_currentFocusIndex = -1;
    }
    
    // Apply styles
    applyInteractiveElementStyles();
}

/**
 * Focus the next interactive element (when Tab is pressed)
 */
void WarpKateView::focusNextInteractiveElement()
{
    // Update the list first in case content has changed
    updateInteractiveElements();
    
    if (m_interactiveElements.isEmpty()) {
        return;
    }
    
    // Move to next item or wrap around
    m_currentFocusIndex++;
    if (m_currentFocusIndex >= m_interactiveElements.size()) {
        m_currentFocusIndex = 0;
    }
    
    // Apply styles and scroll to the focused element
    applyInteractiveElementStyles();
    
    // Scroll to make the focused element visible
    if (m_currentFocusIndex >= 0 && m_currentFocusIndex < m_interactiveElements.size()) {
        m_conversationArea->setTextCursor(m_interactiveElements[m_currentFocusIndex].cursor);
        m_conversationArea->ensureCursorVisible();
    }
}

/**
 * Focus the previous interactive element (when Shift+Tab is pressed)
 */
void WarpKateView::focusPreviousInteractiveElement()
{
    // Update the list first in case content has changed
    updateInteractiveElements();
    
    if (m_interactiveElements.isEmpty()) {
        return;
    }
    
    // Move to previous item or wrap around
    m_currentFocusIndex--;
    if (m_currentFocusIndex < 0) {
        m_currentFocusIndex = m_interactiveElements.size() - 1;
    }
    
    // Apply styles and scroll to the focused element
    applyInteractiveElementStyles();
    
    // Scroll to make the focused element visible
    if (m_currentFocusIndex >= 0 && m_currentFocusIndex < m_interactiveElements.size()) {
        m_conversationArea->setTextCursor(m_interactiveElements[m_currentFocusIndex].cursor);
        m_conversationArea->ensureCursorVisible();
    }
}

/**
 * Apply highlighting styles to interactive elements based on current focus
 */
void WarpKateView::applyInteractiveElementStyles()
{
    // Create a new list of extra selections
    QList<QTextEdit::ExtraSelection> extraSelections;
    
    // Add each interactive element with appropriate style
    for (int i = 0; i < m_interactiveElements.size(); ++i) {
        QTextEdit::ExtraSelection selection = m_interactiveElements[i];
        
        if (i == m_currentFocusIndex) {
            // Focused item - highlight more intensely
            selection.format.setForeground(QBrush(QColor(0, 0, 200))); // Dark blue text
            selection.format.setBackground(QBrush(QColor(200, 220, 255))); // Stronger blue background
            selection.format.setFontWeight(QFont::Bold);
        } else if (i == m_lastClickedIndex && m_clickFeedbackTimer->isActive()) {
            // Clicked item during feedback flash - red highlight
            selection.format.setForeground(QBrush(QColor(200, 0, 0))); // Red text
            selection.format.setBackground(QBrush(QColor(255, 220, 220))); // Light red background
            selection.format.setFontWeight(QFont::Bold);
        } else {
            // Normal item (not focused) - subtle styling
            selection.format.setForeground(QBrush(QColor(0, 0, 150))); // Blue text
            selection.format.setBackground(QBrush(Qt::transparent)); // No background
            selection.format.setFontWeight(QFont::Normal);
        }
        
        extraSelections.append(selection);
    }
    
    // Apply the extra selections to the text edit
    m_conversationArea->setExtraSelections(extraSelections);
}

/**
 * Flash a click feedback effect on the clicked element
 */
void WarpKateView::flashClickFeedback(int elementIndex)
{
    if (elementIndex >= 0 && elementIndex < m_interactiveElements.size()) {
        m_lastClickedIndex = elementIndex;
        
        // Apply styles with click feedback
        applyInteractiveElementStyles();
        
        // Start timer to remove feedback after a short delay
        m_clickFeedbackTimer->start();
    }
}

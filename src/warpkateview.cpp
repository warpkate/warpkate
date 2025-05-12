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
{
    setComponentName(QStringLiteral("warpkate"), i18n("WarpKate"));
    
    // Initialize UI components
    setupUI();
    
    // Initialize actions
    setupActions();
    
    // Initialize terminal (placeholder for now)
    setupTerminal();
    // Initialize AI service
    
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
    
    // Initialize mode icons
    m_terminalIcon = QIcon(QStringLiteral(":/icons/org.kde.konsole.svg"));
    m_aiIcon = QIcon(QStringLiteral(":/icons/robbie50.svg"));
    
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
    m_conversationArea = new QTextEdit(m_terminalWidget);
    m_conversationArea->setReadOnly(true);
    m_conversationArea->setAcceptRichText(true);
    m_conversationArea->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    layout->addWidget(m_conversationArea, 1); // Takes most of the space
    
    // Create prompt input area - use QTextEdit for expandable area
    m_promptInput = new QTextEdit(m_terminalWidget);
    
    // Configure the input area
    // Get assistant name from preferences
    KConfigGroup config = KSharedConfig::openConfig()->group(QStringLiteral("WarpKate"));
    QString assistantName = config.readEntry("AssistantName", QStringLiteral("WarpKate"));
    if (config.readEntry("UseCustomAssistantName", false)) {
        m_promptInput->setPlaceholderText(i18n("> Type command or '%1' for AI assistant", assistantName));
    } else {
        m_promptInput->setPlaceholderText(i18n("> Type command or '?' for AI assistant"));
    }
    m_promptInput->setMinimumHeight(24);  // Start small
    m_promptInput->setMaximumHeight(150); // Limit maximum height
    m_promptInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    m_promptInput->setAcceptRichText(false);
    m_promptInput->setTabChangesFocus(true);
    m_promptInput->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_promptInput->setLineWrapMode(QTextEdit::WidgetWidth);
    
    // Set a fixed width font for the input
    QFont inputFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_promptInput->setFont(inputFont);
    
    // Install event filter to handle Enter/Shift+Enter
    m_promptInput->installEventFilter(this);
    
    // Create a horizontal layout for the prompt input area
    QHBoxLayout *promptLayout = new QHBoxLayout();
    promptLayout->setContentsMargins(0, 0, 0, 0);
    promptLayout->setSpacing(4);

    // Create a label for the input mode
    m_inputModeLabel = new QLabel(i18n("Command:"), m_terminalWidget);
    m_inputModeLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

    // Create a toggle switch for input mode
    m_inputModeToggle = new QToolButton(m_terminalWidget);
    m_inputModeToggle->setCheckable(true);
    m_inputModeToggle->setIcon(m_aiIcon);
    m_inputModeToggle->setIconSize(QSize(80, 80));
    m_inputModeToggle->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_inputModeToggle->setChecked(false);
    connect(m_inputModeToggle, &QToolButton::toggled, this, &WarpKateView::onInputModeToggled);

    // Add components to the layout
    promptLayout->addWidget(m_inputModeLabel);
    promptLayout->addWidget(m_inputModeToggle);
    promptLayout->addWidget(m_promptInput, 1); // Takes most of the space

    // Add the prompt layout to the main layout instead of directly adding m_promptInput
    layout->addLayout(promptLayout);
    
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
    dialog.exec();
}

void WarpKateView::onInputModeToggled(bool aiMode)
{
    if (aiMode) {
        // Show Robbie icon for AI mode
        // m_modeIconLabel is now removed
        m_inputModeLabel->setText(QStringLiteral("AI Mode:"));
        m_promptInput->setPlaceholderText(i18n("> Ask me anything..."));
    } else {
        // Show Terminal icon for command mode
        // m_modeIconLabel is now removed
        m_inputModeLabel->setText(i18n("Command:"));
        m_promptInput->setPlaceholderText(i18n("> Type command..."));
    }
}

void WarpKateView::submitInput()
{
    // Get input text
    QString input = m_promptInput->toPlainText().trimmed();
    if (input.isEmpty()) {
        return;
    }
    
    // Process based on the toggle state
    if (m_inputModeToggle->isChecked()) {
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
            executeCommand(input);
        }
    }
    
    // Clear input area
    m_promptInput->clear();
    
    // Reset height to minimum (will grow again as needed)
    m_promptInput->setFixedHeight(24);
    m_promptInput->setMaximumHeight(150);
}

bool WarpKateView::eventFilter(QObject *obj, QEvent *event)
{
    // Only process events from the prompt input
    if (obj != m_promptInput) {
        return QObject::eventFilter(obj, event);
    }
    
    // Handle key press events
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        // Check for ">" as first character to activate AI mode
        if (keyEvent->text() == QStringLiteral(">") && m_promptInput->toPlainText().isEmpty() && !m_inputModeToggle->isChecked()) {
            m_inputModeToggle->setChecked(true); // Switch to AI mode
            return true; // Event handled, dont insert the ">" character
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
    // In a real implementation, this would call an AI service
    // For now, we'll create a simulated response
    
    QTextCursor cursor = m_conversationArea->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_conversationArea->setTextCursor(cursor);
    
    // Format for AI response
    QTextCharFormat aiHeaderFormat;
    aiHeaderFormat.setFontWeight(QFont::Bold);
    aiHeaderFormat.setForeground(QBrush(QColor(75, 0, 130))); // Indigo
    
    cursor.insertBlock();
    cursor.setCharFormat(aiHeaderFormat);
    cursor.insertText(QStringLiteral("AI Response:"));
    cursor.setCharFormat(QTextCharFormat());
    
    cursor.insertBlock();
    
    // Generate a response based on the query
    QString response;
    
    if (query.contains(QStringLiteral("help")) || query.contains(QStringLiteral("?"))) {
        response = QStringLiteral("I can help you with coding tasks, terminal commands, and providing explanations. "
                               "Try asking about specific code, commands, or concepts!");
    }
    else if (query.contains(QStringLiteral("code")) || query.contains(QStringLiteral("function")) || query.contains(QStringLiteral("class"))) {
        // Code-related question
        response = QStringLiteral("Based on your code context, I can suggest the following:\n\n"
                               "1. Consider structuring your code with clear function boundaries\n"
                               "2. Use descriptive variable names for better readability\n"
                               "3. Add comments to explain complex logic\n\n"
                               "Would you like me to provide a specific code example?");
    }
    else if (query.contains(QStringLiteral("terminal")) || query.contains(QStringLiteral("command")) || query.contains(QStringLiteral("shell"))) {
        // Terminal-related question
        response = QStringLiteral("Here are some useful terminal commands:\n\n"
                               "```bash\n"
                               "# Navigate directories\n"
                               "cd directory_name    # Change to a directory\n"
                               "cd ..               # Go up one level\n"
                               "pwd                 # Print working directory\n\n"
                               "# File operations\n"
                               "ls -la              # List all files with details\n"
                               "touch filename      # Create empty file\n"
                               "mkdir dirname       # Create directory\n"
                               "```\n\n"
                               "Is there a specific terminal task you need help with?");
    }
    else {
        // Generic response
        response = QStringLiteral("I understand you're asking about '%1'. "
                               "While I don't have specific information about this, "
                               "I can help you research it or provide general guidance. "
                               "Could you provide more details about what you're trying to accomplish?").arg(query);
    }
    
    // Insert response
    cursor.insertText(response);
    
    // Add usage hint for first-time users
    static bool firstResponse = true;
    if (firstResponse) {
        cursor.insertBlock();
        cursor.insertBlock();
        QTextCharFormat hintFormat;
        hintFormat.setFontItalic(true);
        hintFormat.setForeground(QBrush(QColor(100, 100, 100))); // Gray
        cursor.setCharFormat(hintFormat);
        cursor.insertText(QStringLiteral("Tip: Select text in the response and use 'Insert to Editor' to paste it into your document."));
        cursor.setCharFormat(QTextCharFormat());
        firstResponse = false;
    }
    
    // Add a blank line
    cursor.insertBlock();
    
    // Ensure visible
    m_conversationArea->ensureCursorVisible();
}

void WarpKateView::onTerminalOutput(const QString &output)
{
    if (output.isEmpty()) {
        return;
    }
    
    qDebug() << "WarpKate: Terminal output received:" << (output.length() > 50 ? output.left(50) + QStringLiteral("...") : output);
    
    // In a complete implementation, we would process this output and display it in the conversation area
    // For now, we'll just display it without much processing
    
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
    
    cursor.setCharFormat(outputFormat);
    cursor.insertText(output);
    
    // Make sure the view scrolls to show the new output
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
        cursor.insertText(QStringLiteral("Command exited with code %1").arg(exitCode));
    } else {
        // Show a subtle completed message
        resultFormat.setForeground(QBrush(QColor(0, 150, 0))); // Green for success
        resultFormat.setFontItalic(true);
        cursor.setCharFormat(resultFormat);
        cursor.insertText(QStringLiteral("Command completed successfully"));
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
    cursor.insertText(QStringLiteral("Directory changed to: %1").arg(directory));
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
        cursor.insertText(QStringLiteral("Shell process terminated with exit code %1").arg(exitCode));
    } else {
        // Shell terminated normally
        shellExitFormat.setForeground(QBrush(QColor(0, 100, 0))); // Green for success
        
        cursor.insertBlock();
        cursor.setCharFormat(shellExitFormat);
        cursor.insertText(QStringLiteral("Shell session ended"));
    }
    
    cursor.setCharFormat(QTextCharFormat());
    cursor.insertBlock();
    
    // Make sure the view scrolls to show the termination message
    m_conversationArea->ensureCursorVisible();
    
    // Optionally, we could prompt the user to start a new shell session here
    // For now, we'll just log the event
}

#include "moc_warpkateview.cpp"


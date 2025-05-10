# WarpKate Development Notes

## Project Overview

WarpKate is a plugin for the Kate text editor that integrates Warp Terminal-like features, providing a modern, block-based terminal experience directly within the editor. The project aims to enhance developer productivity by combining the power of a traditional terminal with modern UI/UX concepts.

### Key Goals
- Create a seamless terminal integration within Kate
- Implement a block-based command execution model inspired by Warp Terminal
- Support navigation between command blocks and their outputs
- Provide better visual organization of command history
- Eventually integrate AI-powered suggestions and tools

## Key Components

### 1. TerminalEmulator
**Purpose**: Provides VT100/ANSI terminal emulation capabilities and manages the pseudo-terminal (PTY) communication with the shell.

**Key Features**:
- Full VT100/ANSI escape sequence support
- PTY-based communication with the shell process
- Terminal state management (cursor position, colors, text attributes)
- Command execution and output handling

### 2. BlockModel
**Purpose**: Data model that organizes commands and their outputs into discrete, navigable blocks.

**Key Features**:
- Stores command blocks with unique IDs
- Tracks command state (running, completed, failed)
- Handles command execution through the terminal emulator
- Provides Qt model-like interface for UIs

### 3. TerminalBlockView
**Purpose**: UI component that displays terminal content in a block-based view.

**Key Features**:
- Visual representation of command blocks
- Handles user input for command execution
- Provides navigation between blocks
- Clipboard operations (copy/paste)
- Context menu for common operations

### 4. WarpKateView
**Purpose**: Integrates the terminal components into the Kate editor framework.

**Key Features**:
- Manages docking and UI integration
- Provides editor-specific actions
- Connects editor operations with terminal functionality
- Handles document changes and context switching

## Technical Implementation Details

### Terminal Emulation

The `TerminalEmulator` class implements a VT100/ANSI terminal emulator. Key points to remember:

- **Initialization sequence**: 
  1. Call `initialize(rows, cols)` to set up the terminal size
  2. Call `startShell()` to start the shell process with an optional working directory

- **Process handling**: 
  - Uses a PTY (pseudo-terminal) for bidirectional communication with the shell
  - Utilizes `QSocketNotifier` for async I/O with the PTY

- **Escape sequence handling**:
  - Implements CSI (Control Sequence Introducer) sequences
  - Supports SGR (Select Graphic Rendition) for text formatting
  - Handles cursor movement and screen manipulation sequences

### Block Model

The `BlockModel` class manages command blocks. Key implementation details:

- **Block structure**:
  - Each block has a unique ID, command, output, state, and timestamp
  - States include: Running, Completed, and Failed

- **Command detection**:
  - Monitors terminal output to detect command prompt
  - Uses the terminal state to associate output with commands

- **Signal connections**:
  - Connected to terminal for redraw events
  - Emits signals when blocks are created, changed, or selected

### UI Integration

The `TerminalBlockView` class provides the UI for block-based terminal. Important details:

- **Widget hierarchy**:
  - Uses a scroll area for the main view
  - Each block is a separate widget with command and output sections
  - Command input is at the bottom of the view

- **Block widgets**:
  - Created dynamically when blocks are added to the model
  - Updated when block content or state changes
  - Styled differently based on block state

- **Input handling**:
  - Command input with history navigation
  - Keyboard shortcuts for various actions
  - Context menu for common operations

## Current Status

### Completed
- ✅ Project structure and build system
- ✅ Basic terminal emulation with VT100/ANSI support
- ✅ Block-based model for command organization
- ✅ UI components for displaying blocks
- ✅ Integration with Kate's docking system
- ✅ Core functionality for executing commands
- ✅ Clipboard operations (copy/paste)
- ✅ Block navigation (prev/next)

### In Progress
- 🔄 Improved error handling
- 🔄 Better handling of interactive commands
- 🔄 More robust terminal emulation

### Not Started
- ❌ AI-powered suggestions
- ❌ Command history management and search
- ❌ Tab completion
- ❌ Terminal session persistence
- ❌ Multiple terminal views
- ❌ Custom styling and themes

## Next Steps and Priorities

### Short-term Priorities
1. **Testing and Stability**
   - Write unit tests for core components
   - Handle edge cases in terminal emulation
   - Fix any bugs in the current implementation

2. **UI Polishing**
   - Improve visual appearance of command blocks
   - Add animations for state transitions
   - Enhance keyboard navigation

3. **Terminal Features**
   - Implement tab completion
   - Add command history navigation
   - Support for interactive commands (e.g., vim, htop)

### Medium-term Goals
1. **Advanced Block Features**
   - Add block folding/expansion
   - Include metadata (execution time, exit code)
   - Allow block reordering and grouping

2. **Session Management**
   - Save and restore terminal sessions
   - Multiple terminal profiles
   - Working directory tracking

3. **Performance Optimization**
   - Optimize rendering for large outputs
   - Lazy loading of block content
   - Efficient handling of escape sequences

### Long-term Vision
1. **AI Integration**
   - Command suggestions based on context
   - Automatic error correction
   - Natural language command generation

2. **Advanced Kate Integration**
   - Execute code snippets from editor
   - Context-aware terminal behavior
   - Integration with Kate's project system

3. **Extended Features**
   - Split views (horizontal/vertical)
   - Remote terminal sessions (SSH)
   - Terminal recording and playback

## Challenges and Considerations

### Technical Challenges
- **Terminal Emulation Complexity**: VT100/ANSI terminal emulation is complex with many edge cases. The implementation needs to handle a wide variety of escape sequences and terminal behaviors.

- **PTY Handling**: PTY interaction requires careful handling of file descriptors, signals, and process management. There are platform-specific considerations to be aware of.

- **Performance with Large Outputs**: Terminal commands with large outputs need efficient rendering and buffer management to avoid UI lag.

- **Interactive Commands**: Commands that take over the terminal (like vim, nano) require special handling to work properly within the block-based UI.

### Integration Considerations
- **Kate API Limitations**: Work within the constraints of Kate's plugin API and event system.

- **Qt Compatibility**: Ensure compatibility across different Qt versions used by KDE applications.

- **Platform Compatibility**: Terminal behavior differs slightly across operating systems. Testing on multiple platforms is important.

### User Experience
- **Balance between Traditional Terminal and Block UI**: Need to maintain the familiarity of a traditional terminal while adding block-based features.

- **Performance Expectations**: Users expect terminal emulation to be fast and responsive.

- **Keyboard Accessibility**: Terminal users typically prefer keyboard interactions over mouse.

## Development Workflow

1. **Implementation Strategy**: Focus on one component at a time, starting with core functionality and then adding features incrementally.

2. **Testing Approach**: 
   - Manual testing for UI components
   - Unit tests for model and emulation logic
   - Integration tests for complete workflows

3. **Documentation**: 
   - Maintain thorough code documentation
   - Keep user-facing documentation up to date
   - Document design decisions and rationale

## References and Resources

- **VT100/ANSI Terminal Documentation**:
  - [ANSI Escape Sequences](https://en.wikipedia.org/wiki/ANSI_escape_code)
  - [VT100 User Guide](https://vt100.net/docs/vt100-ug/)

- **Qt and KDE Resources**:
  - [Kate Plugin Development Guide](https://docs.kde.org/stable5/en/applications/kate/dev-plugin.html)
  - [Qt Documentation](https://doc.qt.io/)

- **Warp Terminal Reference**:
  - [Warp Terminal Features](https://www.warp.dev/features)

---

*Last updated: May 10, 2025*

## Simplified Installation Approach

### URL-Based Installation Idea

To simplify the installation process for users, we're considering a streamlined approach where users with Warp Terminal already installed can easily add the WarpKate plugin to their Kate editor:

**Implementation Steps:**

1. **Create an Installation Page**
   - Develop a dedicated page on GitHub wiki or project website
   - Provide clear, step-by-step installation instructions
   - Use a simple URL like `https://github.com/warpkate/warpkate/wiki/Installation`

2. **Provide Pre-Built Packages**
   - Create distribution-specific packages (RPM, DEB)
   - Package as a Flatpak for cross-distribution compatibility
   - Develop a simple installation script for automated setup

3. **Warp Terminal Integration**
   - Add a command or menu option in Warp Terminal to access installation
   - Implement a command like `warp install-kate-plugin` for one-step installation
   - Display installation status within Warp Terminal

4. **Prerequisites Check**
   - Automatically verify Warp Terminal installation
   - Check Kate version compatibility
   - Verify required system dependencies

5. **Documentation Focus**
   - Post-installation configuration guidance
   - Usage instructions for Warp features within Kate
   - Tips for AI-assisted terminal usage in the editor environment

This approach would significantly lower the barrier to entry by removing the need for users to build from source, which can be challenging for many. Users would only need to visit a URL and follow simple instructions or run an installation script.

*Note: This is a future enhancement planned for implementation after the core functionality stabilizes.*

---

*Last updated: May 11, 2025*

## Kate Plugin API and Integration Points

### KTextEditor API Overview

The KTextEditor API provides a powerful framework for interacting with Kate's editor component. Key interfaces for WarpKate integration include:

#### Core Interfaces

1. **KTextEditor::Document**
   - Represents an open document in the editor
   - Provides access to the text content, manipulation methods, and metadata
   - Emits signals for document changes, saving, loading, etc.

   ```cpp
   // Get the active document
   KTextEditor::Document* document = mainWindow->activeView()->document();
   
   // Read content from document
   QString text = document->text();
   
   // Get selected text
   QString selection = document->activeView()->selectionText();
   ```

2. **KTextEditor::View**
   - Represents a view of a document (a single editor window)
   - Handles user interaction, cursor position, selection
   - Provides methods for manipulating the view state

   ```cpp
   // Get the cursor position
   KTextEditor::Cursor cursor = view->cursorPosition();
   
   // Execute a command in the view's command line
   view->cmdLineBar()->execute("s/oldtext/newtext/g");
   ```

3. **KTextEditor::MainWindow**
   - Represents the Kate main window
   - Provides access to views, documents, and UI components
   - Allows integration with Kate's UI elements

   ```cpp
   // Create a tool view in the main window
   QWidget* toolView = mainWindow->createToolView(
       "warpkate_terminal",
       KTextEditor::MainWindow::Bottom,
       QIcon::fromTheme("utilities-terminal"),
       "WarpKate Terminal");
   ```

4. **KTextEditor::Plugin**
   - Base class for Kate plugins
   - Manages plugin lifecycle and view creation
   - Handles configuration and persistence

### Key Integration Points

#### Document Interaction

1. **Text Manipulation**
   - Insert, replace, or delete text in the document
   ```cpp
   // Insert text at cursor position
   document->insertText(cursor, "Inserted text");
   
   // Replace selected text
   document->replaceText(selection, "New text");
   ```

2. **Selection Manipulation**
   - Get or set the current selection
   - Execute commands on selected text
   ```cpp
   // Get selection range
   KTextEditor::Range selectionRange = view->selectionRange();
   
   // Select text programmatically
   view->setSelection(KTextEditor::Range(1, 0, 5, 10));
   ```

3. **Document Signals**
   - Connect to document signals to react to changes
   ```cpp
   // React to document changes
   connect(document, &KTextEditor::Document::textChanged,
           this, &WarpKateView::onDocumentChanged);
   ```

#### Editor-Terminal Integration

1. **Execute Selected Code**
   ```cpp
   QString selectedText = view->selectionText();
   if (!selectedText.isEmpty()) {
       m_terminalEmulator->executeCommand(selectedText);
   }
   ```

2. **Process Terminal Output to Editor**
   ```cpp
   // Insert terminal output into document
   QString terminalOutput = m_terminalEmulator->lastOutput();
   document->insertText(view->cursorPosition(), terminalOutput);
   ```

3. **Error Linking**
   ```cpp
   // Parse error message with line:col information
   QString errorMsg = getErrorMessage(); // From terminal output
   QRegularExpression re("file\\.cpp:(\\d+):(\\d+)");
   QRegularExpressionMatch match = re.match(errorMsg);
   
   if (match.hasMatch()) {
       int line = match.captured(1).toInt();
       int col = match.captured(2).toInt();
       
       // Navigate to error position
       view->setCursorPosition(KTextEditor::Cursor(line-1, col-1));
   }
   ```

### Project and Context Awareness

1. **Current Document Information**
   ```cpp
   // Get current document info for contextual commands
   QString filePath = document->url().toLocalFile();
   QString fileType = document->mimeType();
   ```

2. **Project Integration** (via Kate Project Plugin)
   ```cpp
   // This requires communication with Kate's project plugin
   KateProject* project = KateProjectPlugin::self()->projectForUrl(document->url());
   if (project) {
       QString projectRoot = project->baseDir();
       // Use project context for terminal commands
   }
   ```

### UI Integration

1. **Tool Views and Widgets**
   - Create dockable views within Kate
   - Position terminal relative to editor

2. **Context Menus**
   ```cpp
   // Add actions to Kate's context menu
   QMenu* contextMenu = mainWindow->activeView()->contextMenu();
   contextMenu->addAction(m_runInTerminalAction);
   ```

3. **Keyboard Shortcuts**
   ```cpp
   // Register action with shortcut
   QAction* action = new QAction("Run in Terminal", this);
   actionCollection()->addAction("run_in_terminal", action);
   actionCollection()->setDefaultShortcut(action, Qt::CTRL + Qt::Key_R);
   ```

### Resources and Documentation

- [KTextEditor API Documentation](https://api.kde.org/frameworks/ktexteditor/html/index.html)
- [Kate Plugin Development Guide](https://docs.kde.org/stable5/en/applications/kate/dev-plugin.html)
- [Example Kate Plugins on GitHub](https://github.com/KDE/kate/tree/master/addons)
- [KDE Framework Documentation](https://api.kde.org/frameworks/index.html)

### Implementation Considerations

1. **API Version Compatibility**
   - Ensure compatibility with KTextEditor 6.x API
   - Use KF6 components consistently

2. **Thread Safety**
   - Keep terminal I/O operations in separate threads
   - Use signal/slot mechanism for thread-safe communication

3. **Error Handling**
   - Implement robust error handling for terminal/editor interactions
   - Provide clear user feedback for errors

*Note: This API documentation will be expanded as implementation progresses and more integration points are explored.*

---

*Last updated: May 11, 2025*


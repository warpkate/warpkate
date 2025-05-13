#!/bin/bash
# Script to update WarpKate with clickable file paths feature, build, and install the plugin

set -e  # Exit on error

PROJECT_DIR="/home/yani/Projects/WarpKate"
SOURCE_DIR="$PROJECT_DIR/src"
BUILD_DIR="$PROJECT_DIR/build"
CPP_FILE="$SOURCE_DIR/warpkateview.cpp"

echo "===== WarpKate Updater Script ====="
echo "This script will update WarpKate with the clickable file paths feature, build, and install the plugin."
echo

# Make sure we're in the project directory
cd "$PROJECT_DIR"

# Create backup of the original files
echo "Creating backup of original files..."
BACKUP_DIR="$PROJECT_DIR/backup_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$BACKUP_DIR/src"
cp "$CPP_FILE" "$BACKUP_DIR/src/"
echo "Backup created at $BACKUP_DIR"
echo

# Function to add isExecutable function to warpkateview.cpp
add_is_executable() {
    echo "Adding isExecutable function to warpkateview.cpp..."
    
    # Check if function already exists
    if grep -q "bool WarpKateView::isExecutable" "$CPP_FILE"; then
        echo "isExecutable function already exists. Skipping."
    else
        # Add the function at the end of the file before the last closing brace
        cat >> "$CPP_FILE" << 'EOF'

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
EOF
        echo "isExecutable function added successfully."
    fi
}

# Function to add executeFile function to warpkateview.cpp
add_execute_file() {
    echo "Adding executeFile function to warpkateview.cpp..."
    
    # Check if function already exists
    if grep -q "void WarpKateView::executeFile" "$CPP_FILE"; then
        echo "executeFile function already exists. Skipping."
    else
        # Add the function at the end of the file before the last closing brace
        cat >> "$CPP_FILE" << 'EOF'

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
EOF
        echo "executeFile function added successfully."
    fi
}

# Function to update onTerminalOutput to handle links
update_on_terminal_output() {
    echo "Updating onTerminalOutput function to handle links properly..."
    
    # Check if the function already has the link handling code
    if grep -q "setOpenExternalLinks(false)" "$CPP_FILE"; then
        echo "onTerminalOutput already has link handling code. Skipping."
    else
        # Find the end of the function where we need to add the code
        LINE_NUM=$(grep -n "} else {" "$CPP_FILE" | grep -A1 "insertHtml" | tail -n1 | cut -d':' -f1)
        
        if [ -n "$LINE_NUM" ]; then
            # Calculate the line number where we should insert our new code
            INSERT_LINE=$((LINE_NUM + 2))
            
            # Insert the new code
            sed -i "${INSERT_LINE}i\\    // Make sure links are clickable but handled by our own handler\\n    m_conversationArea->setOpenExternalLinks(false);\\n\\n    // Connect the anchorClicked signal to our onLinkClicked slot if not already connected\\n    connect(m_conversationArea, &QTextEdit::anchorClicked, \\n            this, &WarpKateView::onLinkClicked,\\n            Qt::UniqueConnection);" "$CPP_FILE"
            
            echo "onTerminalOutput function updated successfully."
        else
            echo "Failed to locate insertion point in onTerminalOutput. Skipping update."
        fi
    fi
}

# Function to update setupUI to configure text edit for links
update_setup_ui() {
    echo "Updating setupUI function to configure text edit for links..."
    
    # Check if the setupUI function already has the link handling code
    if grep -q "setOpenExternalLinks(false)" "$CPP_FILE" && grep -q "m_conversationArea->setOpenExternalLinks" "$CPP_FILE"; then
        echo "setupUI already has link handling code. Skipping."
    else
        # Find the line where we create m_conversationArea
        LINE_NUM=$(grep -n "m_conversationArea->setAcceptRichText" "$CPP_FILE" | tail -n1 | cut -d':' -f1)
        
        if [ -n "$LINE_NUM" ]; then
            # Insert the new code after that line
            INSERT_LINE=$((LINE_NUM + 1))
            
            # Insert the new code
            sed -i "${INSERT_LINE}i\\    m_conversationArea->setOpenExternalLinks(false);  // We handle link clicks ourselves" "$CPP_FILE"
            
            echo "setupUI function updated successfully."
        else
            echo "Failed to locate insertion point in setupUI. Skipping update."
        fi
    fi
}

# Main script execution
echo "Starting WarpKate updates..."
add_is_executable
add_execute_file
update_on_terminal_output
update_setup_ui
echo "Source code updates completed successfully."
echo

# Build the project
echo "Building WarpKate..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake .. -DCMAKE_INSTALL_PREFIX=`kf6-config --prefix` -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
echo "Build completed successfully."
echo

# Install the plugin
echo "Installing WarpKate plugin..."
sudo make install
echo "Installation completed successfully."
echo

echo "All tasks completed successfully! WarpKate with clickable file paths is now installed."
echo "Please restart Kate to use the new features."
echo
echo "To test the clickable file paths feature, try the following commands in the terminal:"
echo "  - ls -l"
echo "  - find . -name \"*.cpp\""
echo "  - pwd"
echo
echo "Files and directories should now be clickable in the terminal output."


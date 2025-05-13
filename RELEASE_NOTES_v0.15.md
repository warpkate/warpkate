# WarpKate v0.15 Release Notes

*Release Date: May 13, 2025*

We're excited to announce the release of WarpKate v0.15, which introduces significant usability improvements including clickable file paths in terminal output and a fix for mode switching behavior. This release enhances the integration between terminal operations and file management, offering a more seamless workflow for Kate users.

## New Features

### Clickable File Paths in Terminal Output 🔗

- **Terminal Links**: Files and directories in terminal output are now automatically detected and converted to clickable links
- **Context Menus**: Right-click on any file or directory path to access context-specific actions
- **Smart Actions**:
  - Directories open in file manager or change the current working directory
  - Text files open in Kate editor
  - Other file types open with their default applications
  - Executable files can be run directly from the terminal output
- **Visual Indicators**: Directories are displayed in bold for easier identification

### Enhanced Mode Switching with '>' Character

- **Bidirectional Toggle**: The '>' character now properly toggles between Terminal and AI modes in both directions
- **Improved User Experience**: Typing '>' at the beginning of an empty input will:
  - Switch from Terminal mode to AI mode
  - Switch from AI mode back to Terminal mode
- **Consistent UI Updates**: Mode toggle buttons are properly synchronized when switching modes

## Improvements

- **File Type Detection**: Better detection of file types for more accurate default actions
- **Terminal Output Processing**: Improved processing of ANSI escape sequences and control characters
- **Conversation Area**: Now uses `QTextBrowser` for better link handling capabilities
- **Error Handling**: More robust error handling for file operations

## Bug Fixes

- **Mode Toggle**: Fixed the '>' toggle to properly switch from AI mode back to Terminal mode
- **Terminal Output**: Fixed issues with terminal output processing that could lead to garbled text
- **Link Detection**: Improved accuracy of file path detection in various terminal output formats

## Known Issues

- Some complex terminal output with embedded control sequences may not be processed correctly
- File path detection may not work correctly with all command outputs or in all shells
- Currently lacks support for remote file paths (SSH, FTP, etc.)

## Coming in Future Versions

- Improved terminal emulation capabilities
- Enhanced AI assistance for terminal operations
- Multiple terminal views
- Session persistence across Kate restarts
- Remote file system integration

## Installation

To install WarpKate v0.15:

1. Download the latest release from the GitHub releases page
2. Use the package manager for your distribution or run the provided install script
3. Restart Kate after installation

For detailed installation instructions, please refer to the README.md file.

---

*The WarpKate Team*


# WarpKate Installation Guide

<div align="center">
  <img src="images/warpkate_logo.png" alt="WarpKate Logo" width="200"/>
  <br>
  <em>Enhance your Kate editor with Warp Terminal features and AI assistance</em>
</div>

## Introduction

WarpKate is a powerful plugin for the Kate text editor that brings the best features of Warp Terminal directly into your development environment. It provides:

- AI-assisted coding and command suggestions
- Block-based terminal execution
- Code checking and analysis
- Seamless Obsidian integration
- Smart clipboard handling

> **IMPORTANT:** WarpKate is designed to work optimally with Warp Terminal. For the full experience, you must install Warp Terminal first and create an account. This ensures that WarpKate's AI features can provide you with the best possible assistance during your development workflow.

## Prerequisites

### 1. Warp Terminal (Required)

[<img src="https://assets-global.website-files.com/6203daf47137054c031fa0e6/64d3d14694ea394d04229cc2_logo%20with%20icon%20for%20light.svg" width="150" alt="Warp Terminal">](https://www.warp.dev)

Warp Terminal is the next-generation terminal with AI built in, providing a foundation for WarpKate's functionality.

1. Visit [Warp.dev](https://www.warp.dev/) and download the Warp Terminal for your platform
2. Install and launch Warp Terminal
3. Create a Warp account (Note: free trial accounts have limited usage)
4. Set up your Warp Terminal preferences

> **Note:** Warp Terminal provides limited-use trial accounts. For full functionality, consider upgrading to a paid plan.

### 2. Kate Editor

**On Fedora:**
```bash
sudo dnf install kate
```

**On Ubuntu/Debian:**
```bash
sudo apt install kate
```

**On Arch Linux:**
```bash
sudo pacman -S kate
```

### 3. Obsidian (Optional, but recommended)

[<img src="https://obsidian.md/images/obsidian-logo-gradient.svg" width="100" alt="Obsidian">](https://obsidian.md)

Obsidian is a knowledge base application that works on top of a local folder of Markdown files.

1. Visit the [Obsidian Download Page](https://obsidian.md/download)
2. Download the appropriate version for your system:
   - For Fedora/Linux: Download the AppImage or .deb/.rpm package
   - Alternative: Install via Flatpak: `flatpak install flathub md.obsidian.Obsidian`

3. Make the AppImage executable (if using AppImage):
   ```bash
   chmod +x Obsidian-*.AppImage
   ```

4. Create a vault folder for your notes:
   ```bash
   mkdir -p ~/Documents/ObsidianVault
   ```

5. Launch Obsidian and select "Open folder as vault" to set up your new vault

### 4. Build Dependencies

**On Fedora:**
```bash
sudo dnf install git cmake extra-cmake-modules gcc-c++ kf6-ktexteditor-devel kf6-kparts-devel kf6-kio-devel qt6-qtbase-devel
```

**On Ubuntu/Debian:**
```bash
sudo apt install git cmake extra-cmake-modules build-essential libkf6texteditor-dev libkf6parts-dev libkf6kio-dev libqt6core6-dev
```

**On Arch Linux:**
```bash
sudo pacman -S git cmake extra-cmake-modules kf6-ktexteditor kf6-kparts kf6-kio qt6-base
```

## WarpKate Installation

Follow these steps carefully to ensure proper installation of WarpKate:

### 1. Clone the Repository

```bash
# Create a projects directory if you don't have one
mkdir -p ~/Projects
cd ~/Projects

# Clone the WarpKate repository
git clone https://github.com/warpkate/warpkate.git
cd warpkate
```

### 2. Build the Plugin

```bash
# Create and enter the build directory
mkdir -p build
cd build

# Configure the build
cmake ..

# Build the plugin
make
```

### 3. Install the Plugin

```bash
# Install to the system (requires sudo)
sudo make install
```

### 4. Activate the Plugin

1. Launch Kate: `kate --startanew` 
2. Press F8 to activate the WarpKate terminal panel
   - If the F8 shortcut doesn't work, you can try restarting Kate completely

## Configuration

### Initial Setup

1. Press F8 to open the WarpKate panel in Kate
2. Click the gear icon (⚙️) in the WarpKate panel to open preferences
3. Configure the following settings:

### Obsidian Integration

1. Set your Obsidian vault path (e.g., `~/Documents/ObsidianVault`)
2. Choose whether to automatically suggest saving to Obsidian
3. Customize the default filename pattern for saved notes

### AI Assistant Personalization

1. Enable custom assistant name if desired
2. Set your preferred assistant name
3. Enter your name for personalized responses
4. Choose a response style or customize the detail level and creativity

## Usage Guide

### Basic Usage

- **Show/Hide Terminal**: Press F8 to toggle the WarpKate panel
- **Switch Modes**: Use the toggle button to switch between command and AI modes
- **Execute Commands**: Type shell commands in command mode
- **Ask AI**: Type questions or requests in AI mode
- **Multi-line Input**: Use Shift+Enter for multi-line commands

### Key Features

#### Terminal Commands
- Execute any shell command directly from Kate
- Navigate directories while working on your project
- Run build commands, tests, or scripts

#### AI Assistance
- Ask coding questions
- Request explanations of complex code
- Get suggestions for improvements
- Troubleshoot errors

#### Code Check
1. Select code in your editor
2. Click the "Code Check" button
3. Review AI analysis of your code

#### Save to Obsidian
1. Have a useful conversation in WarpKate
2. Click the "Save to Obsidian" button
3. Review the suggested content to save
4. Confirm to save to your Obsidian vault

#### Insert to Editor
1. Select text in the WarpKate panel
2. Click "Insert to Editor"
3. The selected text will be inserted at your cursor position in the editor

## Troubleshooting

### Plugin Not Visible in Kate Plugin List
- This is a known issue with the current version
- The plugin is properly installed and functional despite not appearing in the list
- Test by pressing F8 - if the WarpKate panel appears, it's working correctly

### Build Errors
- Make sure all required development packages are installed
- Check for distribution-specific package names that might differ
- Verify that you have the correct KDE Framework version

### Terminal Not Working
- Ensure your Warp Terminal account is properly set up
- Check that the plugin has proper permissions
- Try restarting Kate with `kate --startanew`

### AI Features Not Responding
- Verify your Warp account is active
- Check your internet connection
- Ensure you haven't exceeded usage limits on trial accounts

## Getting Help

For issues or questions:
- Open an issue on the [GitHub repository](https://github.com/warpkate/warpkate/issues)
- Visit the [WarpKate Documentation](https://warpkate.github.io/)
- Contact the developers at support@warpkate.dev

---

<div align="center">
  <p>WarpKate © 2025 WarpKate Contributors</p>
  <p>
    <a href="https://github.com/warpkate/warpkate/blob/main/LICENSE">GPL-3.0 License</a> •
    <a href="https://github.com/warpkate/warpkate">GitHub</a>
  </p>
</div>


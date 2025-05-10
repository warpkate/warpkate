# Contributing to WarpKate

Thank you for your interest in contributing to WarpKate! This document provides guidelines and instructions for contributing to the project.

## Development Environment Setup

### Prerequisites

- C++ 17 or later
- Qt 5.15 or Qt 6.x
- KDE Frameworks 5.80+
- CMake 3.16+
- Kate development headers

### Setting Up Your Environment

1. Install the required dependencies:
   ```bash
   # For Fedora/RHEL
   sudo dnf install cmake extra-cmake-modules qt5-qtbase-devel kf5-kio-devel kf5-kparts-devel kf5-ktexteditor-devel kf5-ki18n-devel kate-devel
   
   # For Ubuntu/Debian
   sudo apt install cmake extra-cmake-modules qtbase5-dev libkf5kio-dev libkf5parts-dev libkf5texteditor-dev libkf5i18n-dev kate-dev
   
   # For Arch Linux
   sudo pacman -S cmake extra-cmake-modules qt5-base kio kparts ktexteditor ki18n kate
   ```

2. Clone the repository:
   ```bash
   git clone https://github.com/warpkate/warpkate.git
   cd warpkate
   ```

3. Create a build directory and build the project:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

## Development Workflow

1. **Fork the repository** on GitHub.

2. **Create a branch** for your feature or bugfix:
   ```bash
   git checkout -b feature/your-feature-name
   # or
   git checkout -b fix/issue-description
   ```

3. **Make your changes** and commit them with clear, descriptive commit messages:
   ```bash
   git commit -m "Add feature: description of the feature"
   ```

4. **Push your branch** to your fork:
   ```bash
   git push origin feature/your-feature-name
   ```

5. **Submit a Pull Request** (PR) to the main repository.

6. **Address review feedback** and update your PR as needed.

## Coding Style Guidelines

WarpKate follows the [KDE coding style](https://community.kde.org/Policies/Frameworks_Coding_Style):

- Use 4 spaces for indentation (no tabs)
- Class names should be in CamelCase
- Methods and variables should be in camelCase
- Member variables should be prefixed with `m_`
- Follow Qt naming conventions for slots: `onSomethingHappened`
- Use Doxygen-style comments for public APIs
- Keep lines under 100 characters when possible

For automatic formatting, you can use:
```bash
clang-format -style=file -i src/*.cpp src/*.h
```

## Testing Requirements

- All new features should include appropriate tests
- All bug fixes should include a test that reproduces the fixed issue
- Run existing tests before submitting a PR:
  ```bash
  cd build
  make test
  ```

- For UI changes, verify that the plugin works correctly in Kate

## Reporting Issues

When reporting issues, please include:

1. A clear description of the issue
2. Steps to reproduce
3. Expected vs. actual behavior
4. Version information:
   - WarpKate version
   - KDE Frameworks version
   - Qt version
   - Operating system and version

Use the GitHub issue tracker with the appropriate template.

## Review Process

1. All code changes require review before merging
2. PR needs approval from at least one maintainer
3. CI checks must pass (build, tests, linting)
4. Large changes may require discussion in the KDE community

## Communication

- Join the KDE development mailing lists
- Use the project's GitHub discussions for questions
- Follow the [KDE Code of Conduct](https://kde.org/code-of-conduct/)

Thank you for contributing to WarpKate!


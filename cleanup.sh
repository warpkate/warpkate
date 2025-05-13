#!/bin/bash
# WarpKate Cleanup Script
# This script cleans up temporary files and the build environment after a release

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Header
echo -e "${BLUE}===== WarpKate Cleanup Script =====${NC}"
echo "This script will clean up temporary files and prepare for the next development phase."

# Default configuration
CLEAN_BUILD=true      # Clean build directory
CLEAN_BACKUPS=false   # Clean old backup directories
CLEAN_TEMP=true       # Clean temporary files
CLEAN_DEEP=false      # Deep clean (more aggressive)
KEEP_BACKUPS=3        # Number of most recent backups to keep

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --no-build)
            CLEAN_BUILD=false
            shift
            ;;
        --clean-backups)
            CLEAN_BACKUPS=true
            shift
            ;;
        --keep-backups=*)
            KEEP_BACKUPS="${1#*=}"
            CLEAN_BACKUPS=true
            shift
            ;;
        --deep-clean)
            CLEAN_DEEP=true
            shift
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --no-build          Don't clean build directory"
            echo "  --clean-backups     Clean old backup directories"
            echo "  --keep-backups=N    Keep N most recent backups (default: 3)"
            echo "  --deep-clean        Perform a deeper cleanup (more aggressive)"
            echo "  --help              Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Function to ask for confirmation
confirm() {
    read -p "$1 (y/n): " response
    case "$response" in
        [yY][eE][sS]|[yY]) 
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

# Step 1: Clean build directory
if $CLEAN_BUILD; then
    echo -e "${YELLOW}Step 1: Cleaning build directory...${NC}"
    
    if [ -d "build" ]; then
        if $CLEAN_DEEP; then
            echo "Performing deep clean of build directory..."
            rm -rf build
            mkdir -p build
            echo -e "${GREEN}Build directory has been reset${NC}"
        else
            echo "Cleaning build artifacts..."
            
            # Clean CMake build artifacts
            if [ -f "build/Makefile" ]; then
                (cd build && make clean)
            fi
            
            # Remove other common build artifacts
            find build -name "*.o" -delete
            find build -name "moc_*.cpp" -delete
            find build -name "*.a" -delete
            find build -name "CMakeCache.txt" -delete
            find build -name "CMakeFiles" -type d -exec rm -rf {} +
            
            echo -e "${GREEN}Build artifacts cleaned${NC}"
        fi
    else
        echo "Build directory not found. Nothing to clean."
    fi
    echo ""
fi

# Step 2: Clean old backup directories
if $CLEAN_BACKUPS; then
    echo -e "${YELLOW}Step 2: Cleaning old backup directories...${NC}"
    
    # Find all backup directories
    BACKUPS=($(find . -maxdepth 1 -type d -name "backup_*" | sort -r))
    
    if [ ${#BACKUPS[@]} -le $KEEP_BACKUPS ]; then
        echo "Found ${#BACKUPS[@]} backup directories, keeping all (threshold: $KEEP_BACKUPS)"
    else
        # Keep the N most recent backups
        TO_DELETE=("${BACKUPS[@]:$KEEP_BACKUPS}")
        
        echo "Found ${#BACKUPS[@]} backup directories, keeping $KEEP_BACKUPS most recent"
        echo "The following backup directories will be deleted:"
        
        for backup in "${TO_DELETE[@]}"; do
            echo "  - $backup"
        done
        
        if confirm "Are you sure you want to delete these backup directories?"; then
            for backup in "${TO_DELETE[@]}"; do
                rm -rf "$backup"
                echo "Deleted: $backup"
            done
            echo -e "${GREEN}Old backup directories cleaned${NC}"
        else
            echo "Skipped deleting backup directories"
        fi
    fi
    echo ""
fi

# Step 3: Clean temporary files
if $CLEAN_TEMP; then
    echo -e "${YELLOW}Step 3: Cleaning temporary files...${NC}"
    
    # Clean common temporary files
    find . -name "*.o" -delete
    find . -name "*.tmp" -delete
    find . -name "*~" -delete
    find . -name ".*.swp" -delete
    find . -name "core" -delete
    
    # Clean IDE/editor temporary files
    find . -name ".vscode" -type d -exec rm -rf {} +
    find . -name ".idea" -type d -exec rm -rf {} +
    find . -name "__pycache__" -type d -exec rm -rf {} +
    find . -name ".DS_Store" -delete
    
    # Clean old log files
    find . -name "*.log" -delete
    
    echo -e "${GREEN}Temporary files cleaned${NC}"
    echo ""
fi

# Step 4: Clean release files if requested
if $CLEAN_DEEP; then
    echo -e "${YELLOW}Step 4: Performing deep cleanup...${NC}"
    
    if confirm "This will remove all release directories. Continue?"; then
        # Remove release directories
        find . -maxdepth 1 -type d -name "release_v*" -exec rm -rf {} +
        echo "Removed release directories"
        
        # Remove any tarballs or zip files
        find . -maxdepth 1 -name "*.tar.gz" -delete
        find . -maxdepth 1 -name "*.zip" -delete
        echo "Removed archive files"
        
        echo -e "${GREEN}Deep cleanup completed${NC}"
    else
        echo "Skipped deep cleanup"
    fi
    echo ""
fi

echo -e "${BLUE}===== Cleanup completed =====${NC}"
echo "The development environment has been cleaned up and is ready for the next phase of work."
echo ""
echo "If you need to perform a more specific cleanup, use the following options:"
echo "  ./cleanup.sh --clean-backups     Clean old backup directories"
echo "  ./cleanup.sh --deep-clean        Perform a deeper cleanup"
echo "  ./cleanup.sh --help              Show all options"


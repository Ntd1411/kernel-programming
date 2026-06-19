#!/bin/bash

# common.sh - Shared functions for demo modules
# Sourced by all demo-*.sh files

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Print colored header
print_header() {
    echo ""
    echo -e "${CYAN}========================================${NC}"
    echo -e "${CYAN}  $1${NC}"
    echo -e "${CYAN}========================================${NC}"
    echo ""
}

# Print step/section
print_step() {
    echo -e "${BLUE}▶ $1${NC}"
}

# Print success message
print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

# Print info message
print_info() {
    echo -e "${YELLOW}ℹ $1${NC}"
}

# Print error message
print_error() {
    echo -e "${RED}✗ $1${NC}"
}

# Pause for user input
pause() {
    echo ""
    read -p "Nhấn Enter để tiếp tục..." dummy
    echo ""
}

# Setup demo directory
setup_demo_dir() {
    DEMO_DIR="$HOME/shell_demo_$(date +%s)"
    mkdir -p "$DEMO_DIR"/{source,backup,temp}
    echo "$DEMO_DIR"
}

# Cleanup demo directory
cleanup_demo_dir() {
    local demo_dir="$1"
    if [ -d "$demo_dir" ]; then
        print_info "Cleaning up demo directory: $demo_dir"
        rm -rf "$demo_dir"
    fi
}
#!/bin/bash

# demo.sh - Shell Scripting Demo Entry Point (Modular)
# Menu-driven interface to run individual module demos

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEMO_DIR="${SCRIPT_DIR}/demo"

# Source common functions
if [ ! -f "${DEMO_DIR}/common.sh" ]; then
    echo "Error: demo/common.sh not found!"
    exit 1
fi

source "${DEMO_DIR}/common.sh"

# Show main menu
show_menu() {
    print_header "Shell Scripting Demo - Main Menu"
    echo "Choose a module to demo:"
    echo ""
    echo "  1. File Management Demo"
    echo "  2. Time Management Demo"
    echo "  3. Package Management Demo"
    echo "  4. Task Scheduler Demo"
    echo "  5. Advanced Workflows Demo"
    echo ""
    echo "  0. Exit"
    echo ""
}

# Run all demos
run_all_demos() {
    print_header "Running All Demos"
    "${DEMO_DIR}/demo-file-management.sh"
    echo ""
    "${DEMO_DIR}/demo-time-management.sh"
    echo ""
    "${DEMO_DIR}/demo-package-management.sh"
    echo ""
    "${DEMO_DIR}/demo-task-scheduler.sh"
    echo ""
    "${DEMO_DIR}/demo-advanced.sh"
    echo ""
    print_success "All demos completed!"
}

# Main loop
while true; do
    show_menu
    read -p "Choose an option (0-5): " choice
    echo ""
    
    case $choice in
        1)
            "${DEMO_DIR}/demo-file-management.sh"
            ;;
        2)
            "${DEMO_DIR}/demo-time-management.sh"
            ;;
        3)
            "${DEMO_DIR}/demo-package-management.sh"
            ;;
        4)
            "${DEMO_DIR}/demo-task-scheduler.sh"
            ;;
        5)
            "${DEMO_DIR}/demo-advanced.sh"
            ;;
        0)
            print_info "Goodbye!"
            exit 0
            ;;
        *)
            print_error "Invalid choice. Please choose 0-5."
            ;;
    esac
    
    echo ""
    read -p "Press Enter to return to main menu..."
    echo ""
done

#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG_DIR="${SCRIPT_DIR}/../../logs"
LOG_FILE="${LOG_DIR}/package_manager.log"

mkdir -p "$LOG_DIR"

log_message() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

detect_package_manager() {
    if command -v apt-get &> /dev/null; then
        echo "apt"
    elif command -v dnf &> /dev/null; then
        echo "dnf"
    elif command -v yum &> /dev/null; then
        echo "yum"
    elif command -v pacman &> /dev/null; then
        echo "pacman"
    elif command -v zypper &> /dev/null; then
        echo "zypper"
    else
        echo "unknown"
    fi
}

install_package() {
    local pkg_manager="$1"
    local package="$2"
    
    log_message "Installing package: $package"
    
    case "$pkg_manager" in
        apt)
            sudo apt-get update
            sudo apt-get install -y "$package"
            ;;
        dnf)
            sudo dnf install -y "$package"
            ;;
        yum)
            sudo yum install -y "$package"
            ;;
        pacman)
            sudo pacman -S --noconfirm "$package"
            ;;
        zypper)
            sudo zypper install -y "$package"
            ;;
        *)
            log_message "ERROR: Unsupported package manager"
            return 1
            ;;
    esac
    
    log_message "Successfully installed: $package"
}

remove_package() {
    local pkg_manager="$1"
    local package="$2"
    
    log_message "Removing package: $package"
    
    case "$pkg_manager" in
        apt)
            sudo apt-get remove -y "$package"
            sudo apt-get autoremove -y
            ;;
        dnf)
            sudo dnf remove -y "$package"
            ;;
        yum)
            sudo yum remove -y "$package"
            ;;
        pacman)
            sudo pacman -R --noconfirm "$package"
            ;;
        zypper)
            sudo zypper remove -y "$package"
            ;;
        *)
            log_message "ERROR: Unsupported package manager"
            return 1
            ;;
    esac
    
    log_message "Successfully removed: $package"
}

update_system() {
    local pkg_manager="$1"
    
    log_message "Updating system packages"
    
    case "$pkg_manager" in
        apt)
            sudo apt-get update
            sudo apt-get upgrade -y
            ;;
        dnf)
            sudo dnf upgrade -y
            ;;
        yum)
            sudo yum update -y
            ;;
        pacman)
            sudo pacman -Syu --noconfirm
            ;;
        zypper)
            sudo zypper update -y
            ;;
        *)
            log_message "ERROR: Unsupported package manager"
            return 1
            ;;
    esac
    
    log_message "System update completed"
}

search_package() {
    local pkg_manager="$1"
    local package="$2"
    
    log_message "Searching for package: $package"
    
    case "$pkg_manager" in
        apt)
            apt-cache search "$package"
            ;;
        dnf)
            dnf search "$package"
            ;;
        yum)
            yum search "$package"
            ;;
        pacman)
            pacman -Ss "$package"
            ;;
        zypper)
            zypper search "$package"
            ;;
        *)
            log_message "ERROR: Unsupported package manager"
            return 1
            ;;
    esac
}

list_installed() {
    local pkg_manager="$1"
    
    case "$pkg_manager" in
        apt)
            dpkg -l | grep '^ii'
            ;;
        dnf)
            dnf list installed
            ;;
        yum)
            yum list installed
            ;;
        pacman)
            pacman -Q
            ;;
        zypper)
            zypper packages --installed-only
            ;;
        *)
            log_message "ERROR: Unsupported package manager"
            return 1
            ;;
    esac
}

show_usage() {
    cat << EOF
Usage: $0 [OPTION] [PACKAGE]

Package management operations:
    install PACKAGE     Install a package
    remove PACKAGE      Remove a package
    update             Update all packages
    search PACKAGE     Search for a package
    list               List installed packages
    help               Show this help message

Examples:
    $0 install vim
    $0 remove vim
    $0 update
    $0 search nginx
    $0 list
EOF
}

main() {
    if [ $# -eq 0 ]; then
        show_usage
        exit 1
    fi
    
    local pkg_manager
    pkg_manager=$(detect_package_manager)
    
    if [ "$pkg_manager" = "unknown" ]; then
        log_message "ERROR: No supported package manager found"
        exit 1
    fi
    
    log_message "Detected package manager: $pkg_manager"
    
    case "$1" in
        install)
            if [ $# -ne 2 ]; then
                echo "ERROR: Package name required"
                exit 1
            fi
            install_package "$pkg_manager" "$2"
            ;;
        remove)
            if [ $# -ne 2 ]; then
                echo "ERROR: Package name required"
                exit 1
            fi
            remove_package "$pkg_manager" "$2"
            ;;
        update)
            update_system "$pkg_manager"
            ;;
        search)
            if [ $# -ne 2 ]; then
                echo "ERROR: Package name required"
                exit 1
            fi
            search_package "$pkg_manager" "$2"
            ;;
        list)
            list_installed "$pkg_manager"
            ;;
        help|--help|-h)
            show_usage
            ;;
        *)
            echo "ERROR: Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
}

main "$@"

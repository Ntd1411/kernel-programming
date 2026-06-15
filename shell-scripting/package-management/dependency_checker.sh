#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG_DIR="${SCRIPT_DIR}/../../logs"
LOG_FILE="${LOG_DIR}/dependency_checker.log"

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
    else
        echo "unknown"
    fi
}

check_dependencies_apt() {
    local package="$1"
    
    echo "Dependencies for $package:"
    apt-cache depends "$package" | grep -E "Depends|PreDepends" | awk '{print $2}'
    
    echo ""
    echo "Reverse dependencies (packages that depend on $package):"
    apt-cache rdepends "$package" | tail -n +3
}

check_dependencies_dnf() {
    local package="$1"
    
    echo "Dependencies for $package:"
    dnf repoquery --requires "$package"
    
    echo ""
    echo "Reverse dependencies:"
    dnf repoquery --whatrequires "$package"
}

check_dependencies_yum() {
    local package="$1"
    
    echo "Dependencies for $package:"
    yum deplist "$package" | grep provider | awk '{print $2}'
}

check_dependencies_pacman() {
    local package="$1"
    
    echo "Dependencies for $package:"
    pactree "$package"
    
    echo ""
    echo "Reverse dependencies:"
    pactree -r "$package"
}

check_broken_dependencies() {
    local pkg_manager="$1"
    
    log_message "Checking for broken dependencies"
    
    case "$pkg_manager" in
        apt)
            echo "Broken packages:"
            dpkg -l | grep -E "^iF|^iU" || echo "No broken packages found"
            
            echo ""
            echo "Checking apt integrity:"
            sudo apt-get check
            ;;
        dnf)
            echo "Checking for problems:"
            dnf check
            ;;
        yum)
            echo "Checking for problems:"
            package-cleanup --problems
            ;;
        pacman)
            echo "Checking for broken dependencies:"
            pacman -Qk 2>&1 | grep -i "warning" || echo "No issues found"
            ;;
        *)
            log_message "ERROR: Unsupported package manager"
            return 1
            ;;
    esac
}

fix_broken_dependencies() {
    local pkg_manager="$1"
    
    log_message "Attempting to fix broken dependencies"
    
    case "$pkg_manager" in
        apt)
            sudo apt-get install -f
            sudo dpkg --configure -a
            ;;
        dnf)
            sudo dnf check
            sudo dnf autoremove
            ;;
        yum)
            sudo package-cleanup --cleandupes
            sudo yum-complete-transaction
            ;;
        pacman)
            sudo pacman -Dk
            ;;
        *)
            log_message "ERROR: Unsupported package manager"
            return 1
            ;;
    esac
    
    log_message "Fix attempt completed"
}

list_orphaned_packages() {
    local pkg_manager="$1"
    
    log_message "Listing orphaned packages"
    
    case "$pkg_manager" in
        apt)
            echo "Orphaned packages (no longer needed):"
            apt-get autoremove --dry-run | grep -Po '^\s+\K[^ ]+'
            ;;
        dnf)
            echo "Orphaned packages:"
            dnf list extras
            ;;
        yum)
            echo "Orphaned packages:"
            package-cleanup --leaves
            ;;
        pacman)
            echo "Orphaned packages:"
            pacman -Qdtq || echo "No orphaned packages"
            ;;
        *)
            log_message "ERROR: Unsupported package manager"
            return 1
            ;;
    esac
}

remove_orphaned_packages() {
    local pkg_manager="$1"
    
    log_message "Removing orphaned packages"
    
    case "$pkg_manager" in
        apt)
            sudo apt-get autoremove -y
            sudo apt-get autoclean
            ;;
        dnf)
            sudo dnf autoremove -y
            ;;
        yum)
            sudo package-cleanup --leaves --all
            ;;
        pacman)
            orphans=$(pacman -Qdtq)
            if [ -n "$orphans" ]; then
                sudo pacman -Rns --noconfirm $orphans
            else
                echo "No orphaned packages to remove"
            fi
            ;;
        *)
            log_message "ERROR: Unsupported package manager"
            return 1
            ;;
    esac
    
    log_message "Orphaned packages removed"
}

show_package_info() {
    local pkg_manager="$1"
    local package="$2"
    
    case "$pkg_manager" in
        apt)
            apt-cache show "$package"
            ;;
        dnf)
            dnf info "$package"
            ;;
        yum)
            yum info "$package"
            ;;
        pacman)
            pacman -Si "$package"
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

Dependency checking operations:
    check PACKAGE       Show dependencies for a package
    broken             Check for broken dependencies
    fix                Fix broken dependencies
    orphans            List orphaned packages
    clean              Remove orphaned packages
    info PACKAGE       Show detailed package information
    help               Show this help message

Examples:
    $0 check nginx
    $0 broken
    $0 fix
    $0 orphans
    $0 clean
    $0 info vim
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
    
    log_message "Using package manager: $pkg_manager"
    
    case "$1" in
        check)
            if [ $# -ne 2 ]; then
                echo "ERROR: Package name required"
                exit 1
            fi
            case "$pkg_manager" in
                apt) check_dependencies_apt "$2" ;;
                dnf) check_dependencies_dnf "$2" ;;
                yum) check_dependencies_yum "$2" ;;
                pacman) check_dependencies_pacman "$2" ;;
            esac
            ;;
        broken)
            check_broken_dependencies "$pkg_manager"
            ;;
        fix)
            fix_broken_dependencies "$pkg_manager"
            ;;
        orphans)
            list_orphaned_packages "$pkg_manager"
            ;;
        clean)
            remove_orphaned_packages "$pkg_manager"
            ;;
        info)
            if [ $# -ne 2 ]; then
                echo "ERROR: Package name required"
                exit 1
            fi
            show_package_info "$pkg_manager" "$2"
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

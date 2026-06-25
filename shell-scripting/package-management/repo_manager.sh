#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG_DIR="${SCRIPT_DIR}/../../logs"
LOG_FILE="${LOG_DIR}/repo_manager.log"

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

list_repositories_apt() {
    log_message "Listing APT repositories"
    
    echo "=== Main repositories ==="
    # Kiểm tra nếu dùng định dạng mới DEB822 (Ubuntu 24.04+)
    if [ -f /etc/apt/sources.list.d/ubuntu.sources ]; then
        grep -E "^URIs:|^Suites:|^Components:" /etc/apt/sources.list.d/ubuntu.sources
    # Nếu là Ubuntu đời cũ dùng sources.list truyền thống
    elif [ -f /etc/apt/sources.list ]; then
        grep -E "^deb " /etc/apt/sources.list | grep -v "^#"
    else
        echo "No main repository file found."
    fi
    
    echo ""
    echo "=== Additional repositories ==="
    if [ -d /etc/apt/sources.list.d/ ]; then
        # Thêm kiểm tra loại trừ file ubuntu.sources của hệ thống ra
        for file in /etc/apt/sources.list.d/*; do
            if [ -f "$file" ] && [ "$file" != "/etc/apt/sources.list.d/ubuntu.sources" ]; then
                echo "File: $file"
                # Quét cho cả file .list truyền thống và file .sources cấu hình mới
                if [[ "$file" == *.sources ]]; then
                    grep -E "^URIs:" "$file"
                else
                    grep -E "^deb " "$file" | grep -v "^#"
                fi
                echo ""
            fi
        done
    fi
    
    echo ""
    echo "=== PPA repositories ==="
    apt-cache policy | grep -E "http|file" | awk '{print $2}' | sort -u
}

list_repositories_dnf() {
    log_message "Listing DNF repositories"
    
    dnf repolist all
}

list_repositories_yum() {
    log_message "Listing YUM repositories"
    
    yum repolist all
}

list_repositories_pacman() {
    log_message "Listing Pacman repositories"
    
    echo "=== Configured repositories ==="
    grep -E "^\[.*\]|^Server" /etc/pacman.conf | grep -v "^#"
    
    echo ""
    echo "=== Mirror list ==="
    if [ -f /etc/pacman.d/mirrorlist ]; then
        grep "^Server" /etc/pacman.d/mirrorlist | head -10
    fi
}

add_repository_apt() {
    local repo="$1"
    
    log_message "Adding APT repository: $repo"
    
    if [[ "$repo" == ppa:* ]]; then
        sudo add-apt-repository -y "$repo"
    else
        echo "$repo" | sudo tee -a /etc/apt/sources.list.d/custom.list
    fi
    
    sudo apt-get update
    log_message "Repository added successfully"
}

add_repository_dnf() {
    local repo_url="$1"
    local repo_name="${2:-custom-repo}"
    
    log_message "Adding DNF repository: $repo_name"
    
    sudo dnf config-manager --add-repo "$repo_url"
    sudo dnf makecache
    
    log_message "Repository added successfully"
}

add_repository_yum() {
    local repo_url="$1"
    local repo_name="${2:-custom-repo}"
    
    log_message "Adding YUM repository: $repo_name"
    
    sudo yum-config-manager --add-repo "$repo_url"
    sudo yum makecache
    
    log_message "Repository added successfully"
}

remove_repository_apt() {
    local repo="$1"
    
    log_message "Removing APT repository: $repo"
    
    if [[ "$repo" == ppa:* ]]; then
        sudo add-apt-repository -r -y "$repo"
    else
        echo "Please manually remove from /etc/apt/sources.list or /etc/apt/sources.list.d/"
    fi
    
    sudo apt-get update
    log_message "Repository removed"
}

remove_repository_dnf() {
    local repo_name="$1"
    
    log_message "Disabling DNF repository: $repo_name"
    
    sudo dnf config-manager --disable "$repo_name"
    
    log_message "Repository disabled"
}

remove_repository_yum() {
    local repo_name="$1"
    
    log_message "Disabling YUM repository: $repo_name"
    
    sudo yum-config-manager --disable "$repo_name"
    
    log_message "Repository disabled"
}

update_repository_cache() {
    local pkg_manager="$1"
    
    log_message "Updating repository cache"
    
    case "$pkg_manager" in
        apt)
            sudo apt-get update
            ;;
        dnf)
            sudo dnf makecache
            ;;
        yum)
            sudo yum makecache
            ;;
        pacman)
            sudo pacman -Sy
            ;;
        *)
            log_message "ERROR: Unsupported package manager"
            return 1
            ;;
    esac
    
    log_message "Cache updated successfully"
}

clean_repository_cache() {
    local pkg_manager="$1"
    
    log_message "Cleaning repository cache"
    
    case "$pkg_manager" in
        apt)
            sudo apt-get clean
            sudo apt-get autoclean
            ;;
        dnf)
            sudo dnf clean all
            ;;
        yum)
            sudo yum clean all
            ;;
        pacman)
            sudo pacman -Sc --noconfirm
            ;;
        *)
            log_message "ERROR: Unsupported package manager"
            return 1
            ;;
    esac
    
    log_message "Cache cleaned successfully"
}

show_repo_info() {
    local pkg_manager="$1"
    local repo_name="$2"
    
    case "$pkg_manager" in
        apt)
            echo "Repository information not directly available for APT"
            echo "Use: apt-cache policy"
            ;;
        dnf)
            dnf repoinfo "$repo_name"
            ;;
        yum)
            yum repoinfo "$repo_name"
            ;;
        pacman)
            echo "Repository configuration:"
            grep -A 5 "\[$repo_name\]" /etc/pacman.conf
            ;;
        *)
            log_message "ERROR: Unsupported package manager"
            return 1
            ;;
    esac
}

show_usage() {
    cat << EOF
Usage: $0 [OPTION] [ARGUMENTS]

Repository management operations:
    list                List all repositories
    add REPO [NAME]     Add a repository
    remove REPO         Remove/disable a repository
    update              Update repository cache
    clean               Clean repository cache
    info REPO           Show repository information
    help                Show this help message

Examples:
    $0 list
    $0 add ppa:ubuntu-toolchain-r/test
    $0 add https://example.com/repo custom-repo
    $0 remove ppa:ubuntu-toolchain-r/test
    $0 update
    $0 clean
    $0 info custom-repo
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
        list)
            case "$pkg_manager" in
                apt) list_repositories_apt ;;
                dnf) list_repositories_dnf ;;
                yum) list_repositories_yum ;;
                pacman) list_repositories_pacman ;;
            esac
            ;;
        add)
            if [ $# -lt 2 ]; then
                echo "ERROR: Repository URL/PPA required"
                exit 1
            fi
            case "$pkg_manager" in
                apt) add_repository_apt "$2" ;;
                dnf) add_repository_dnf "$2" "${3:-custom-repo}" ;;
                yum) add_repository_yum "$2" "${3:-custom-repo}" ;;
                *)
                    echo "ERROR: Add operation not supported for $pkg_manager"
                    exit 1
                    ;;
            esac
            ;;
        remove)
            if [ $# -ne 2 ]; then
                echo "ERROR: Repository name required"
                exit 1
            fi
            case "$pkg_manager" in
                apt) remove_repository_apt "$2" ;;
                dnf) remove_repository_dnf "$2" ;;
                yum) remove_repository_yum "$2" ;;
                *)
                    echo "ERROR: Remove operation not supported for $pkg_manager"
                    exit 1
                    ;;
            esac
            ;;
        update)
            update_repository_cache "$pkg_manager"
            ;;
        clean)
            clean_repository_cache "$pkg_manager"
            ;;
        info)
            if [ $# -ne 2 ]; then
                echo "ERROR: Repository name required"
                exit 1
            fi
            show_repo_info "$pkg_manager" "$2"
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

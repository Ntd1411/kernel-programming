#!/bin/bash

# demo-package-management.sh - Package Management Module Demo
# Demonstrates package search, dependencies, and repository management

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PARENT_DIR="$(dirname "$SCRIPT_DIR")"

# Source common functions
source "${SCRIPT_DIR}/common.sh"

print_header "Package Management Demo"

#=============================================================================
# Demo 3.1: Search Packages
#=============================================================================

print_step "3.1 Searching for packages"
echo "Searching for 'vim':"
"${PARENT_DIR}/package-management/package_manager.sh" search vim | head -10
print_success "Package search completed"
pause

#=============================================================================
# Demo 3.2: Check Dependencies
#=============================================================================

print_step "3.2 Checking package dependencies"
"${PARENT_DIR}/package-management/dependency_checker.sh" check bash 2>/dev/null || \
    print_info "Note: Some commands may require package to be installed"
print_success "Dependency check completed"
pause

#=============================================================================
# Demo 3.3: List Repositories
#=============================================================================

print_step "3.3 Listing repositories"
"${PARENT_DIR}/package-management/repo_manager.sh" list | head -10
print_success "Repository list displayed"
pause

#=============================================================================
# Summary
#=============================================================================

print_header "Package Management Demo - Summary"
echo "✓ Package search performed"
echo "✓ Dependencies checked"
echo "✓ Repositories listed"
echo ""
print_success "Package Management demo completed!"
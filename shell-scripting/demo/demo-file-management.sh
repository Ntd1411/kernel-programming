#!/bin/bash

# demo-file-management.sh - File Management Module Demo
# Demonstrates backup, duplicates, and cleanup features

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PARENT_DIR="$(dirname "$SCRIPT_DIR")"

# Source common functions
source "${SCRIPT_DIR}/common.sh"

# Setup demo environment
DEMO_DIR=$(setup_demo_dir)
trap "cleanup_demo_dir '$DEMO_DIR'" EXIT

print_header "File Management Demo"

#=============================================================================
# Demo 1.1: Create Test Files
#=============================================================================

print_step "1.1 Creating test files"
for i in {1..3}; do
    echo "Content of file $i - $(date)" > "$DEMO_DIR/source/file$i.txt"
done
echo "Duplicate content" > "$DEMO_DIR/source/dup1.txt"
echo "Duplicate content" > "$DEMO_DIR/source/dup2.txt"
dd if=/dev/zero of="$DEMO_DIR/source/largefile.bin" bs=1M count=10 2>/dev/null
ls -lh "$DEMO_DIR/source/"
print_success "Test files created"
pause

#=============================================================================
# Demo 1.2: Backup
#=============================================================================

print_step "1.2 Backup with retention policy"
"${PARENT_DIR}/file-management/backup.sh" "$DEMO_DIR/source" "$DEMO_DIR/backup" 7
print_success "Backup completed"
echo ""
echo "Backup files:"
ls -lh "$DEMO_DIR/backup/"
pause

#=============================================================================
# Demo 1.3: Find Duplicates
#=============================================================================

print_step "1.3 Finding duplicate files"
"${PARENT_DIR}/file-management/find_duplicates.sh" "$DEMO_DIR/source" list
print_success "Duplicate detection completed"
pause

#=============================================================================
# Demo 1.4: Cleanup (Dry-run)
#=============================================================================

print_step "1.4 Cleanup test (dry-run mode)"
touch "$DEMO_DIR/temp/temp_file.tmp"
echo "Testing cleanup in dry-run mode:"
"${PARENT_DIR}/file-management/cleanup.sh" -d 0 -n -v || true
print_success "Cleanup check completed (no files deleted)"
pause

#=============================================================================
# Summary
#=============================================================================

print_header "File Management Demo - Summary"
echo "✓ Test files created (3 files + 2 duplicates + 1 large file)"
echo "✓ Backup completed with retention policy"
echo "✓ Duplicate files detected"
echo "✓ Cleanup tested in dry-run mode"
echo ""
print_success "File Management demo completed!"
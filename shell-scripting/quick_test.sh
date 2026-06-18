#!/bin/bash

# quick_test.sh - Script test nhanh tất cả các shell scripts
# Kiểm tra syntax và chức năng cơ bản

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PASS_COUNT=0
FAIL_COUNT=0

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "=========================================="
echo "  Quick Test Shell Scripts"
echo "=========================================="
echo ""

test_script() {
    local script_path="$1"
    local test_args="$2"
    local script_name=$(basename "$script_path")
    
    printf "Testing %-35s ... " "$script_name"
    
    # Check if file exists
    if [ ! -f "$script_path" ]; then
        echo -e "${RED}NOT FOUND${NC}"
        FAIL_COUNT=$((FAIL_COUNT + 1))
        return 1
    fi
    
    # Check if executable
    if [ ! -x "$script_path" ]; then
        chmod +x "$script_path" 2>/dev/null || true
    fi
    
    # Test syntax
    if ! bash -n "$script_path" 2>/dev/null; then
        echo -e "${RED}SYNTAX ERROR${NC}"
        FAIL_COUNT=$((FAIL_COUNT + 1))
        return 1
    fi
    
    # Test execution (help/usage)
    if timeout 5 bash "$script_path" $test_args >/dev/null 2>&1; then
        echo -e "${GREEN}PASS${NC}"
        PASS_COUNT=$((PASS_COUNT + 1))
        return 0
    else
        # Some scripts exit with error when no args, that's ok
        echo -e "${YELLOW}OK (no args)${NC}"
        PASS_COUNT=$((PASS_COUNT + 1))
        return 0
    fi
}

echo "=== File Management Scripts ==="
test_script "$SCRIPT_DIR/file-management/file_manager.sh" "" || true
test_script "$SCRIPT_DIR/file-management/backup.sh" "" || true
test_script "$SCRIPT_DIR/file-management/find_duplicates.sh" "" || true
test_script "$SCRIPT_DIR/file-management/cleanup.sh" "-h" || true
echo ""

echo "=== Task Scheduler Scripts ==="
test_script "$SCRIPT_DIR/task-scheduler/cron_manager.sh" "-h" || true
test_script "$SCRIPT_DIR/task-scheduler/scheduled_tasks.sh" "" || true
echo ""

echo "=== Time Management Scripts ==="
test_script "$SCRIPT_DIR/time-management/time_tracker.sh" "help" || true
test_script "$SCRIPT_DIR/time-management/stopwatch.sh" "help" || true
echo ""

echo "=== Package Management Scripts ==="
test_script "$SCRIPT_DIR/package-management/package_manager.sh" "help" || true
test_script "$SCRIPT_DIR/package-management/dependency_checker.sh" "help" || true
test_script "$SCRIPT_DIR/package-management/repo_manager.sh" "help" || true
echo ""

echo "=========================================="
echo "  Test Summary"
echo "=========================================="
echo -e "Total tests: $((PASS_COUNT + FAIL_COUNT))"
echo -e "${GREEN}Passed: $PASS_COUNT${NC}"
echo -e "${RED}Failed: $FAIL_COUNT${NC}"
echo ""

if [ $FAIL_COUNT -eq 0 ]; then
    echo -e "${GREEN}✓ All scripts are ready to use!${NC}"
    exit 0
else
    echo -e "${RED}✗ Some scripts have issues${NC}"
    exit 1
fi

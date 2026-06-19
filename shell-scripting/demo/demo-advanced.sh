#!/bin/bash

# demo-advanced.sh - Advanced Workflows Demo
# Demonstrates integrated workflows combining multiple modules

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PARENT_DIR="$(dirname "$SCRIPT_DIR")"

# Source common functions
source "${SCRIPT_DIR}/common.sh"

# Setup demo environment
DEMO_DIR=$(setup_demo_dir)
trap "cleanup_demo_dir '$DEMO_DIR'" EXIT

print_header "Advanced Workflows Demo"

#=============================================================================
# Workflow 1: Complete Backup Workflow
#=============================================================================

print_step "Workflow 1: Complete Backup Process"
echo "Creating test files..."
for i in {1..3}; do
    echo "Test data $i" > "$DEMO_DIR/source/test$i.txt"
done

echo "→ Creating backup..."
"${PARENT_DIR}/file-management/backup.sh" "$DEMO_DIR/source" "$DEMO_DIR/backup" 7

echo "→ Verifying backup..."
backup_file=$(ls -t "$DEMO_DIR/backup"/backup_*.tar.gz | head -1)
tar -tzf "$backup_file" | head -5

print_success "Complete backup workflow finished"
pause

#=============================================================================
# Workflow 2: System Maintenance
#=============================================================================

print_step "Workflow 2: System Maintenance Check"
echo "→ Checking disk space..."
sudo "${PARENT_DIR}/task-scheduler/scheduled_tasks.sh" check_disk 2>/dev/null || \
    "${PARENT_DIR}/task-scheduler/scheduled_tasks.sh" check_disk

echo "→ Checking system uptime..."
"${PARENT_DIR}/time-management/time_tracker.sh" uptime

print_success "System maintenance check completed"
pause

#=============================================================================
# Workflow 3: Time Tracking
#=============================================================================

print_step "Workflow 3: Time Tracking Session"
echo "→ Starting stopwatch..."
"${PARENT_DIR}/time-management/stopwatch.sh" start "workflow-task"
sleep 3
echo "→ Recording lap time..."
"${PARENT_DIR}/time-management/stopwatch.sh" lap
sleep 2
echo "→ Stopping stopwatch..."
"${PARENT_DIR}/time-management/stopwatch.sh" stop

print_success "Time tracking workflow completed"
pause

#=============================================================================
# Workflow 4: Package Information
#=============================================================================

print_step "Workflow 4: Package Research"
echo "→ Searching for package..."
"${PARENT_DIR}/package-management/package_manager.sh" search curl | head -5

echo "→ Checking dependencies..."
"${PARENT_DIR}/package-management/dependency_checker.sh" check bash 2>/dev/null || true

print_success "Package research workflow completed"
pause

#=============================================================================
# Summary
#=============================================================================

print_header "Advanced Workflows Demo - Summary"
echo "✓ Complete backup workflow (create → backup → verify)"
echo "✓ System maintenance checks (disk + uptime)"
echo "✓ Time tracking session (start → lap → stop)"
echo "✓ Package research (search → dependencies)"
echo ""
print_success "All advanced workflows completed!"
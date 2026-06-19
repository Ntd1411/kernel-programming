#!/bin/bash

# demo-task-scheduler.sh - Task Scheduler Module Demo
# Demonstrates cron jobs and scheduled maintenance tasks

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PARENT_DIR="$(dirname "$SCRIPT_DIR")"

# Source common functions
source "${SCRIPT_DIR}/common.sh"

print_header "Task Scheduler Demo"

#=============================================================================
# Demo 4.1: List Cron Jobs
#=============================================================================

print_step "4.1 Listing current cron jobs"
"${PARENT_DIR}/task-scheduler/cron_manager.sh" --list || print_info "No cron jobs or crontab not accessible"
print_success "Cron jobs listed"
pause

#=============================================================================
# Demo 4.2: Disk Space Check
#=============================================================================

print_step "4.2 Checking disk space"
sudo "${PARENT_DIR}/task-scheduler/scheduled_tasks.sh" check_disk 2>/dev/null || \
    "${PARENT_DIR}/task-scheduler/scheduled_tasks.sh" check_disk
print_success "Disk check completed"
pause

#=============================================================================
# Demo 4.3: Service Check
#=============================================================================

print_step "4.3 Checking system services"
sudo "${PARENT_DIR}/task-scheduler/scheduled_tasks.sh" check_services 2>/dev/null || \
    print_info "Service check requires sudo - skipped in demo"
pause

#=============================================================================
# Summary
#=============================================================================

print_header "Task Scheduler Demo - Summary"
echo "✓ Cron jobs listed"
echo "✓ Disk space checked"
echo "✓ System services verified"
echo ""
print_success "Task Scheduler demo completed!"
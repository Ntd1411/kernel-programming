#!/bin/bash

# demo-time-management.sh - Time Management Module Demo
# Demonstrates time tracking, timers, and timezone features

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PARENT_DIR="$(dirname "$SCRIPT_DIR")"

# Source common functions
source "${SCRIPT_DIR}/common.sh"

print_header "Time Management Demo"

#=============================================================================
# Demo 2.1: Current Time & Zones
#=============================================================================

print_step "2.1 Current time and worldwide zones"
"${PARENT_DIR}/time-management/time_tracker.sh" now
echo ""
"${PARENT_DIR}/time-management/time_tracker.sh" zones
print_success "Time info displayed"
pause

#=============================================================================
# Demo 2.2: System Uptime
#=============================================================================

print_step "2.2 System uptime information"
"${PARENT_DIR}/time-management/time_tracker.sh" uptime
print_success "Uptime displayed"
pause

#=============================================================================
# Demo 2.3: Calendar
#=============================================================================

print_step "2.3 Calendar display"
"${PARENT_DIR}/time-management/time_tracker.sh" calendar 6 2026 || print_info "cal command not available"
print_success "Calendar displayed"
pause

#=============================================================================
# Demo 2.4: Duration Calculation
#=============================================================================

print_step "2.4 Duration calculation"
"${PARENT_DIR}/time-management/time_tracker.sh" duration "2026-12-31 23:59:59"
print_success "Duration calculated"
pause

#=============================================================================
# Demo 2.5: Timezone Conversion
#=============================================================================

print_step "2.5 Timezone conversion"
"${PARENT_DIR}/time-management/time_tracker.sh" convert "America/New_York" "Asia/Ho_Chi_Minh"
print_success "Timezone conversion completed"
pause

#=============================================================================
# Demo 2.6: Time Addition
#=============================================================================

print_step "2.6 Time addition operations"
"${PARENT_DIR}/time-management/time_tracker.sh" add now 7 days
"${PARENT_DIR}/time-management/time_tracker.sh" add now 3 hours
print_success "Time addition completed"
pause

#=============================================================================
# Demo 2.7: Stopwatch
#=============================================================================

print_step "2.7 Stopwatch demo (5 seconds)"
print_info "Starting stopwatch..."
"${PARENT_DIR}/time-management/stopwatch.sh" start "demo-task"
sleep 2
"${PARENT_DIR}/time-management/stopwatch.sh" lap
sleep 3
"${PARENT_DIR}/time-management/stopwatch.sh" stop
print_success "Stopwatch demo completed"
pause

#=============================================================================
# Demo 2.8: Countdown Timer
#=============================================================================

print_step "2.8 Countdown timer (10 seconds)"
"${PARENT_DIR}/time-management/stopwatch.sh" countdown 10 "Timer finished!" || print_info "Countdown completed"
print_success "Countdown timer completed"
pause

#=============================================================================
# Summary
#=============================================================================

print_header "Time Management Demo - Summary"
echo "✓ Current time and worldwide zones displayed"
echo "✓ System uptime checked"
echo "✓ Calendar displayed"
echo "✓ Duration calculations performed"
echo "✓ Timezone conversions completed"
echo "✓ Time addition operations tested"
echo "✓ Stopwatch with lap timing"
echo "✓ Countdown timer demonstrated"
echo ""
print_success "Time Management demo completed!"
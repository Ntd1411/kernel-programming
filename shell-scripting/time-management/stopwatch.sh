#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG_DIR="${SCRIPT_DIR}/../../logs"
LOG_FILE="${LOG_DIR}/stopwatch.log"
STOPWATCH_FILE="/tmp/stopwatch_${USER}"

mkdir -p "$LOG_DIR"

log_message() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

start_stopwatch() {
    local name="${1:-stopwatch}"
    
    if [ -f "$STOPWATCH_FILE" ]; then
        echo "ERROR: Stopwatch already running"
        echo "Use 'stop' to stop it first"
        return 1
    fi
    
    local start_time=$(date +%s)
    echo "$start_time|$name" > "$STOPWATCH_FILE"
    
    log_message "Stopwatch '$name' started"
    echo "Stopwatch '$name' started at $(date '+%H:%M:%S')"
}

stop_stopwatch() {
    if [ ! -f "$STOPWATCH_FILE" ]; then
        echo "ERROR: No stopwatch running"
        return 1
    fi
    
    local data=$(cat "$STOPWATCH_FILE")
    local start_time=$(echo "$data" | cut -d'|' -f1)
    local name=$(echo "$data" | cut -d'|' -f2)
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    rm "$STOPWATCH_FILE"
    
    local hours=$((duration / 3600))
    local minutes=$(( (duration % 3600) / 60 ))
    local seconds=$((duration % 60))
    
    log_message "Stopwatch '$name' stopped: ${hours}h ${minutes}m ${seconds}s"
    
    echo "Stopwatch '$name' stopped"
    echo "Duration: ${hours}h ${minutes}m ${seconds}s"
    echo "Total seconds: $duration"
}

status_stopwatch() {
    if [ ! -f "$STOPWATCH_FILE" ]; then
        echo "No stopwatch running"
        return 0
    fi
    
    local data=$(cat "$STOPWATCH_FILE")
    local start_time=$(echo "$data" | cut -d'|' -f1)
    local name=$(echo "$data" | cut -d'|' -f2)
    local current_time=$(date +%s)
    local duration=$((current_time - start_time))
    
    local hours=$((duration / 3600))
    local minutes=$(( (duration % 3600) / 60 ))
    local seconds=$((duration % 60))
    
    echo "Stopwatch '$name' is running"
    echo "Started at: $(date -d "@$start_time" '+%Y-%m-%d %H:%M:%S')"
    echo "Elapsed time: ${hours}h ${minutes}m ${seconds}s"
}

lap_stopwatch() {
    if [ ! -f "$STOPWATCH_FILE" ]; then
        echo "ERROR: No stopwatch running"
        return 1
    fi
    
    local data=$(cat "$STOPWATCH_FILE")
    local start_time=$(echo "$data" | cut -d'|' -f1)
    local name=$(echo "$data" | cut -d'|' -f2)
    local current_time=$(date +%s)
    local duration=$((current_time - start_time))
    
    local hours=$((duration / 3600))
    local minutes=$(( (duration % 3600) / 60 ))
    local seconds=$((duration % 60))
    
    local lap_msg="Lap time: ${hours}h ${minutes}m ${seconds}s"
    echo "$lap_msg"
    log_message "$lap_msg"
}

countdown_timer() {
    local total_seconds="$1"
    local message="${2:-Timer finished}"
    
    echo "Starting countdown: $total_seconds seconds"
    
    local end_time=$(($(date +%s) + total_seconds))
    
    while [ $(date +%s) -lt $end_time ]; do
        local remaining=$((end_time - $(date +%s)))
        local hours=$((remaining / 3600))
        local minutes=$(( (remaining % 3600) / 60 ))
        local seconds=$((remaining % 60))
        
        printf "\rTime remaining: %02d:%02d:%02d" $hours $minutes $seconds
        sleep 1
    done
    
    echo ""
    echo "$message"
    
    if command -v notify-send &> /dev/null; then
        notify-send "Timer" "$message"
    fi
    
    if command -v paplay &> /dev/null && [ -f /usr/share/sounds/freedesktop/stereo/complete.oga ]; then
        paplay /usr/share/sounds/freedesktop/stereo/complete.oga
    fi
    
    log_message "Countdown timer completed: $total_seconds seconds"
}

pomodoro_timer() {
    local work_minutes="${1:-25}"
    local break_minutes="${2:-5}"
    local cycles="${3:-4}"
    
    echo "Starting Pomodoro Timer"
    echo "Work: ${work_minutes}m, Break: ${break_minutes}m, Cycles: $cycles"
    echo ""
    
    for ((i=1; i<=cycles; i++)); do
        echo "=== Cycle $i/$cycles ==="
        echo "Work session starting..."
        countdown_timer $((work_minutes * 60)) "Work session $i complete! Time for a break."
        
        if [ $i -lt $cycles ]; then
            echo "Break session starting..."
            countdown_timer $((break_minutes * 60)) "Break complete! Ready for next work session."
            echo ""
        fi
    done
    
    echo "All Pomodoro cycles completed!"
    log_message "Pomodoro session completed: $cycles cycles"
}

alarm_timer() {
    local target_time="$1"
    local message="${2:-Alarm}"
    
    local target_ts=$(date -d "$target_time" +%s)
    local current_ts=$(date +%s)
    
    if [ $target_ts -le $current_ts ]; then
        echo "ERROR: Target time must be in the future"
        return 1
    fi
    
    local wait_seconds=$((target_ts - current_ts))
    
    echo "Alarm set for: $(date -d "$target_time" '+%Y-%m-%d %H:%M:%S')"
    echo "Waiting $wait_seconds seconds..."
    
    sleep "$wait_seconds"
    
    echo "$message"
    
    if command -v notify-send &> /dev/null; then
        notify-send "Alarm" "$message"
    fi
    
    log_message "Alarm triggered: $message"
}

show_usage() {
    cat << EOF
Usage: $0 [OPTION] [ARGUMENTS]

Stopwatch operations:
    start [NAME]        Start a stopwatch
    stop                Stop the running stopwatch
    status              Show stopwatch status
    lap                 Record a lap time

Timer operations:
    countdown SECONDS [MESSAGE]     Countdown timer
    pomodoro [WORK] [BREAK] [CYCLES]  Pomodoro timer
    alarm TIME [MESSAGE]            Set an alarm

Examples:
    # Stopwatch
    $0 start "Task 1"
    $0 status
    $0 lap
    $0 stop

    # Countdown
    $0 countdown 300 "5 minutes done"
    $0 countdown 3600 "1 hour done"

    # Pomodoro (default: 25m work, 5m break, 4 cycles)
    $0 pomodoro
    $0 pomodoro 30 10 3

    # Alarm
    $0 alarm "14:30" "Meeting time"
    $0 alarm "tomorrow 09:00" "Morning reminder"
EOF
}

main() {
    if [ $# -eq 0 ]; then
        show_usage
        exit 1
    fi
    
    case "$1" in
        start)
            start_stopwatch "${2:-stopwatch}"
            ;;
        stop)
            stop_stopwatch
            ;;
        status)
            status_stopwatch
            ;;
        lap)
            lap_stopwatch
            ;;
        countdown)
            if [ $# -lt 2 ]; then
                echo "ERROR: SECONDS required"
                exit 1
            fi
            countdown_timer "$2" "${3:-Timer finished}"
            ;;
        pomodoro)
            pomodoro_timer "${2:-25}" "${3:-5}" "${4:-4}"
            ;;
        alarm)
            if [ $# -lt 2 ]; then
                echo "ERROR: TIME required"
                exit 1
            fi
            alarm_timer "$2" "${3:-Alarm}"
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

#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG_DIR="${SCRIPT_DIR}/../../logs"
LOG_FILE="${LOG_DIR}/time_tracker.log"

mkdir -p "$LOG_DIR"

log_message() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

show_current_time() {
    echo "=== Current Time Information ==="
    echo "Local time: $(date '+%Y-%m-%d %H:%M:%S %Z')"
    echo "UTC time: $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
    echo "Unix timestamp: $(date +%s)"
    echo "Week number: $(date +%U)"
    echo "Day of year: $(date +%j)"
    echo "Timezone: $(date +%Z)"
}

show_uptime() {
    echo "=== System Uptime ==="
    uptime -p
    echo ""
    echo "Detailed uptime:"
    uptime
    
    echo ""
    echo "Boot time:"
    who -b
    
    echo ""
    echo "Current users:"
    who
}

show_time_zones() {
    echo "=== World Time Zones ==="
    
    declare -A cities=(
        ["America/New_York"]="New York"
        ["America/Los_Angeles"]="Los Angeles"
        ["America/Chicago"]="Chicago"
        ["Europe/London"]="London"
        ["Europe/Paris"]="Paris"
        ["Europe/Berlin"]="Berlin"
        ["Asia/Tokyo"]="Tokyo"
        ["Asia/Shanghai"]="Shanghai"
        ["Asia/Ho_Chi_Minh"]="Ho Chi Minh"
        ["Australia/Sydney"]="Sydney"
    )
    
    for tz in "${!cities[@]}"; do
        time=$(TZ="$tz" date '+%H:%M %Z')
        printf "%-20s: %s\n" "${cities[$tz]}" "$time"
    done
}

convert_timezone() {
    local from_tz="$1"
    local to_tz="$2"
    local time_str="${3:-now}"
    
    if [ "$time_str" = "now" ]; then
        echo "Converting current time:"
        echo "From $from_tz: $(TZ="$from_tz" date '+%Y-%m-%d %H:%M:%S %Z')"
        echo "To $to_tz: $(TZ="$to_tz" date '+%Y-%m-%d %H:%M:%S %Z')"
    else
        echo "Converting $time_str:"
        echo "From $from_tz: $(TZ="$from_tz" date -d "$time_str" '+%Y-%m-%d %H:%M:%S %Z')"
        echo "To $to_tz: $(TZ="$to_tz" date -d "$time_str" '+%Y-%m-%d %H:%M:%S %Z')"
    fi
}

calculate_duration() {
    local start_time="$1"
    local end_time="${2:-now}"
    
    local start_ts
    local end_ts
    
    if [ "$start_time" = "now" ]; then
        start_ts=$(date +%s)
    else
        start_ts=$(date -d "$start_time" +%s)
    fi
    
    if [ "$end_time" = "now" ]; then
        end_ts=$(date +%s)
    else
        end_ts=$(date -d "$end_time" +%s)
    fi
    
    local duration=$((end_ts - start_ts))
    
    local days=$((duration / 86400))
    local hours=$(( (duration % 86400) / 3600 ))
    local minutes=$(( (duration % 3600) / 60 ))
    local seconds=$((duration % 60))
    
    echo "Duration: ${days}d ${hours}h ${minutes}m ${seconds}s"
    echo "Total seconds: $duration"
    echo "Total minutes: $((duration / 60))"
    echo "Total hours: $((duration / 3600))"
}

format_timestamp() {
    local timestamp="$1"
    local format="${2:-%Y-%m-%d %H:%M:%S}"
    
    if [[ "$timestamp" =~ ^[0-9]+$ ]]; then
        date -d "@$timestamp" +"$format"
    else
        date -d "$timestamp" +"$format"
    fi
}

show_calendar() {
    local month="${1:-}"
    local year="${2:-}"
    
    if [ -n "$month" ] && [ -n "$year" ]; then
        cal "$month" "$year"
    elif [ -n "$month" ]; then
        cal "$month"
    else
        cal
        echo ""
        echo "Current year:"
        cal -y
    fi
}

add_time() {
    local base_time="${1:-now}"
    local amount="$2"
    local unit="$3"
    
    local result
    case "$unit" in
        seconds|second|s)
            result=$(date -d "$base_time + $amount seconds" '+%Y-%m-%d %H:%M:%S')
            ;;
        minutes|minute|m)
            result=$(date -d "$base_time + $amount minutes" '+%Y-%m-%d %H:%M:%S')
            ;;
        hours|hour|h)
            result=$(date -d "$base_time + $amount hours" '+%Y-%m-%d %H:%M:%S')
            ;;
        days|day|d)
            result=$(date -d "$base_time + $amount days" '+%Y-%m-%d %H:%M:%S')
            ;;
        weeks|week|w)
            result=$(date -d "$base_time + $amount weeks" '+%Y-%m-%d %H:%M:%S')
            ;;
        months|month|M)
            result=$(date -d "$base_time + $amount months" '+%Y-%m-%d %H:%M:%S')
            ;;
        years|year|y)
            result=$(date -d "$base_time + $amount years" '+%Y-%m-%d %H:%M:%S')
            ;;
        *)
            echo "ERROR: Invalid unit. Use: seconds, minutes, hours, days, weeks, months, years"
            return 1
            ;;
    esac
    
    echo "Base time: $(date -d "$base_time" '+%Y-%m-%d %H:%M:%S')"
    echo "Add: $amount $unit"
    echo "Result: $result"
}

compare_times() {
    local time1="$1"
    local time2="$2"
    
    local ts1=$(date -d "$time1" +%s)
    local ts2=$(date -d "$time2" +%s)
    
    echo "Time 1: $(date -d "$time1" '+%Y-%m-%d %H:%M:%S')"
    echo "Time 2: $(date -d "$time2" '+%Y-%m-%d %H:%M:%S')"
    echo ""
    
    if [ "$ts1" -eq "$ts2" ]; then
        echo "Times are equal"
    elif [ "$ts1" -lt "$ts2" ]; then
        echo "Time 1 is earlier than Time 2"
        local diff=$((ts2 - ts1))
        echo "Difference: $diff seconds"
    else
        echo "Time 1 is later than Time 2"
        local diff=$((ts1 - ts2))
        echo "Difference: $diff seconds"
    fi
}

show_usage() {
    cat << EOF
Usage: $0 [OPTION] [ARGUMENTS]

Time management operations:
    now                 Show current time information
    uptime              Show system uptime
    zones               Show world time zones
    convert FROM TO [TIME]  Convert between time zones
    duration START [END]    Calculate duration between times
    format TIMESTAMP [FORMAT]  Format timestamp
    calendar [MONTH] [YEAR]    Show calendar
    add TIME AMOUNT UNIT       Add time to a timestamp
    compare TIME1 TIME2        Compare two timestamps
    help                Show this help message

Examples:
    $0 now
    $0 uptime
    $0 zones
    $0 convert "America/New_York" "Asia/Tokyo"
    $0 duration "2026-01-01 00:00:00"
    $0 format 1704067200
    $0 calendar 6 2026
    $0 add now 5 hours
    $0 compare "2026-01-01" "2026-12-31"

Time formats:
    - now
    - YYYY-MM-DD
    - YYYY-MM-DD HH:MM:SS
    - Unix timestamp
    - Relative: "1 hour ago", "tomorrow", "next week"
EOF
}

main() {
    if [ $# -eq 0 ]; then
        show_usage
        exit 1
    fi
    
    case "$1" in
        now)
            show_current_time
            ;;
        uptime)
            show_uptime
            ;;
        zones)
            show_time_zones
            ;;
        convert)
            if [ $# -lt 3 ]; then
                echo "ERROR: FROM and TO timezone required"
                exit 1
            fi
            convert_timezone "$2" "$3" "${4:-now}"
            ;;
        duration)
            if [ $# -lt 2 ]; then
                echo "ERROR: START time required"
                exit 1
            fi
            calculate_duration "$2" "${3:-now}"
            ;;
        format)
            if [ $# -lt 2 ]; then
                echo "ERROR: TIMESTAMP required"
                exit 1
            fi
            format_timestamp "$2" "${3:-%Y-%m-%d %H:%M:%S}"
            ;;
        calendar)
            show_calendar "${2:-}" "${3:-}"
            ;;
        add)
            if [ $# -lt 4 ]; then
                echo "ERROR: TIME, AMOUNT, and UNIT required"
                exit 1
            fi
            add_time "$2" "$3" "$4"
            ;;
        compare)
            if [ $# -lt 3 ]; then
                echo "ERROR: Two timestamps required"
                exit 1
            fi
            compare_times "$2" "$3"
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

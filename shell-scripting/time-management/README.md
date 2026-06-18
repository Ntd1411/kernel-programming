# Time Management Scripts

Công cụ quản lý và theo dõi thời gian.

## Scripts

### 1. time_tracker.sh
**Time utilities and timezone operations**

```bash
./time_tracker.sh [operation]
```

**Operations:**

#### now
Display current time in multiple formats
```bash
./time_tracker.sh now
```
Output: Local time, UTC, Unix timestamp

#### uptime
Show system uptime
```bash
./time_tracker.sh uptime
```

#### zones
Display world timezones
```bash
./time_tracker.sh zones
```

#### convert
Convert time between timezones
```bash
./time_tracker.sh convert "America/New_York" "Asia/Ho_Chi_Minh"
```

#### duration
Calculate duration to a specific date
```bash
./time_tracker.sh duration "2026-12-31 23:59:59"
```

#### format
Format a date/time string
```bash
./time_tracker.sh format "2026-06-16 14:30:00" "%A, %B %d, %Y"
```

#### calendar
Display calendar for month/year
```bash
./time_tracker.sh calendar 6 2026
```

#### add
Add time to a date
```bash
./time_tracker.sh add now 5 hours
./time_tracker.sh add now 3 days
./time_tracker.sh add "2026-06-16 10:00:00" 2 weeks
```

#### compare
Compare two dates
```bash
./time_tracker.sh compare "2026-06-01" "2026-06-30"
```

---

### 2. stopwatch.sh
**Stopwatch, countdown, and timer utilities**

```bash
./stopwatch.sh [command] [arguments]
```

**Commands:**

#### start
Start stopwatch
```bash
./stopwatch.sh start "task-name"
```

#### lap
Record a lap time
```bash
./stopwatch.sh lap
```

#### status
Check stopwatch status
```bash
./stopwatch.sh status
```

#### stop
Stop stopwatch and show results
```bash
./stopwatch.sh stop
```

#### countdown
Countdown timer with notification
```bash
./stopwatch.sh countdown 60 "Time's up!"
```

#### pomodoro
Pomodoro timer
```bash
./stopwatch.sh pomodoro [work_minutes] [break_minutes] [cycles]
# Default: 25 minutes work, 5 minutes break, 4 cycles
./stopwatch.sh pomodoro 25 5 4
```

#### alarm
Set an alarm for specific time
```bash
./stopwatch.sh alarm "14:30" "Meeting time"
```

---

## Quick Examples

### Track Work Time
```bash
# Start tracking
./stopwatch.sh start "project-work"

# Record milestones
./stopwatch.sh lap  # After task 1
./stopwatch.sh lap  # After task 2

# Stop and see total time
./stopwatch.sh stop
```

### Timezone Conversion
```bash
# Check current time in multiple zones
./time_tracker.sh zones

# Convert specific timezone
./time_tracker.sh convert "America/Los_Angeles" "Europe/London"
```

### Quick Timer
```bash
# 10 minute timer
./stopwatch.sh countdown 600 "Break is over"
```

### Pomodoro Session
```bash
# Standard pomodoro: 25m work, 5m break, 4 cycles
./stopwatch.sh pomodoro 25 5 4

# Short session: 15m work, 3m break, 3 cycles
./stopwatch.sh pomodoro 15 3 3
```

### Calculate Duration
```bash
# Days until specific date
./time_tracker.sh duration "2026-12-31 23:59:59"

# Add time to current date
./time_tracker.sh add now 7 days
```

---

## Use Cases

### For Developers
- Track coding sessions with stopwatch
- Use pomodoro for focused work
- Convert timezone for remote team meetings
- Calculate project deadlines

### For Students
- Time study sessions
- Break reminders with countdown
- Track assignment progress
- Calculate time to exams

### For Productivity
- Pomodoro technique implementation
- Task time tracking
- Break management
- Time zone coordination

---

## Features

✅ Multiple timezone support  
✅ Unix timestamp conversion  
✅ Duration calculations  
✅ Countdown timers  
✅ Pomodoro technique  
✅ Lap time recording  
✅ Alarm notifications  
✅ Calendar display  

---

## Dependencies

**Required:**
- `date` - Date manipulation
- `cal` - Calendar display

**Optional:**
- `notify-send` - Desktop notifications
- `paplay` - Sound notifications

---

## More Info

See main [README.md](../README.md) for detailed documentation.

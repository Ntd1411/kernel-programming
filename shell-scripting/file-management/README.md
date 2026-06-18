# File Management Scripts

Các script quản lý file và thư mục tự động.

## Scripts

### 1. file_manager.sh
**Interactive file management menu**

```bash
./file_manager.sh
```

**Features:**
- Search files by pattern
- Copy/move files
- Delete files/directories
- Compress/extract archives
- Change permissions
- Show file information
- Find largest files

---

### 2. backup.sh
**Automated backup with retention policy**

```bash
./backup.sh <source_dir> <backup_dir> [retention_days]
```

**Example:**
```bash
./backup.sh /home/user /backup 7
```

**Features:**
- Creates timestamped tar.gz archives
- Automatic cleanup of old backups
- Detailed logging
- Backup verification

---

### 3. find_duplicates.sh
**Find and handle duplicate files**

```bash
./find_duplicates.sh <directory> [action]
```

**Actions:** `list`, `delete`, `move`

**Example:**
```bash
./find_duplicates.sh /home/user/Documents list
./find_duplicates.sh /home/user/Downloads delete
```

**Features:**
- MD5 hash-based detection
- Safe duplicate handling
- Keeps one copy when deleting
- Moves duplicates to separate folder

---

### 4. cleanup.sh
**Clean temporary files and logs**

```bash
./cleanup.sh [options]
```

**Options:**
- `-d, --days DAYS` - Delete files older than DAYS (default: 30)
- `-n, --dry-run` - Show what would be deleted without deleting
- `-v, --verbose` - Verbose output

**Example:**
```bash
./cleanup.sh -d 7 -n -v
```

**Features:**
- Cleans temp directories
- Removes old log files
- Clears browser caches
- Dry-run mode for safety

---

## Quick Start

```bash
# Make scripts executable
chmod +x *.sh

# Interactive file manager
./file_manager.sh

# Backup your data
./backup.sh /source /backup 7

# Find duplicates
./find_duplicates.sh /path list

# Cleanup dry-run
./cleanup.sh -d 30 -n
```

## More Info

See main [README.md](../README.md) for detailed documentation.

# Package Management Scripts

Công cụ quản lý packages, dependencies và repositories trên nhiều distro Linux.

## Scripts

### 1. package_manager.sh
**Cross-distribution package manager**

```bash
./package_manager.sh [command] [package_name]
```

**Commands:**

#### install
Install a package
```bash
sudo ./package_manager.sh install vim
sudo ./package_manager.sh install python3 git curl
```

#### remove
Remove a package
```bash
sudo ./package_manager.sh remove vim
```

#### search
Search for packages
```bash
./package_manager.sh search python
./package_manager.sh search "text editor"
```

#### update
Update package list
```bash
sudo ./package_manager.sh update
```

#### upgrade
Upgrade all packages
```bash
sudo ./package_manager.sh upgrade
```

#### list
List installed packages
```bash
./package_manager.sh list
./package_manager.sh list | grep python
```

**Supported Package Managers:**
- APT (Ubuntu/Debian)
- DNF (Fedora/RHEL 8+)
- YUM (CentOS/RHEL 7)
- Pacman (Arch Linux)
- Zypper (openSUSE)

---

### 2. dependency_checker.sh
**Package dependency analysis**

```bash
./dependency_checker.sh [command] [package_name]
```

**Commands:**

#### check
Check dependencies for a package
```bash
./dependency_checker.sh check vim
./dependency_checker.sh check python3
```
Shows all required dependencies.

#### broken
Find broken dependencies
```bash
./dependency_checker.sh broken
```
Lists packages with missing or broken dependencies.

#### fix
Fix broken dependencies
```bash
sudo ./dependency_checker.sh fix
```
Attempts to repair broken packages.

#### orphans
List orphaned packages
```bash
./dependency_checker.sh orphans
```
Shows packages no longer needed by any other package.

#### clean
Remove orphaned packages
```bash
sudo ./dependency_checker.sh clean
```
Removes unnecessary packages to free space.

#### info
Show detailed package information
```bash
./dependency_checker.sh info vim
```
Displays version, size, dependencies, description.

---

### 3. repo_manager.sh
**Repository management**

```bash
./repo_manager.sh [command] [arguments]
```

**Commands:**

#### list
List all repositories
```bash
./repo_manager.sh list
```

#### add
Add a new repository
```bash
# Ubuntu/Debian PPA
sudo ./repo_manager.sh add ppa:user/repo

# Manual repository
sudo ./repo_manager.sh add "deb http://example.com/ubuntu focal main"
```

#### remove
Remove a repository
```bash
sudo ./repo_manager.sh remove ppa:user/repo
```

#### update
Update repository cache
```bash
sudo ./repo_manager.sh update
```

#### clean
Clean repository cache
```bash
sudo ./repo_manager.sh clean
```

#### info
Show repository information
```bash
./repo_manager.sh info
```

---

## Quick Start

### Install Packages
```bash
# Search for a package
./package_manager.sh search vim

# Install it
sudo ./package_manager.sh install vim

# Verify installation
./package_manager.sh list | grep vim
```

### Check Dependencies
```bash
# Check what vim needs
./dependency_checker.sh check vim

# Find broken packages
./dependency_checker.sh broken

# Fix them
sudo ./dependency_checker.sh fix
```

### Manage Repositories
```bash
# List current repos
./repo_manager.sh list

# Add new repo
sudo ./repo_manager.sh add ppa:deadsnakes/ppa

# Update cache
sudo ./repo_manager.sh update
```

### System Maintenance
```bash
# Update package list
sudo ./package_manager.sh update

# Upgrade all packages
sudo ./package_manager.sh upgrade

# Clean orphaned packages
sudo ./dependency_checker.sh clean

# Clean repository cache
sudo ./repo_manager.sh clean
```

---

## Use Cases

### For System Administrators
- Automated package installation
- Dependency troubleshooting
- Repository management
- System updates and maintenance

### For Developers
- Install development tools
- Manage project dependencies
- Check library requirements
- Clean up unused packages

### For DevOps
- Scripted deployments
- Consistent package management across distros
- Automated dependency checking
- CI/CD integration

---

## Platform Support

| Platform | package_manager.sh | dependency_checker.sh | repo_manager.sh |
|----------|-------------------|----------------------|-----------------|
| Ubuntu/Debian | ✅ APT | ✅ Full | ✅ Full |
| Fedora | ✅ DNF | ✅ Full | ✅ Full |
| RHEL/CentOS 8+ | ✅ DNF | ✅ Full | ✅ Full |
| RHEL/CentOS 7 | ✅ YUM | ✅ Full | ✅ Full |
| Arch Linux | ✅ Pacman | ✅ Partial | ✅ Partial |
| openSUSE | ✅ Zypper | ✅ Partial | ✅ Partial |

---

## Features

✅ Multi-distro support  
✅ Automatic package manager detection  
✅ Dependency resolution  
✅ Broken package detection  
✅ Repository management  
✅ Orphaned package cleanup  
✅ Detailed logging  
✅ Error handling  

---

## Common Workflows

### Setup New System
```bash
# Update package list
sudo ./package_manager.sh update

# Install essential tools
sudo ./package_manager.sh install vim git curl wget build-essential

# Check for issues
./dependency_checker.sh broken
```

### Before Installing New Software
```bash
# Search for the package
./package_manager.sh search package-name

# Check its dependencies
./dependency_checker.sh info package-name

# Install if satisfied
sudo ./package_manager.sh install package-name
```

### Regular Maintenance
```bash
# Update package list
sudo ./package_manager.sh update

# Check for broken packages
./dependency_checker.sh broken

# Fix if needed
sudo ./dependency_checker.sh fix

# Clean orphans
sudo ./dependency_checker.sh clean

# Upgrade system
sudo ./package_manager.sh upgrade
```

### Add Third-Party Software
```bash
# Add repository
sudo ./repo_manager.sh add ppa:graphics-drivers/ppa

# Update package list
sudo ./repo_manager.sh update

# Install from new repo
sudo ./package_manager.sh install nvidia-driver-535
```

---

## Dependencies

**Required:**
- One of: `apt`, `dnf`, `yum`, `pacman`, or `zypper`
- `grep`, `awk`, `sed` (standard utilities)

**Optional:**
- `add-apt-repository` (Ubuntu/Debian PPA support)

---

## Logging

All operations are logged to:
```
../logs/package_manager.log
../logs/dependency_checker.log
../logs/repo_manager.log
```

View logs:
```bash
tail -f ../logs/package_manager.log
```

---

## Troubleshooting

### "Package manager not found"
Install your distribution's package manager or check PATH.

### "Permission denied"
Use `sudo` for install/remove/update operations.

### "Repository not found"
Verify repository URL and format for your distribution.

### "Broken packages"
Run:
```bash
sudo ./dependency_checker.sh fix
```

---

## Security Notes

- Scripts detect package manager automatically
- Always verify packages before installation
- Review repository sources before adding
- Logs may contain system information
- Sudo required for system modifications

---

## More Info

See main [README.md](../README.md) for detailed documentation.

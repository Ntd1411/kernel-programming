# Documentation Index - Kernel Programming Project

## Quick Navigation

### 🎯 Start Here
1. **[README.md](./README.md)** - Project overview & structure
2. **[DOCUMENTATION_PLAN.md](./DOCUMENTATION_PLAN.md)** - Improvement roadmap

### 📚 Main Sections

#### Part 1: Shell Scripting (shell-scripting/)
- **[shell-scripting/README.md](./shell-scripting/README.md)** - Overview & usage
- **[shell-scripting/GUI_README.md](./shell-scripting/GUI_README.md)** - GUI detailed guide
- **[shell-scripting/GUI_SUMMARY.md](./shell-scripting/GUI_SUMMARY.md)** - GUI technical summary
- **[shell-scripting/QUICKSTART.sh](./shell-scripting/QUICKSTART.sh)** - Quick start guide

**Sub-modules:**
- [file-management/README.md](./shell-scripting/file-management/README.md)
- [task-scheduler/README.md](./shell-scripting/task-scheduler/README.md)
- [time-management/README.md](./shell-scripting/time-management/README.md)
- [package-management/README.md](./shell-scripting/package-management/README.md)

**Key Files:**
- `demo.sh` - Interactive demo menu
- `quick_test.sh` - Quick functionality test
- `gui_launcher.py` - GUI launcher (new!)

#### Part 2: SMP Programming (smp-programming/)
- **[smp-programming/README.md](./smp-programming/README.md)** - SMP overview
- 12 examples from race conditions to RCU

**Lecture Resources:**
- [smp-programming/doc/smp-lecture.html](./smp-programming/doc/smp-lecture.html)
- [smp-programming/doc/smp-audit.html](./smp-programming/doc/smp-audit.html)

#### Part 3: System Programming (system/)
- **[system/README.md](./system/README.md)** - System programming overview (NEW!)
- **[system/file/README.md](./system/file/README.md)** - File operations
- **[system/process/README.md](./system/process/README.md)** - Process management
- **[system/network/README.md](./system/network/README.md)** - Network programming
- **[system/socket/README.md](./system/socket/README.md)** - Socket programming

## Documentation Standards

### File Organization
```
project/
├── README.md              # Main entry point
├── DOCUMENTATION_PLAN.md  # This plan
├── CONTRIBUTING.md        # (to be created)
├── INDEX.md              # This file
│
├── shell-scripting/
│   ├── README.md         # Module overview
│   ├── GUI_README.md     # GUI guide
│   ├── GUI_SUMMARY.md    # Technical summary
│   └── [sub-modules]/README.md
│
├── smp-programming/
│   ├── README.md         # Module overview
│   ├── [examples]/README.md
│   └── doc/
│
└── system/
    ├── README.md         # Module overview
    └── [modules]/README.md
```

### README Format
Each README should include:
1. **Title & Overview** - What it is
2. **Structure/Contents** - What's inside
3. **Quick Start** - How to run/build
4. **Key Concepts** - What you'll learn
5. **Examples/Usage** - How to use
6. **References** - Man pages, links

## Documentation Status

### ✅ Completed
- [x] Root README.md (updated)
- [x] system/README.md (created)
- [x] shell-scripting/README.md (enhanced with GUI)
- [x] shell-scripting/GUI_README.md (created)
- [x] shell-scripting/GUI_SUMMARY.md (created)
- [x] smp-programming/README.md (existing - good)
- [x] DOCUMENTATION_PLAN.md (created)
- [x] This INDEX.md

### ⚠️ In Progress / To Review
- [ ] system/file/README.md (needs minor polish)
- [ ] system/process/README.md (needs minor polish)
- [ ] system/network/README.md (needs minor polish)
- [ ] system/socket/README.md (needs review)

### 📋 To Create
- [ ] CONTRIBUTING.md - Contribution guidelines
- [ ] ARCHITECTURE.md - Project architecture
- [ ] TROUBLESHOOTING.md - Common issues

## Quick Links

### For Beginners
1. Start with: [README.md](./README.md)
2. Try shell scripting: [shell-scripting/README.md](./shell-scripting/README.md)
3. Try GUI: [shell-scripting/GUI_README.md](./shell-scripting/GUI_README.md)
4. Run demo: `./shell-scripting/demo.sh`

### For Advanced Users
1. Read: [smp-programming/README.md](./smp-programming/README.md)
2. Study: [smp-programming/doc/smp-lecture.html](./smp-programming/doc/smp-lecture.html)
3. Explore: System programming examples
4. Hands-on: Compile and run examples

### For Developers
1. Review: [DOCUMENTATION_PLAN.md](./DOCUMENTATION_PLAN.md)
2. Check: Contributing guidelines (to come)
3. Follow: Coding standards
4. Submit: PRs with documentation

## Key Features

### Shell Scripting GUI (NEW!)
- Python/tkinter based
- Real-time output with colors
- Interactive input support
- Ubuntu 24.10 compatible

### SMP Examples
- 12 synchronization primitives
- User-space simulations
- Based on kernel lecture
- Educational focus

### System Programming
- C examples
- File, process, network, socket APIs
- Real system calls
- Practical demonstrations

## Getting Started

### Ubuntu 24.10 Setup
```bash
# Clone repository
git clone <repo-url>
cd kernel-programming

# Setup shell scripting
cd shell-scripting
./setup_gui.sh

# Try GUI
./launch_gui.sh

# Or try SMP
cd ../smp-programming
make
./demo.sh
```

## Contributing

Documentation improvements are welcome!

Areas needing work:
- [ ] Expand system programming examples
- [ ] Add more real-world use cases
- [ ] Create video tutorials (future)
- [ ] Translate to English (future)

See [CONTRIBUTING.md](./CONTRIBUTING.md) (coming soon) for guidelines.

## License

Educational purpose only.

---

**Last Updated**: 2026-06-22  
**Documentation Version**: 1.0  
**Status**: DRAFT - Improvements in progress

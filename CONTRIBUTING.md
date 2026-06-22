# Contributing Guide

Cảm ơn bạn quan tâm đến dự án Kernel Programming! Hướng dẫn này giúp bạn đóng góp hiệu quả.

## Quy Tắc Chung

### Code Style
- **Bash scripts**: Lowercase filenames, use `#!/bin/bash`, set -e
- **C code**: Follow Linux kernel coding style, use meaningful variable names
- **Python**: PEP 8 compliant, add docstrings

### Documentation
- Write in **Tiếng Việt** (Vietnamese)
- Each file/script should have clear comments
- READMEs: Overview, Structure, Usage, Concepts
- Use markdown formatting

### Commit Messages
```
[type] brief description (50 chars max)

Detailed explanation if needed.
- Bullet points for changes
- Reference issues if applicable
```

Types: `feat`, `fix`, `docs`, `refactor`, `test`, `chore`

## How to Contribute

### 1. Report Issues
- Check existing issues first
- Describe problem clearly
- Include: OS version, error message, steps to reproduce
- Label: bug, enhancement, documentation

### 2. Improve Documentation
- Fix typos/clarity
- Add examples
- Create troubleshooting guides
- Update README sections

**Steps:**
1. Fork repository
2. Edit documentation files
3. Follow documentation standards (see INDEX.md)
4. Submit PR with clear title

### 3. Add Examples
- New script in appropriate subdirectory
- Create README.md with:
  - Concept explanation
  - Usage instructions
  - System calls used
  - Example output
- Test thoroughly
- Submit PR

### 4. Fix Bugs
1. Create issue describing bug
2. Fork & create feature branch
3. Fix the bug
4. Add test case if applicable
5. Update documentation
6. Submit PR

### 5. Code Review Checklist

Before submitting PR:
- [ ] Code follows project style
- [ ] Comments explain non-obvious parts
- [ ] README/documentation updated
- [ ] Scripts have proper shebang
- [ ] Code tested on Ubuntu 24.10
- [ ] No hardcoded paths/credentials
- [ ] Error handling implemented

## Project Structure

```
kernel-programming/
├── shell-scripting/        # Bash scripts
│   ├── file-management/
│   ├── task-scheduler/
│   ├── time-management/
│   ├── package-management/
│   ├── gui_launcher.py     # Python GUI
│   └── README.md
│
├── smp-programming/        # C + user-space demos
│   ├── race-condition-ex01/
│   ├── [more examples]/
│   └── README.md
│
├── system/                 # System programming C
│   ├── file/
│   ├── process/
│   ├── network/
│   ├── socket/
│   └── README.md
│
├── docs/                   # Documentation
├── INDEX.md               # Navigation guide
├── DOCUMENTATION_PLAN.md  # Improvement roadmap
└── README.md             # Main entry point
```

## Common Tasks

### Adding a Shell Script
```bash
# 1. Create in appropriate directory
shell-scripting/[category]/new_script.sh

# 2. Add shebang and set -e
#!/bin/bash
set -e

# 3. Add comments
# Function description
my_function() {
    # Implementation
}

# 4. Test thoroughly
chmod +x new_script.sh
./new_script.sh --help

# 5. Update category README
shell-scripting/[category]/README.md

# 6. Commit with good message
git add ...
git commit -m "[feat] Add new_script description"
```

### Adding a C Program
```bash
# 1. Create file in appropriate directory
system/[module]/program.c

# 2. Include proper headers
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

# 3. Add comments explaining concepts
/* Demonstrates: concept_name */

# 4. Create Makefile entry
gcc -o program program.c

# 5. Test compilation
make program
./program

# 6. Create/update README
system/[module]/README.md
- What it demonstrates
- Compilation & usage
- Expected output
- Key concepts

# 7. Commit
git commit -m "[feat] Add program for concept_name"
```

### Improving Documentation
1. Identify outdated/unclear sections
2. Read latest documentation standards
3. Update content clearly
4. Test any code examples
5. Commit with [docs] prefix

## Testing

### Shell Scripts
```bash
# Syntax check
bash -n script.sh

# Run with test data
./script.sh --test

# Check for common issues
shellcheck script.sh
```

### C Programs
```bash
# Compile with warnings
gcc -Wall -Wextra program.c

# Run with valgrind for memory leaks
valgrind ./a.out

# Test on Ubuntu 24.10
# (primary target OS)
```

### Documentation
```bash
# Check markdown syntax
# Check links are correct
# Verify code examples work
# Test on actual system
```

## Debugging Tips

### Shell Scripts
```bash
# Enable debug mode
bash -x script.sh

# Or in script:
set -x  # Enable
set +x  # Disable

# Check variables
echo "DEBUG: var=$var"
```

### C Programs
```bash
# Compile with debug info
gcc -g program.c

# Run with debugger
gdb ./a.out

# Check memory
valgrind --leak-check=full ./a.out
```

## Questions?

- Check existing issues/discussions
- Review similar code/documentation
- Ask in PR/issue comments
- Read referenced man pages

## License

By contributing, you agree your work is educational purpose.

---

**Thank you for contributing!** 🎉

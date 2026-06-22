#!/usr/bin/env python3
"""
Ứng dụng GUI quản lý dự án Kernel Linux
Hỗ trợ chạy các demo và script từ 3 phần: Shell, System, SMP

PHIÊN BẢN ĐÃ SỬA: dùng PTY (pseudo-terminal) thật thay vì subprocess.PIPE
để hành vi của các tiến trình con (buffering, màu ANSI, sudo, Ctrl+C, ...)
giống hệt như khi chạy trực tiếp trên terminal thật.
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
import subprocess
import threading
import os
import sys
import signal
import struct
import fcntl
import termios
from pathlib import Path
import queue
import pty
import select

class KernelLinuxGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Kernel Linux")
        self.root.geometry("1400x900")
        # Cho phép kéo viền cửa sổ để resize (mặc định tkinter đã True,
        # nhưng đặt rõ ra để chắc chắn WM không bị nhầm là cố định kích thước)
        self.root.resizable(True, True)
        
        # Trạng thái toàn màn hình / phóng to, dùng cho các nút tự vẽ
        # vì một số window manager (đặc biệt khi chạy qua VNC/X11 forwarding)
        # không hiển thị nút thu nhỏ/phóng to trên thanh tiêu đề hệ thống.
        self.is_fullscreen = False
        self.is_maximized = False
        self._normal_geometry = "1400x900"
        
        # Phím tắt: F11 toàn màn hình, Esc thoát toàn màn hình
        self.root.bind("<F11>", lambda e: self.toggle_fullscreen())
        self.root.bind("<Escape>", lambda e: self.exit_fullscreen())
        
        # Đường dẫn gốc dự án
        self.project_root = Path(__file__).parent.parent
        
        # Queue để giao tiếp với thread terminal
        self.output_queue = queue.Queue()
        
        # Process hiện tại
        self.current_process = None
        self.process_lock = threading.Lock()
        
        # File descriptor của đầu "master" PTY đang dùng cho process hiện tại
        self.master_fd = None
        
        # Define script parameters
        self.script_params = self.define_script_parameters()
        
        # Thiết lập giao diện
        self.setup_ui()
        
        # Bắt đầu cập nhật output
        self.update_output()
    
    def define_script_parameters(self):
        """Định nghĩa parameters cho các shell scripts"""
        return {
            "shell-scripting/file-management/backup.sh": {
                "needs_sudo": True,  # Writes to system directories
                "params": [
                    {"name": "source_dir", "prompt": "Source directory", "example": "/home/user/Documents", "required": True},
                    {"name": "backup_dir", "prompt": "Backup directory", "example": "/backup", "required": True},
                    {"name": "retention_days", "prompt": "Retention days (optional)", "example": "7", "required": False, "default": "7"}
                ]
            },
            "shell-scripting/file-management/find_duplicates.sh": {
                "needs_sudo": False,
                "params": [
                    {"name": "directory", "prompt": "Directory to scan", "example": "/home/user/Documents", "required": True},
                    {"name": "action", "prompt": "Action (list/delete/move, optional)", "example": "list", "required": False, "default": "list"}
                ]
            },
            "shell-scripting/file-management/cleanup.sh": {
                "needs_sudo": False,
                "params": [
                    {"name": "days", "prompt": "Days old (optional, use --days flag)", "example": "30", "required": False, "default": ""},
                    {"name": "dry_run", "prompt": "Dry run? (yes/no, optional)", "example": "no", "required": False, "default": "no"}
                ]
            },
            "shell-scripting/package-management/package_manager.sh": {
                "needs_sudo": True,  # Package installation requires sudo
                "params": [
                    {"name": "command", "prompt": "Command (install/remove/search/update/upgrade/list)", "example": "install", "required": True},
                    {"name": "package_name", "prompt": "Package name (required for install/remove/search)", "example": "vim", "required": False, "default": ""}
                ]
            },
            "shell-scripting/package-management/dependency_checker.sh": {
                "needs_sudo": False,
                "params": [
                    {"name": "package_name", "prompt": "Package name to check", "example": "vim", "required": True}
                ]
            },
            "shell-scripting/time-management/stopwatch.sh": {
                "needs_sudo": False,
                "params": [
                    {"name": "command", "prompt": "Command (start/stop/status)", "example": "start", "required": True},
                    {"name": "name", "prompt": "Stopwatch name (optional for start)", "example": "coding-session", "required": False, "default": "stopwatch"}
                ]
            }
        }
    
    def collect_script_parameters(self, script_path):
        """Thu thập parameters từ người dùng cho script"""
        # Get relative path for lookup
        rel_path = str(script_path.relative_to(self.project_root))
        
        if rel_path not in self.script_params:
            return []
        
        param_defs = self.script_params[rel_path]["params"]
        
        # Create parameter collection dialog
        dialog = tk.Toplevel(self.root)
        dialog.title(f"Parameters - {script_path.name}")
        dialog.geometry("600x400")
        dialog.transient(self.root)
        dialog.grab_set()
        
        # Center dialog
        dialog.update_idletasks()
        x = (dialog.winfo_screenwidth() // 2) - (300)
        y = (dialog.winfo_screenheight() // 2) - (200)
        dialog.geometry(f"+{x}+{y}")
        
        # Title
        title_frame = ttk.Frame(dialog, padding="10")
        title_frame.pack(fill=tk.X)
        
        ttk.Label(
            title_frame,
            text=f"Enter parameters for: {script_path.name}",
            font=("Arial", 12, "bold")
        ).pack()
        
        # Parameters frame with scrollbar
        canvas = tk.Canvas(dialog)
        scrollbar = ttk.Scrollbar(dialog, orient="vertical", command=canvas.yview)
        params_frame = ttk.Frame(canvas)
        
        params_frame.bind(
            "<Configure>",
            lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
        )
        
        canvas.create_window((0, 0), window=params_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)
        
        canvas.pack(side="left", fill="both", expand=True, padx=10)
        scrollbar.pack(side="right", fill="y")
        
        # Create input fields
        entries = []
        for idx, param in enumerate(param_defs):
            # Label
            label_text = f"{param['prompt']}"
            if param['required']:
                label_text += " *"
            else:
                label_text += f" (default: {param.get('default', 'none')})"
            
            label = ttk.Label(params_frame, text=label_text)
            label.grid(row=idx*2, column=0, sticky=tk.W, pady=(5, 0), padx=5)
            
            # Entry with example placeholder
            entry = ttk.Entry(params_frame, width=50)
            entry.grid(row=idx*2+1, column=0, sticky=(tk.W, tk.E), pady=(0, 10), padx=5)
            entry.insert(0, param.get('example', ''))
            entry.config(foreground='gray')
            
            # Bind events to handle placeholder
            def on_focus_in(event, e=entry, ex=param.get('example', '')):
                if e.get() == ex:
                    e.delete(0, tk.END)
                    e.config(foreground='black')
            
            def on_focus_out(event, e=entry, ex=param.get('example', '')):
                if not e.get():
                    e.insert(0, ex)
                    e.config(foreground='gray')
            
            entry.bind('<FocusIn>', on_focus_in)
            entry.bind('<FocusOut>', on_focus_out)
            
            entries.append((param, entry))
        
        params_frame.columnconfigure(0, weight=1)
        
        # Calculate next row after all entries
        row = len(param_defs) * 2
        
        # Info label in same grid column as fields
        ttk.Label(
            params_frame,
            text="* Required parameters. Leave optional parameters empty to use default.",
            foreground="blue",
            font=("Arial", 9, "italic")
        ).grid(row=row, column=0, sticky=tk.W, pady=(10, 5), padx=5)
        
        row += 1
        
        # Buttons in same grid column as fields
        button_container = ttk.Frame(params_frame)
        button_container.grid(row=row, column=0, sticky=tk.W, pady=(5, 10), padx=5)
        
        result = {"ok": False, "params": []}
        
        def on_ok():
            params = []
            error_msgs = []
            
            for param_def, entry in entries:
                # Check if it's still the placeholder (gray foreground = placeholder)
                if entry.cget('foreground') == 'gray':
                    value = ""
                else:
                    value = entry.get()
                
                # Validate required parameters
                if param_def['required'] and not value:
                    error_msgs.append(f"- {param_def['prompt']} is required")
                    continue
                
                # Use default if empty and not required
                if not value and not param_def['required']:
                    value = param_def.get('default', '')
                
                # Only add non-empty values to params
                if value:
                    # Special handling for cleanup.sh flags
                    if 'dry_run' in param_def['name']:
                        if value.lower() in ['yes', 'y', '1', 'true']:
                            params.append('--dry-run')
                    elif 'days' in param_def['name'] and 'cleanup.sh' in str(script_path):
                        if value:
                            params.extend(['--days', value])
                    else:
                        params.append(value)
            
            if error_msgs:
                messagebox.showerror(
                    "Missing Required Parameters",
                    "Please fill in the required parameters:\n\n" + "\n".join(error_msgs),
                    parent=dialog
                )
                return
            
            result["ok"] = True
            result["params"] = params
            dialog.destroy()
        
        def on_cancel():
            result["ok"] = False
            dialog.destroy()
        
        ok_btn = ttk.Button(button_container, text="OK", command=on_ok, width=15)
        ok_btn.pack(side=tk.LEFT, padx=5)
        
        cancel_btn = ttk.Button(button_container, text="Cancel", command=on_cancel, width=15)
        cancel_btn.pack(side=tk.LEFT, padx=5)
        
        # Wait for dialog to close
        self.root.wait_window(dialog)
        
        return result["params"] if result["ok"] else None
        
    def setup_ui(self):
        """Thiết lập giao diện người dùng"""
        # Thanh điều khiển cửa sổ tự vẽ (vì WM có thể không hiển thị
        # nút thu nhỏ/phóng to trên thanh tiêu đề hệ thống)
        window_controls = ttk.Frame(self.root, padding=(5, 2))
        window_controls.grid(row=0, column=0, sticky=(tk.W, tk.E))
        
        ttk.Label(
            window_controls,
            text="Kernel Linux",
            font=("Arial", 9, "bold")
        ).grid(row=0, column=0, sticky=tk.W)
        
        window_controls.columnconfigure(0, weight=1)
        
        ttk.Button(
            window_controls, text="🗕 Thu nhỏ", width=12,
            command=self.minimize_window
        ).grid(row=0, column=1, padx=2)
        
        self.maximize_btn = ttk.Button(
            window_controls, text="🗖 Phóng to", width=12,
            command=self.toggle_maximize
        )
        self.maximize_btn.grid(row=0, column=2, padx=2)
        
        self.fullscreen_btn = ttk.Button(
            window_controls, text="⛶ Toàn màn hình", width=15,
            command=self.toggle_fullscreen
        )
        self.fullscreen_btn.grid(row=0, column=3, padx=2)
        
        ttk.Separator(self.root, orient=tk.HORIZONTAL).grid(
            row=1, column=0, sticky=(tk.W, tk.E)
        )
        
        # Main container với layout dọc
        main_container = ttk.Frame(self.root, padding="5")
        main_container.grid(row=2, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Cấu hình grid weight
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=0)
        self.root.rowconfigure(1, weight=0)
        self.root.rowconfigure(2, weight=1)
        main_container.columnconfigure(0, weight=0)
        main_container.columnconfigure(1, weight=1)
        main_container.rowconfigure(0, weight=1)
        
        # Panel bên trái cho controls (cố định 250px)
        left_panel = ttk.Frame(main_container, padding="5", width=250)
        left_panel.grid(row=0, column=0, sticky=(tk.W, tk.N, tk.S))
        left_panel.grid_propagate(False)
        left_panel.columnconfigure(0, weight=1)
        left_panel.rowconfigure(0, weight=1)
        
        # Notebook cho 3 tabs trong left panel
        self.notebook = ttk.Notebook(left_panel)
        self.notebook.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Tạo 3 tabs
        self.shell_frame = ttk.Frame(self.notebook)
        self.system_frame = ttk.Frame(self.notebook)
        self.smp_frame = ttk.Frame(self.notebook)
        
        self.notebook.add(self.shell_frame, text="Shell")
        self.notebook.add(self.system_frame, text="System")
        self.notebook.add(self.smp_frame, text="SMP")
        
        # Thiết lập nội dung cho từng tab
        self.setup_shell_tab()
        self.setup_system_tab()
        self.setup_smp_tab()
        
        # Panel bên phải cho terminal (4/5 width)
        right_panel = ttk.Frame(main_container, padding="5")
        right_panel.grid(row=0, column=1, sticky=(tk.W, tk.E, tk.N, tk.S))
        right_panel.columnconfigure(0, weight=1)
        right_panel.rowconfigure(0, weight=1)
        
        # Terminal container
        terminal_container = ttk.Frame(right_panel)
        terminal_container.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        terminal_container.columnconfigure(0, weight=1)
        terminal_container.rowconfigure(0, weight=1)
        
        # Terminal output
        self.terminal_output = scrolledtext.ScrolledText(
            terminal_container,
            wrap=tk.WORD,
            height=15,
            bg="#1e1e1e",
            fg="#ffffff",
            font=("Consolas", 9),
            insertbackground="white"
        )
        self.terminal_output.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Terminal input frame
        input_frame = ttk.Frame(terminal_container)
        input_frame.grid(row=1, column=0, sticky=(tk.W, tk.E), pady=(5, 0))
        input_frame.columnconfigure(0, weight=1)
        
        self.terminal_input = ttk.Entry(input_frame)
        self.terminal_input.grid(row=0, column=0, sticky=(tk.W, tk.E), padx=(0, 5))
        self.terminal_input.bind("<Return>", self.send_input)
        self.terminal_input.bind("<Control-c>", self.send_ctrl_c)
        
        send_btn = ttk.Button(input_frame, text="Enter", command=self.send_input, width=8)
        send_btn.grid(row=0, column=1, sticky=tk.E)
        
        ctrl_c_btn = ttk.Button(
            input_frame, 
            text="Ctrl+C", 
            command=self.send_ctrl_c,
            width=8
        )
        ctrl_c_btn.grid(row=0, column=2, sticky=tk.E, padx=(5, 0))
        
        stop_btn = ttk.Button(
            input_frame, 
            text="Dừng", 
            command=self.stop_process,
            width=8
        )
        stop_btn.grid(row=0, column=3, sticky=tk.E, padx=(5, 0))
        
        clear_btn = ttk.Button(
            input_frame,
            text="Xóa",
            command=self.clear_terminal,
            width=8
        )
        clear_btn.grid(row=0, column=4, sticky=tk.E, padx=(5, 0))
    
    def setup_shell_tab(self):
        """Thiết lập tab Shell Scripting"""
        container = ttk.Frame(self.shell_frame, padding="5")
        container.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        self.shell_frame.columnconfigure(0, weight=1)
        self.shell_frame.rowconfigure(0, weight=1)
        
        # Canvas với scrollbar
        canvas = tk.Canvas(container, highlightthickness=0)
        scrollbar = ttk.Scrollbar(container, orient="vertical", command=canvas.yview)
        scrollable_frame = ttk.Frame(canvas)
        
        scrollable_frame.bind(
            "<Configure>",
            lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
        )
        
        canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)
        
        canvas.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        
        container.columnconfigure(0, weight=1)
        container.rowconfigure(0, weight=1)
        
        # Danh sách các module shell
        shell_modules = [
            ("File Management", "file-management"),
            ("Task Scheduler", "task-scheduler"),
            ("Time Management", "time-management"),
            ("Package Management", "package-management"),
            ("Demo Tổng Hợp", "demo")
        ]
        
        row = 0
        for module_name, module_dir in shell_modules:
            # Tiêu đề module
            ttk.Label(
                scrollable_frame,
                text=module_name,
                font=("Arial", 10, "bold")
            ).grid(row=row, column=0, sticky=tk.W, pady=(10, 5))
            row += 1
            
            module_path = self.project_root / "shell-scripting" / module_dir
            
            if module_dir == "demo":
                # Demo scripts
                demo_scripts = [
                    ("File", "demo-file-management.sh"),
                    ("Package", "demo-package-management.sh"),
                    ("Task", "demo-task-scheduler.sh"),
                    ("Time", "demo-time-management.sh"),
                    ("Advanced", "demo-advanced.sh")
                ]
                
                for btn_name, script in demo_scripts:
                    script_path = module_path / script
                    if script_path.exists():
                        ttk.Button(
                            scrollable_frame,
                            text=btn_name,
                            command=lambda p=script_path: self.run_shell_script(p),
                            width=20
                        ).grid(row=row, column=0, sticky=(tk.W, tk.E), pady=2, padx=5)
                        row += 1
            else:
                # Liệt kê các script trong thư mục
                if module_path.exists():
                    scripts = sorted(module_path.glob("*.sh"))
                    for script in scripts:
                        ttk.Button(
                            scrollable_frame,
                            text=script.stem,
                            command=lambda p=script: self.run_shell_script(p),
                            width=20
                        ).grid(row=row, column=0, sticky=(tk.W, tk.E), pady=2, padx=5)
                        row += 1
        
        # Quick actions
        ttk.Separator(scrollable_frame, orient=tk.HORIZONTAL).grid(
            row=row, column=0, sticky=(tk.W, tk.E), pady=10, padx=5
        )
        row += 1
        
        ttk.Label(
            scrollable_frame,
            text="Quick Actions",
            font=("Arial", 10, "bold")
        ).grid(row=row, column=0, sticky=tk.W, pady=(0, 5), padx=5)
        row += 1
        
        ttk.Button(
            scrollable_frame,
            text="Demo Tổng Hợp",
            command=lambda: self.run_shell_script(
                self.project_root / "shell-scripting" / "demo.sh"
            ),
            width=20
        ).grid(row=row, column=0, sticky=(tk.W, tk.E), pady=2, padx=5)
        row += 1
        
        ttk.Button(
            scrollable_frame,
            text="Quick Start",
            command=lambda: self.run_shell_script(
                self.project_root / "shell-scripting" / "QUICKSTART.sh"
            ),
            width=20
        ).grid(row=row, column=0, sticky=(tk.W, tk.E), pady=2, padx=5)
        row += 1
        
        ttk.Button(
            scrollable_frame,
            text="Setup GUI",
            command=lambda: self.run_shell_script(
                self.project_root / "shell-scripting" / "setup_gui.sh"
            ),
            width=20
        ).grid(row=row, column=0, sticky=(tk.W, tk.E), pady=2, padx=5)
    
    def setup_system_tab(self):
        """Thiết lập tab System Programming"""
        container = ttk.Frame(self.system_frame, padding="5")
        container.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        self.system_frame.columnconfigure(0, weight=1)
        self.system_frame.rowconfigure(0, weight=1)
        
        # Canvas với scrollbar
        canvas = tk.Canvas(container, highlightthickness=0)
        scrollbar = ttk.Scrollbar(container, orient="vertical", command=canvas.yview)
        scrollable_frame = ttk.Frame(canvas)
        
        scrollable_frame.bind(
            "<Configure>",
            lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
        )
        
        canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)
        
        canvas.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        
        container.columnconfigure(0, weight=1)
        container.rowconfigure(0, weight=1)
        
        # Danh sách các module system
        system_modules = [
            ("Process Management", "process", False, False),
            ("File I/O Operations", "file", False, True),
            ("Socket Programming", "socket", False, False),
            ("Network Programming", "network", True, True)
        ]
        
        # Danh sách các file cần sudo trong từng module
        files_need_sudo = {
            "process": ["process_priority"],
            "network": []  # Tất cả file trong network đều cần sudo
        }
        
        row = 0
        for module_name, module_dir, needs_sudo, has_auto_test in system_modules:
            # Tiêu đề module
            ttk.Label(
                scrollable_frame,
                text=module_name,
                font=("Arial", 10, "bold")
            ).grid(row=row, column=0, sticky=tk.W, pady=(10, 5), padx=5)
            row += 1
            
            module_path = self.project_root / "system" / module_dir
            
            if module_path.exists():
                # Nút compile
                makefile = module_path / "Makefile"
                if makefile.exists():
                    ttk.Button(
                        scrollable_frame,
                        text="Compile",
                        command=lambda p=module_path: self.compile_c_project(p),
                        width=20
                    ).grid(row=row, column=0, sticky=(tk.W, tk.E), pady=2, padx=5)
                    row += 1
                
                # Liệt kê các file C
                c_files = sorted(module_path.glob("*.c"))
                for c_file in c_files:
                    exe_name = c_file.stem
                    exe_path = module_path / exe_name
                    
                    # Kiểm tra xem file có cần sudo không
                    file_needs_sudo = needs_sudo or (module_dir in files_need_sudo and exe_name in files_need_sudo[module_dir])
                    
                    if has_auto_test:
                        # Tìm test script tương ứng
                        test_script = module_path / f"test_{exe_name}.sh"
                        if test_script.exists():
                            if file_needs_sudo:
                                ttk.Button(
                                    scrollable_frame,
                                    text=f"{exe_name} (sudo)",
                                    command=lambda p=test_script: self.run_shell_script_sudo(p),
                                    width=20
                                ).grid(row=row, column=0, sticky=(tk.W, tk.E), pady=2, padx=5)
                            else:
                                ttk.Button(
                                    scrollable_frame,
                                    text=exe_name,
                                    command=lambda p=test_script: self.run_shell_script(p),
                                    width=20
                                ).grid(row=row, column=0, sticky=(tk.W, tk.E), pady=2, padx=5)
                            row += 1
                        else:
                            # Không có test script, chạy executable
                            if file_needs_sudo:
                                ttk.Button(
                                    scrollable_frame,
                                    text=f"{exe_name} (sudo)",
                                    command=lambda p=exe_path: self.run_executable_sudo(p),
                                    width=20
                                ).grid(row=row, column=0, sticky=(tk.W, tk.E), pady=2, padx=5)
                            else:
                                ttk.Button(
                                    scrollable_frame,
                                    text=exe_name,
                                    command=lambda p=exe_path: self.run_executable(p),
                                    width=20
                                ).grid(row=row, column=0, sticky=(tk.W, tk.E), pady=2, padx=5)
                            row += 1
                    else:
                        if file_needs_sudo:
                            ttk.Button(
                                scrollable_frame,
                                text=f"{exe_name} (sudo)",
                                command=lambda p=exe_path: self.run_executable_sudo(p),
                                width=20
                            ).grid(row=row, column=0, sticky=(tk.W, tk.E), pady=2, padx=5)
                        else:
                            ttk.Button(
                                scrollable_frame,
                                text=exe_name,
                                command=lambda p=exe_path: self.run_executable(p),
                                width=20
                            ).grid(row=row, column=0, sticky=(tk.W, tk.E), pady=2, padx=5)
                        row += 1
                
                # Nút test script đặc biệt (không tương ứng với file C)
                if module_dir == "file":
                    # Thêm VFS module
                    vfs_test = module_path / "vfs_module" / "test_vfs.sh"
                    if vfs_test.exists():
                        ttk.Button(
                            scrollable_frame,
                            text="VFS Module (sudo)",
                            command=lambda p=vfs_test: self.run_shell_script_sudo(p),
                            width=20
                        ).grid(row=row, column=0, sticky=(tk.W, tk.E), pady=2, padx=5)
                        row += 1
                
                # Test scripts không auto (socket)
                if not has_auto_test:
                    test_scripts = sorted(module_path.glob("test_*.sh"))
                    for test_script in test_scripts:
                        ttk.Button(
                            scrollable_frame,
                            text=f"Test: {test_script.stem}",
                            command=lambda p=test_script: self.run_shell_script(p),
                            width=20
                        ).grid(row=row, column=0, sticky=(tk.W, tk.E), pady=2, padx=5)
                        row += 1
        
        # Quick actions
        ttk.Separator(scrollable_frame, orient=tk.HORIZONTAL).grid(
            row=row, column=0, sticky=(tk.W, tk.E), pady=10, padx=5
        )
        row += 1
        
        ttk.Label(
            scrollable_frame,
            text="Quick Actions",
            font=("Arial", 10, "bold")
        ).grid(row=row, column=0, sticky=tk.W, pady=(0, 5), padx=5)
        row += 1
        
        ttk.Button(
            scrollable_frame,
            text="Compile Tất Cả",
            command=lambda: self.compile_all_system(),
            width=20
        ).grid(row=row, column=0, sticky=(tk.W, tk.E), pady=2, padx=5)
        row += 1
        
        ttk.Button(
            scrollable_frame,
            text="Clean Tất Cả",
            command=lambda: self.clean_all_system(),
            width=20
        ).grid(row=row, column=0, sticky=(tk.W, tk.E), pady=2, padx=5)
    
    def setup_smp_tab(self):
        """Thiết lập tab SMP Programming"""
        container = ttk.Frame(self.smp_frame, padding="5")
        container.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        self.smp_frame.columnconfigure(0, weight=1)
        self.smp_frame.rowconfigure(0, weight=1)
        
        # Canvas với scrollbar
        canvas = tk.Canvas(container, highlightthickness=0)
        scrollbar = ttk.Scrollbar(container, orient="vertical", command=canvas.yview)
        scrollable_frame = ttk.Frame(canvas)
        
        scrollable_frame.bind(
            "<Configure>",
            lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
        )
        
        canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)
        
        canvas.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        
        container.columnconfigure(0, weight=1)
        container.rowconfigure(0, weight=1)
        
        # Danh sách các ví dụ SMP
        smp_examples = [
            ("ex01: Race Condition", "race-condition-ex01"),
            ("ex02: Atomic Fix", "atomic-fix-ex02"),
            ("ex03: IRQ Disable", "irq-disable-ex03"),
            ("ex04: Spinlock Basic", "spinlock-basic-ex04"),
            ("ex05: Spinlock Optimized", "spinlock-optimized-ex05"),
            ("ex06: Preemption Counter", "preemption-counter-ex06"),
            ("ex07: Mutex Lock", "mutex-lock-ex07"),
            ("ex08: Mutex Lock Slow", "mutex-lock-slow-ex08"),
            ("ex09: Mutex Unlock", "mutex-unlock-ex09"),
            ("ex10: Memory Ordering RCU", "memory-ordering-rcu-ex10"),
            ("ex11: Semaphore", "semaphore-ex11"),
            ("ex12: Per-CPU Data", "per-cpu-data-ex12")
        ]
        
        row = 0
        for example_name, example_dir in smp_examples:
            # Tiêu đề
            ttk.Label(
                scrollable_frame,
                text=example_name,
                font=("Arial", 9, "bold")
            ).grid(row=row, column=0, sticky=tk.W, pady=(10, 5), padx=5)
            row += 1
            
            example_path = self.project_root / "smp-programming" / example_dir
            
            if example_path.exists():
                # Nút compile
                makefile = example_path / "Makefile"
                if makefile.exists():
                    ttk.Button(
                        scrollable_frame,
                        text="Compile",
                        command=lambda p=example_path: self.compile_c_project(p),
                        width=20
                    ).grid(row=row, column=0, sticky=(tk.W, tk.E), pady=2, padx=5)
                    row += 1
                
                # Tìm executable
                for f in example_path.iterdir():
                    if f.is_file() and os.access(f, os.X_OK) and not f.suffix:
                        ttk.Button(
                            scrollable_frame,
                            text="Chạy",
                            command=lambda p=f: self.run_executable(p),
                            width=20
                        ).grid(row=row, column=0, sticky=(tk.W, tk.E), pady=2, padx=5)
                        row += 1
                        break
        
        # Quick actions
        ttk.Separator(scrollable_frame, orient=tk.HORIZONTAL).grid(
            row=row, column=0, sticky=(tk.W, tk.E), pady=10, padx=5
        )
        row += 1
        
        ttk.Label(
            scrollable_frame,
            text="Quick Actions",
            font=("Arial", 10, "bold")
        ).grid(row=row, column=0, sticky=tk.W, pady=(0, 5), padx=5)
        row += 1
        
        ttk.Button(
            scrollable_frame,
            text="Demo Tổng Hợp",
            command=lambda: self.run_shell_script(
                self.project_root / "smp-programming" / "demo.sh"
            ),
            width=20
        ).grid(row=row, column=0, sticky=(tk.W, tk.E), pady=2, padx=5)
        row += 1
        
        ttk.Button(
            scrollable_frame,
            text="Compile Tất Cả",
            command=lambda: self.compile_all_smp(),
            width=20
        ).grid(row=row, column=0, sticky=(tk.W, tk.E), pady=2, padx=5)
        row += 1
        
        ttk.Button(
            scrollable_frame,
            text="Clean Tất Cả",
            command=lambda: self.clean_all_smp(),
            width=20
        ).grid(row=row, column=0, sticky=(tk.W, tk.E), pady=2, padx=5)
    
    def run_shell_script(self, script_path):
        """Chạy shell script"""
        if not script_path.exists():
            self.log_terminal(f"Lỗi: Không tìm thấy script {script_path}\n")
            return
        
        # Đảm bảo script có quyền thực thi
        os.chmod(script_path, 0o755)
        
        # Collect parameters if script needs them
        script_params = []
        needs_sudo = False
        rel_path = str(script_path.relative_to(self.project_root))
        if rel_path in self.script_params:
            needs_sudo = self.script_params[rel_path].get("needs_sudo", False)
            script_params = self.collect_script_parameters(script_path)
            if script_params is None:  # User cancelled
                self.log_terminal(f"Đã hủy: {script_path.name}\n")
                return
        
        self.log_terminal(f"\n{'='*60}\n")
        self.log_terminal(f"Đang chạy: {script_path.name}\n")
        if needs_sudo:
            self.log_terminal(f"Quyền: sudo (required)\n")
        self.log_terminal(f"Đường dẫn: {script_path}\n")
        if script_params:
            self.log_terminal(f"Parameters: {' '.join(script_params)}\n")
        self.log_terminal(f"{'='*60}\n\n")
        
        # Build command with sudo if needed
        if needs_sudo:
            command = ["sudo", "bash", str(script_path)]
        else:
            command = ["bash", str(script_path)]
        
        if script_params:
            command.extend(script_params)
        
        # Chạy trong thread riêng
        thread = threading.Thread(
            target=self._run_command,
            args=(command, script_path.parent),
            daemon=True
        )
        thread.start()
    
    def run_shell_script_sudo(self, script_path):
        """Chạy shell script với quyền sudo"""
        if not script_path.exists():
            self.log_terminal(f"Lỗi: Không tìm thấy script {script_path}\n")
            return
        
        # Đảm bảo script có quyền thực thi
        os.chmod(script_path, 0o755)
        
        # Collect parameters if script needs them
        script_params = []
        rel_path = str(script_path.relative_to(self.project_root))
        if rel_path in self.script_params:
            script_params = self.collect_script_parameters(script_path)
            if script_params is None:  # User cancelled
                self.log_terminal(f"Đã hủy: {script_path.name}\n")
                return
        
        self.log_terminal(f"\n{'='*60}\n")
        self.log_terminal(f"Đang chạy với sudo: {script_path.name}\n")
        self.log_terminal(f"Đường dẫn: {script_path}\n")
        if script_params:
            self.log_terminal(f"Parameters: {' '.join(script_params)}\n")
        self.log_terminal(f"{'='*60}\n\n")
        
        # Build command with parameters
        command = ["sudo", "bash", str(script_path)]
        if script_params:
            command.extend(script_params)
        
        # Chạy với sudo trong thread riêng
        thread = threading.Thread(
            target=self._run_command,
            args=(command, script_path.parent),
            daemon=True
        )
        thread.start()
        thread.start()
    
    def run_executable(self, exe_path):
        """Chạy file thực thi C"""
        if not exe_path.exists():
            self.log_terminal(f"Lỗi: Không tìm thấy executable {exe_path}\n")
            return
        
        self.log_terminal(f"\n{'='*60}\n")
        self.log_terminal(f"Đang chạy: {exe_path.name}\n")
        self.log_terminal(f"Đường dẫn: {exe_path}\n")
        self.log_terminal(f"{'='*60}\n\n")
        
        # Chạy trong thread riêng
        thread = threading.Thread(
            target=self._run_command,
            args=([str(exe_path)], exe_path.parent),
            daemon=True
        )
        thread.start()
    
    def run_executable_sudo(self, exe_path):
        """Chạy file thực thi C với quyền sudo"""
        if not exe_path.exists():
            self.log_terminal(f"Lỗi: Không tìm thấy executable {exe_path}\n")
            return
        
        self.log_terminal(f"\n{'='*60}\n")
        self.log_terminal(f"Đang chạy với sudo: {exe_path.name}\n")
        self.log_terminal(f"Đường dẫn: {exe_path}\n")
        self.log_terminal(f"{'='*60}\n\n")
        
        # Chạy với sudo trong thread riêng
        thread = threading.Thread(
            target=self._run_command,
            args=(["sudo", str(exe_path)], exe_path.parent),
            daemon=True
        )
        thread.start()
    
    def compile_c_project(self, project_path):
        """Compile project C với Makefile"""
        makefile = project_path / "Makefile"
        if not makefile.exists():
            self.log_terminal(f"Lỗi: Không tìm thấy Makefile trong {project_path}\n")
            return
        
        self.log_terminal(f"\n{'='*60}\n")
        self.log_terminal(f"Đang clean & compile: {project_path.name}\n")
        self.log_terminal(f"Thư mục: {project_path}\n")
        self.log_terminal(f"{'='*60}\n\n")
        
        # Chạy make clean trước, sau đó make (dùng ';' để dù clean có
        # lỗi/không có gì để xóa thì vẫn tiếp tục compile)
        cmd = f"cd '{project_path}' && make clean; make"
        thread = threading.Thread(
            target=self._run_command,
            args=(["bash", "-c", cmd], self.project_root),
            daemon=True
        )
        thread.start()
    
    def compile_all_system(self):
        """Compile tất cả các module system"""
        self.log_terminal(f"\n{'='*60}\n")
        self.log_terminal("Đang compile tất cả module System Programming...\n")
        self.log_terminal(f"{'='*60}\n\n")
        
        system_dir = self.project_root / "system"
        for module_dir in sorted(system_dir.iterdir()):
            if module_dir.is_dir() and (module_dir / "Makefile").exists():
                self.log_terminal(f"Compile {module_dir.name}...\n")
                cmd = f"cd '{module_dir}' && make clean; make"
                thread = threading.Thread(
                    target=self._run_command,
                    args=(["bash", "-c", cmd], self.project_root),
                    daemon=True
                )
                thread.start()
                thread.join()
    
    def clean_all_system(self):
        """Clean tất cả các module system"""
        self.log_terminal(f"\n{'='*60}\n")
        self.log_terminal("Đang clean tất cả module System Programming...\n")
        self.log_terminal(f"{'='*60}\n\n")
        
        system_dir = self.project_root / "system"
        for module_dir in sorted(system_dir.iterdir()):
            if module_dir.is_dir() and (module_dir / "Makefile").exists():
                self.log_terminal(f"Clean {module_dir.name}...\n")
                cmd = f"cd '{module_dir}' && make clean"
                thread = threading.Thread(
                    target=self._run_command,
                    args=(["bash", "-c", cmd], self.project_root),
                    daemon=True
                )
                thread.start()
                thread.join()
    
    def compile_all_smp(self):
        """Compile tất cả các ví dụ SMP"""
        self.log_terminal(f"\n{'='*60}\n")
        self.log_terminal("Đang compile tất cả ví dụ SMP Programming...\n")
        self.log_terminal(f"{'='*60}\n\n")
        
        smp_dir = self.project_root / "smp-programming"
        for example_dir in sorted(smp_dir.iterdir()):
            if example_dir.is_dir() and (example_dir / "Makefile").exists():
                self.log_terminal(f"Compile {example_dir.name}...\n")
                cmd = f"cd '{example_dir}' && make clean; make"
                thread = threading.Thread(
                    target=self._run_command,
                    args=(["bash", "-c", cmd], self.project_root),
                    daemon=True
                )
                thread.start()
                thread.join()
    
    def clean_all_smp(self):
        """Clean tất cả các ví dụ SMP"""
        self.log_terminal(f"\n{'='*60}\n")
        self.log_terminal("Đang clean tất cả ví dụ SMP Programming...\n")
        self.log_terminal(f"{'='*60}\n\n")
        
        smp_dir = self.project_root / "smp-programming"
        for example_dir in sorted(smp_dir.iterdir()):
            if example_dir.is_dir() and (example_dir / "Makefile").exists():
                self.log_terminal(f"Clean {example_dir.name}...\n")
                cmd = f"cd '{example_dir}' && make clean"
                thread = threading.Thread(
                    target=self._run_command,
                    args=(["bash", "-c", cmd], self.project_root),
                    daemon=True
                )
                thread.start()
                thread.join()
    
    def minimize_window(self):
        """Thu nhỏ cửa sổ xuống taskbar (không phụ thuộc WM có vẽ nút hay không)"""
        try:
            self.root.iconify()
        except tk.TclError as e:
            self.log_terminal(f"Không thể thu nhỏ cửa sổ: {e}\n")
    
    def toggle_maximize(self):
        """Phóng to / khôi phục kích thước cửa sổ"""
        if self.is_fullscreen:
            # Đang ở fullscreen thì thoát fullscreen trước
            self.exit_fullscreen()
        
        if not self.is_maximized:
            self._normal_geometry = self.root.geometry()
            try:
                # Linux/X11
                self.root.attributes('-zoomed', True)
            except tk.TclError:
                # Windows / fallback: phóng to bằng kích thước màn hình
                sw = self.root.winfo_screenwidth()
                sh = self.root.winfo_screenheight()
                self.root.geometry(f"{sw}x{sh}+0+0")
            self.is_maximized = True
            self.maximize_btn.config(text="🗗 Khôi phục")
        else:
            try:
                self.root.attributes('-zoomed', False)
            except tk.TclError:
                pass
            self.root.geometry(self._normal_geometry)
            self.is_maximized = False
            self.maximize_btn.config(text="🗖 Phóng to")
    
    def toggle_fullscreen(self):
        """Bật/tắt chế độ toàn màn hình (ẩn cả thanh tiêu đề hệ thống)"""
        self.is_fullscreen = not self.is_fullscreen
        if self.is_fullscreen:
            self._normal_geometry = self.root.geometry()
        self.root.attributes('-fullscreen', self.is_fullscreen)
        self.fullscreen_btn.config(
            text="⛶ Thoát toàn màn hình" if self.is_fullscreen else "⛶ Toàn màn hình"
        )
    
    def exit_fullscreen(self):
        """Thoát chế độ toàn màn hình (gọi khi nhấn Esc)"""
        if self.is_fullscreen:
            self.is_fullscreen = False
            self.root.attributes('-fullscreen', False)
            self.fullscreen_btn.config(text="⛶ Toàn màn hình")
    
    def view_file(self, file_path):
        """Xem nội dung file trong cửa sổ mới"""
        if not file_path.exists():
            self.log_terminal(f"Lỗi: Không tìm thấy file {file_path}\n")
            return
        
        # Tạo cửa sổ mới
        viewer_window = tk.Toplevel(self.root)
        viewer_window.title(f"Xem File: {file_path.name}")
        viewer_window.geometry("900x700")
        
        # Frame chứa
        frame = ttk.Frame(viewer_window, padding="10")
        frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        viewer_window.columnconfigure(0, weight=1)
        viewer_window.rowconfigure(0, weight=1)
        frame.columnconfigure(0, weight=1)
        frame.rowconfigure(1, weight=1)
        
        # Label
        ttk.Label(
            frame,
            text=f"File: {file_path}",
            font=("Arial", 10, "bold")
        ).grid(row=0, column=0, sticky=tk.W, pady=(0, 5))
        
        # Text widget
        text_widget = scrolledtext.ScrolledText(
            frame,
            wrap=tk.WORD,
            font=("Consolas", 9)
        )
        text_widget.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Đọc và hiển thị nội dung
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
                text_widget.insert('1.0', content)
                text_widget.config(state='disabled')
        except Exception as e:
            text_widget.insert('1.0', f"Lỗi khi đọc file: {e}")
            text_widget.config(state='disabled')
    
    def _set_pty_winsize(self, fd, rows=40, cols=120):
        """Thiết lập kích thước (rows/cols) cho PTY để chương trình con
        biết kích thước 'màn hình' của mình, giống terminal thật."""
        try:
            winsize = struct.pack('HHHH', rows, cols, 0, 0)
            fcntl.ioctl(fd, termios.TIOCSWINSZ, winsize)
        except Exception:
            # Không nghiêm trọng nếu thất bại trên một số hệ thống
            pass
    
    def _run_command(self, cmd, cwd):
        """Chạy lệnh trong subprocess với output realtime, dùng PTY thật
        thay vì pipe thông thường để chương trình con coi đây là tty
        (giống hệt khi chạy trên terminal thật):
        - stdout giữ line-buffering, màu ANSI hiển thị đúng
        - isatty() trả về True
        - sudo có thể đọc password qua tty
        - Ctrl+C gửi đúng tới cả process group (job control)
        """
        master_fd = None
        slave_fd = None
        
        with self.process_lock:
            # Dừng process cũ nếu đang chạy
            if self.current_process and self.current_process.poll() is None:
                try:
                    os.killpg(os.getpgid(self.current_process.pid), signal.SIGTERM)
                except Exception:
                    self.current_process.terminate()
                try:
                    self.current_process.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    try:
                        os.killpg(os.getpgid(self.current_process.pid), signal.SIGKILL)
                    except Exception:
                        self.current_process.kill()
            
            # Đóng PTY cũ (nếu còn) trước khi tạo PTY mới
            if self.master_fd is not None:
                try:
                    os.close(self.master_fd)
                except OSError:
                    pass
                self.master_fd = None
            
            try:
                # Tạo cặp PTY master/slave
                master_fd, slave_fd = pty.openpty()
                self._set_pty_winsize(slave_fd)
                
                # Tạo process mới, gắn cả stdin/stdout/stderr vào đầu slave
                # của PTY -> chương trình con thấy một tty thật.
                self.current_process = subprocess.Popen(
                    cmd,
                    cwd=cwd,
                    stdin=slave_fd,
                    stdout=slave_fd,
                    stderr=slave_fd,
                    env={**os.environ, 'TERM': 'xterm-256color', 'LANG': 'C.UTF-8', 'LC_ALL': 'C.UTF-8'},
                    preexec_fn=os.setsid,  # tạo session/process group mới cho job control
                    close_fds=True,
                )
            except Exception as e:
                if master_fd is not None:
                    os.close(master_fd)
                if slave_fd is not None:
                    os.close(slave_fd)
                self.output_queue.put(f"Lỗi khi chạy lệnh: {e}\n")
                return
            
            # Đầu slave đã được tiến trình con kế thừa, cha có thể đóng lại
            os.close(slave_fd)
            self.master_fd = master_fd
        
        # Đọc output từ đầu master của PTY
        try:
            while True:
                try:
                    ready, _, _ = select.select([master_fd], [], [], 0.1)
                except (OSError, ValueError):
                    break
                
                if self.current_process.poll() is not None and not ready:
                    break
                
                if master_fd in ready:
                    try:
                        data = os.read(master_fd, 4096)
                    except OSError:
                        # PTY đã đóng (process con kết thúc và giải phóng slave cuối)
                        break
                    if not data:
                        break
                    text = data.decode('utf-8', errors='replace')
                    # PTY ở chế độ "cooked" tự chuyển \n -> \r\n (cờ ONLCR) khi
                    # echo/xuống dòng, giống terminal thật. Nhưng tkinter.Text
                    # không phải terminal emulator: nó không hiểu \r là "quay về
                    # đầu dòng" mà chỉ chèn nó như 1 ký tự thường -> hiện ô vuông
                    # lạ (font không có glyph cho \r). Nên chuẩn hóa lại trước khi
                    # đưa vào terminal_output:
                    text = text.replace('\r\n', '\n').replace('\r', '')
                    self.output_queue.put(text)
            
            # Chờ process kết thúc
            return_code = self.current_process.wait()
            
            if return_code == 0:
                self.output_queue.put(f"\n[Process hoàn thành thành công]\n")
            else:
                self.output_queue.put(f"\n[Process kết thúc với mã lỗi: {return_code}]\n")
        
        except Exception as e:
            self.output_queue.put(f"\nLỗi khi đọc output: {e}\n")
        finally:
            with self.process_lock:
                if self.master_fd == master_fd:
                    try:
                        os.close(master_fd)
                    except OSError:
                        pass
                    self.master_fd = None
    
    def send_input(self, event=None):
        """Gửi input đến process đang chạy (ghi trực tiếp vào PTY,
        giống như gõ trên terminal thật)."""
        input_text = self.terminal_input.get()
        
        with self.process_lock:
            if self.current_process and self.current_process.poll() is None and self.master_fd is not None:
                try:
                    os.write(self.master_fd, (input_text + '\n').encode('utf-8'))
                    # PTY ở chế độ "canonical/echo" sẽ tự echo lại ký tự đã gõ,
                    # nên không cần tự log "> input" như trước nữa.
                    self.terminal_input.delete(0, tk.END)
                except OSError as e:
                    self.log_terminal(f"Lỗi khi gửi input: {e}\n")
            else:
                self.log_terminal("Không có process nào đang chạy\n")
    
    def send_ctrl_c(self, event=None):
        """Gửi tín hiệu Ctrl+C đến process đang chạy.
        Gửi tới cả process group (giống terminal thật khi nhấn Ctrl+C),
        không chỉ tới một tiến trình đơn lẻ, để các tiến trình con do
        script/sudo sinh ra cũng nhận được tín hiệu."""
        with self.process_lock:
            if self.current_process and self.current_process.poll() is None:
                try:
                    self.log_terminal("\n[Gửi Ctrl+C...]\n")
                    os.killpg(os.getpgid(self.current_process.pid), signal.SIGINT)
                except Exception as e:
                    self.log_terminal(f"Lỗi khi gửi Ctrl+C: {e}\n")
            else:
                self.log_terminal("Không có process nào đang chạy\n")
    
    def stop_process(self):
        """Dừng process đang chạy (gửi tín hiệu tới cả process group)."""
        with self.process_lock:
            if self.current_process and self.current_process.poll() is None:
                self.log_terminal("\n[Đang dừng process...]\n")
                try:
                    os.killpg(os.getpgid(self.current_process.pid), signal.SIGTERM)
                except Exception:
                    self.current_process.terminate()
                
                try:
                    self.current_process.wait(timeout=2)
                    self.log_terminal("[Process đã dừng]\n")
                except subprocess.TimeoutExpired:
                    try:
                        os.killpg(os.getpgid(self.current_process.pid), signal.SIGKILL)
                    except Exception:
                        self.current_process.kill()
                    self.log_terminal("[Process đã bị kill (force)]\n")
            else:
                self.log_terminal("Không có process nào đang chạy\n")
    
    def clear_terminal(self):
        """Xóa nội dung terminal"""
        self.terminal_output.delete('1.0', tk.END)
    
    def log_terminal(self, text):
        """Ghi log vào terminal"""
        self.terminal_output.insert(tk.END, text)
        self.terminal_output.see(tk.END)
    
    def update_output(self):
        """Cập nhật output từ queue"""
        try:
            while True:
                text = self.output_queue.get_nowait()
                self.log_terminal(text)
        except queue.Empty:
            pass
        
        # Lên lịch cập nhật tiếp theo
        self.root.after(50, self.update_output)


def main():
    """Hàm main"""
    root = tk.Tk()
    app = KernelLinuxGUI(root)
    root.mainloop()


if __name__ == "__main__":
    main()
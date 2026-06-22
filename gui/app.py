#!/usr/bin/env python3
"""
Ứng dụng GUI quản lý dự án Kernel Linux
Hỗ trợ chạy các demo và script từ 3 phần: Shell, System, SMP
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
import subprocess
import threading
import os
import sys
from pathlib import Path
import queue
import select
import fcntl

class KernelLinuxGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Kernel Linux")
        self.root.geometry("1400x900")
        
        # Đường dẫn gốc dự án
        self.project_root = Path(__file__).parent.parent
        
        # Queue để giao tiếp với thread terminal
        self.output_queue = queue.Queue()
        
        # Process hiện tại
        self.current_process = None
        self.process_lock = threading.Lock()
        
        # Thiết lập giao diện
        self.setup_ui()
        
        # Bắt đầu cập nhật output
        self.update_output()
        
    def setup_ui(self):
        """Thiết lập giao diện người dùng"""
        # Main container với layout dọc
        main_container = ttk.Frame(self.root, padding="5")
        main_container.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Cấu hình grid weight
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_container.columnconfigure(0, weight=1)
        main_container.columnconfigure(1, weight=4)
        main_container.rowconfigure(0, weight=1)
        
        # Panel bên trái cho controls (1/5 width)
        left_panel = ttk.Frame(main_container, padding="5")
        left_panel.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
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
        container = ttk.Frame(self.shell_frame, padding="10")
        container.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        self.shell_frame.columnconfigure(0, weight=1)
        self.shell_frame.rowconfigure(0, weight=1)
        container.columnconfigure(1, weight=1)
        
        # Danh sách các module shell
        shell_modules = [
            ("File Management", "file-management"),
            ("Task Scheduler", "task-scheduler"),
            ("Time Management", "time-management"),
            ("Package Management", "package-management"),
            ("Demo Tổng Hợp", "demo")
        ]
        
        row = 0
        ttk.Label(
            container,
            text="Các Module Shell Scripting:",
            font=("Arial", 11, "bold")
        ).grid(row=row, column=0, columnspan=2, sticky=tk.W, pady=(0, 10))
        row += 1
        
        for module_name, module_dir in shell_modules:
            # Label
            ttk.Label(container, text=f"{module_name}:").grid(
                row=row, column=0, sticky=tk.W, pady=5, padx=(0, 10)
            )
            
            # Buttons frame
            btn_frame = ttk.Frame(container)
            btn_frame.grid(row=row, column=1, sticky=tk.W, pady=5)
            
            module_path = self.project_root / "shell-scripting" / module_dir
            
            if module_dir == "demo":
                # Demo scripts
                demo_scripts = [
                    ("Demo File", "demo-file-management.sh"),
                    ("Demo Package", "demo-package-management.sh"),
                    ("Demo Task", "demo-task-scheduler.sh"),
                    ("Demo Time", "demo-time-management.sh"),
                    ("Demo Advanced", "demo-advanced.sh")
                ]
                
                col = 0
                for btn_name, script in demo_scripts:
                    script_path = module_path / script
                    if script_path.exists():
                        ttk.Button(
                            btn_frame,
                            text=btn_name,
                            command=lambda p=script_path: self.run_shell_script(p)
                        ).grid(row=0, column=col, padx=2)
                        col += 1
            else:
                # Liệt kê các script trong thư mục
                if module_path.exists():
                    scripts = sorted(module_path.glob("*.sh"))
                    col = 0
                    for script in scripts[:5]:  # Giới hạn 5 scripts
                        ttk.Button(
                            btn_frame,
                            text=script.stem,
                            command=lambda p=script: self.run_shell_script(p)
                        ).grid(row=0, column=col, padx=2)
                        col += 1
                    
                    # Nút xem README
                    readme = module_path / "README.md"
                    if readme.exists():
                        ttk.Button(
                            btn_frame,
                            text="README",
                            command=lambda p=readme: self.view_file(p)
                        ).grid(row=0, column=col, padx=2)
            
            row += 1
        
        # Quick actions
        ttk.Separator(container, orient=tk.HORIZONTAL).grid(
            row=row, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=10
        )
        row += 1
        
        ttk.Label(
            container,
            text="Quick Actions:",
            font=("Arial", 11, "bold")
        ).grid(row=row, column=0, columnspan=2, sticky=tk.W, pady=(0, 5))
        row += 1
        
        quick_frame = ttk.Frame(container)
        quick_frame.grid(row=row, column=0, columnspan=2, sticky=tk.W)
        
        ttk.Button(
            quick_frame,
            text="Chạy Demo Tổng Hợp",
            command=lambda: self.run_shell_script(
                self.project_root / "shell-scripting" / "demo.sh"
            )
        ).grid(row=0, column=0, padx=5)
        
        ttk.Button(
            quick_frame,
            text="Quick Start",
            command=lambda: self.run_shell_script(
                self.project_root / "shell-scripting" / "QUICKSTART.sh"
            )
        ).grid(row=0, column=1, padx=5)
        
        ttk.Button(
            quick_frame,
            text="Setup GUI",
            command=lambda: self.run_shell_script(
                self.project_root / "shell-scripting" / "setup_gui.sh"
            )
        ).grid(row=0, column=2, padx=5)
    
    def setup_system_tab(self):
        """Thiết lập tab System Programming"""
        container = ttk.Frame(self.system_frame, padding="10")
        container.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        self.system_frame.columnconfigure(0, weight=1)
        self.system_frame.rowconfigure(0, weight=1)
        container.columnconfigure(1, weight=1)
        
        # Danh sách các module system
        system_modules = [
            ("Process Management", "process"),
            ("File I/O Operations", "file"),
            ("Socket Programming", "socket"),
            ("Network Programming", "network")
        ]
        
        row = 0
        ttk.Label(
            container,
            text="Các Module System Programming:",
            font=("Arial", 11, "bold")
        ).grid(row=row, column=0, columnspan=2, sticky=tk.W, pady=(0, 10))
        row += 1
        
        for module_name, module_dir in system_modules:
            # Label
            ttk.Label(container, text=f"{module_name}:").grid(
                row=row, column=0, sticky=tk.W, pady=5, padx=(0, 10)
            )
            
            # Buttons frame
            btn_frame = ttk.Frame(container)
            btn_frame.grid(row=row, column=1, sticky=tk.W, pady=5)
            
            module_path = self.project_root / "system" / module_dir
            
            if module_path.exists():
                # Nút compile
                makefile = module_path / "Makefile"
                if makefile.exists():
                    ttk.Button(
                        btn_frame,
                        text="Compile",
                        command=lambda p=module_path: self.compile_c_project(p)
                    ).grid(row=0, column=0, padx=2)
                
                # Liệt kê các file thực thi
                executables = []
                for f in module_path.iterdir():
                    if f.is_file() and os.access(f, os.X_OK) and not f.suffix:
                        executables.append(f)
                
                col = 1
                for exe in sorted(executables)[:4]:  # Giới hạn 4 executables
                    ttk.Button(
                        btn_frame,
                        text=f"Chạy {exe.name}",
                        command=lambda p=exe: self.run_executable(p)
                    ).grid(row=0, column=col, padx=2)
                    col += 1
                
                # Nút test script nếu có
                test_scripts = list(module_path.glob("test_*.sh"))
                for test_script in test_scripts[:2]:
                    ttk.Button(
                        btn_frame,
                        text=f"Test {test_script.stem}",
                        command=lambda p=test_script: self.run_shell_script(p)
                    ).grid(row=0, column=col, padx=2)
                    col += 1
                
                # Nút README
                readme = module_path / "README.md"
                if readme.exists():
                    ttk.Button(
                        btn_frame,
                        text="README",
                        command=lambda p=readme: self.view_file(p)
                    ).grid(row=0, column=col, padx=2)
            
            row += 1
        
        # Quick actions
        ttk.Separator(container, orient=tk.HORIZONTAL).grid(
            row=row, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=10
        )
        row += 1
        
        ttk.Label(
            container,
            text="Quick Actions:",
            font=("Arial", 11, "bold")
        ).grid(row=row, column=0, columnspan=2, sticky=tk.W, pady=(0, 5))
        row += 1
        
        quick_frame = ttk.Frame(container)
        quick_frame.grid(row=row, column=0, columnspan=2, sticky=tk.W)
        
        ttk.Button(
            quick_frame,
            text="Compile Tất Cả",
            command=lambda: self.compile_all_system()
        ).grid(row=0, column=0, padx=5)
        
        ttk.Button(
            quick_frame,
            text="Clean Tất Cả",
            command=lambda: self.clean_all_system()
        ).grid(row=0, column=1, padx=5)
    
    def setup_smp_tab(self):
        """Thiết lập tab SMP Programming"""
        container = ttk.Frame(self.smp_frame, padding="10")
        container.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        self.smp_frame.columnconfigure(0, weight=1)
        self.smp_frame.rowconfigure(0, weight=1)
        container.columnconfigure(1, weight=1)
        
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
        ttk.Label(
            container,
            text="Các Ví Dụ SMP Programming:",
            font=("Arial", 11, "bold")
        ).grid(row=row, column=0, columnspan=2, sticky=tk.W, pady=(0, 10))
        row += 1
        
        for example_name, example_dir in smp_examples:
            # Label
            ttk.Label(container, text=f"{example_name}:").grid(
                row=row, column=0, sticky=tk.W, pady=5, padx=(0, 10)
            )
            
            # Buttons frame
            btn_frame = ttk.Frame(container)
            btn_frame.grid(row=row, column=1, sticky=tk.W, pady=5)
            
            example_path = self.project_root / "smp-programming" / example_dir
            
            if example_path.exists():
                # Nút compile
                makefile = example_path / "Makefile"
                if makefile.exists():
                    ttk.Button(
                        btn_frame,
                        text="Compile",
                        command=lambda p=example_path: self.compile_c_project(p)
                    ).grid(row=0, column=0, padx=2)
                
                # Tìm executable
                executable = None
                for f in example_path.iterdir():
                    if f.is_file() and os.access(f, os.X_OK) and not f.suffix:
                        executable = f
                        break
                
                if executable:
                    ttk.Button(
                        btn_frame,
                        text="Chạy",
                        command=lambda p=executable: self.run_executable(p)
                    ).grid(row=0, column=1, padx=2)
                
                # Nút README
                readme = example_path / "README.md"
                if readme.exists():
                    ttk.Button(
                        btn_frame,
                        text="README",
                        command=lambda p=readme: self.view_file(p)
                    ).grid(row=0, column=2, padx=2)
            
            row += 1
        
        # Quick actions
        ttk.Separator(container, orient=tk.HORIZONTAL).grid(
            row=row, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=10
        )
        row += 1
        
        ttk.Label(
            container,
            text="Quick Actions:",
            font=("Arial", 11, "bold")
        ).grid(row=row, column=0, columnspan=2, sticky=tk.W, pady=(0, 5))
        row += 1
        
        quick_frame = ttk.Frame(container)
        quick_frame.grid(row=row, column=0, columnspan=2, sticky=tk.W)
        
        ttk.Button(
            quick_frame,
            text="Chạy Demo Tổng Hợp",
            command=lambda: self.run_shell_script(
                self.project_root / "smp-programming" / "demo.sh"
            )
        ).grid(row=0, column=0, padx=5)
        
        ttk.Button(
            quick_frame,
            text="Compile Tất Cả",
            command=lambda: self.compile_all_smp()
        ).grid(row=0, column=1, padx=5)
        
        ttk.Button(
            quick_frame,
            text="Clean Tất Cả",
            command=lambda: self.clean_all_smp()
        ).grid(row=0, column=2, padx=5)
    
    def run_shell_script(self, script_path):
        """Chạy shell script"""
        if not script_path.exists():
            self.log_terminal(f"Lỗi: Không tìm thấy script {script_path}\n")
            return
        
        # Đảm bảo script có quyền thực thi
        os.chmod(script_path, 0o755)
        
        self.log_terminal(f"\n{'='*60}\n")
        self.log_terminal(f"Đang chạy: {script_path.name}\n")
        self.log_terminal(f"Đường dẫn: {script_path}\n")
        self.log_terminal(f"{'='*60}\n\n")
        
        # Chạy trong thread riêng
        thread = threading.Thread(
            target=self._run_command,
            args=(["bash", str(script_path)], script_path.parent),
            daemon=True
        )
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
    
    def compile_c_project(self, project_path):
        """Compile project C với Makefile"""
        makefile = project_path / "Makefile"
        if not makefile.exists():
            self.log_terminal(f"Lỗi: Không tìm thấy Makefile trong {project_path}\n")
            return
        
        self.log_terminal(f"\n{'='*60}\n")
        self.log_terminal(f"Đang compile: {project_path.name}\n")
        self.log_terminal(f"{'='*60}\n\n")
        
        # Chạy make
        thread = threading.Thread(
            target=self._run_command,
            args=(["make"], project_path),
            daemon=True
        )
        thread.start()
    
    def compile_all_system(self):
        """Compile tất cả các module system"""
        self.log_terminal(f"\n{'='*60}\n")
        self.log_terminal("Đang compile tất cả module System Programming...\n")
        self.log_terminal(f"{'='*60}\n\n")
        
        system_dir = self.project_root / "system"
        for module_dir in system_dir.iterdir():
            if module_dir.is_dir() and (module_dir / "Makefile").exists():
                self.log_terminal(f"Compile {module_dir.name}...\n")
                thread = threading.Thread(
                    target=self._run_command,
                    args=(["make"], module_dir),
                    daemon=True
                )
                thread.start()
                thread.join()  # Chờ hoàn thành trước khi compile module tiếp theo
    
    def clean_all_system(self):
        """Clean tất cả các module system"""
        self.log_terminal(f"\n{'='*60}\n")
        self.log_terminal("Đang clean tất cả module System Programming...\n")
        self.log_terminal(f"{'='*60}\n\n")
        
        system_dir = self.project_root / "system"
        for module_dir in system_dir.iterdir():
            if module_dir.is_dir() and (module_dir / "Makefile").exists():
                self.log_terminal(f"Clean {module_dir.name}...\n")
                thread = threading.Thread(
                    target=self._run_command,
                    args=(["make", "clean"], module_dir),
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
                thread = threading.Thread(
                    target=self._run_command,
                    args=(["make"], example_dir),
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
                thread = threading.Thread(
                    target=self._run_command,
                    args=(["make", "clean"], example_dir),
                    daemon=True
                )
                thread.start()
                thread.join()
    
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
    
    def _run_command(self, cmd, cwd):
        """Chạy lệnh trong subprocess với output realtime"""
        with self.process_lock:
            # Dừng process cũ nếu đang chạy
            if self.current_process and self.current_process.poll() is None:
                self.current_process.terminate()
                try:
                    self.current_process.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    self.current_process.kill()
            
            try:
                # Tạo process mới
                self.current_process = subprocess.Popen(
                    cmd,
                    cwd=cwd,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    stdin=subprocess.PIPE,
                    bufsize=1,
                    universal_newlines=True,
                    errors='replace'
                )
            except Exception as e:
                self.output_queue.put(f"Lỗi khi chạy lệnh: {e}\n")
                return
        
        # Đọc output
        try:
            for line in self.current_process.stdout:
                self.output_queue.put(line)
            
            # Chờ process kết thúc
            return_code = self.current_process.wait()
            
            if return_code == 0:
                self.output_queue.put(f"\n[Process hoàn thành thành công]\n")
            else:
                self.output_queue.put(f"\n[Process kết thúc với mã lỗi: {return_code}]\n")
        
        except Exception as e:
            self.output_queue.put(f"\nLỗi khi đọc output: {e}\n")
    
    def send_input(self, event=None):
        """Gửi input đến process đang chạy"""
        input_text = self.terminal_input.get()
        
        # Nếu input trống, chỉ gửi Enter
        if not input_text:
            with self.process_lock:
                if self.current_process and self.current_process.poll() is None:
                    try:
                        self.current_process.stdin.write('\n')
                        self.current_process.stdin.flush()
                        self.log_terminal("\n")
                    except Exception as e:
                        self.log_terminal(f"Lỗi khi gửi Enter: {e}\n")
                else:
                    self.log_terminal("Không có process nào đang chạy\n")
            return
        
        with self.process_lock:
            if self.current_process and self.current_process.poll() is None:
                try:
                    self.current_process.stdin.write(input_text + '\n')
                    self.current_process.stdin.flush()
                    self.log_terminal(f"> {input_text}\n")
                    self.terminal_input.delete(0, tk.END)
                except Exception as e:
                    self.log_terminal(f"Lỗi khi gửi input: {e}\n")
            else:
                self.log_terminal("Không có process nào đang chạy\n")
    
    def send_ctrl_c(self, event=None):
        """Gửi tín hiệu Ctrl+C đến process đang chạy"""
        with self.process_lock:
            if self.current_process and self.current_process.poll() is None:
                try:
                    import signal
                    self.log_terminal("\n[Gửi Ctrl+C...]\n")
                    self.current_process.send_signal(signal.SIGINT)
                except Exception as e:
                    self.log_terminal(f"Lỗi khi gửi Ctrl+C: {e}\n")
            else:
                self.log_terminal("Không có process nào đang chạy\n")
    
    def stop_process(self):
        """Dừng process đang chạy"""
        with self.process_lock:
            if self.current_process and self.current_process.poll() is None:
                self.log_terminal("\n[Đang dừng process...]\n")
                self.current_process.terminate()
                
                try:
                    self.current_process.wait(timeout=2)
                    self.log_terminal("[Process đã dừng]\n")
                except subprocess.TimeoutExpired:
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



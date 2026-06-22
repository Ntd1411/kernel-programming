#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Shell Scripting GUI Launcher
Giao diện đồ họa để thực thi các shell script
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
import subprocess
import threading
import os
import sys
from pathlib import Path
import queue
import time


class ShellScriptGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Shell Scripting Launcher - Ubuntu 24.10")
        self.root.geometry("1200x800")
        
        # Xác định thư mục gốc của script
        self.script_dir = Path(__file__).parent.absolute()
        
        # Queue để xử lý output từ thread
        self.output_queue = queue.Queue()
        
        # Process đang chạy
        self.current_process = None
        self.is_running = False
        
        # Define script parameters
        self.script_params = self.define_script_parameters()
        
        self.setup_ui()
        self.check_output_queue()
    
    def define_script_parameters(self):
        """Định nghĩa parameters cho mỗi script"""
        return {
            "file-management/backup.sh": {
                "params": [
                    {"name": "source_dir", "prompt": "Source directory", "example": "/home/user/Documents", "required": True},
                    {"name": "backup_dir", "prompt": "Backup directory", "example": "/backup", "required": True},
                    {"name": "retention_days", "prompt": "Retention days (optional)", "example": "7", "required": False, "default": "7"}
                ]
            },
            "file-management/find_duplicates.sh": {
                "params": [
                    {"name": "directory", "prompt": "Directory to scan", "example": "/home/user/Documents", "required": True},
                    {"name": "action", "prompt": "Action (list/delete/move, optional)", "example": "list", "required": False, "default": "list"}
                ]
            },
            "file-management/cleanup.sh": {
                "params": [
                    {"name": "days", "prompt": "Days old (optional, use --days flag)", "example": "30", "required": False, "default": ""},
                    {"name": "dry_run", "prompt": "Dry run? (yes/no, optional)", "example": "no", "required": False, "default": "no"},
                ]
            },
            "package-management/package_manager.sh": {
                "params": [
                    {"name": "command", "prompt": "Command (install/remove/search/update/upgrade/list)", "example": "install", "required": True},
                    {"name": "package_name", "prompt": "Package name (required for install/remove/search)", "example": "vim", "required": False, "default": ""}
                ]
            },
            "package-management/dependency_checker.sh": {
                "params": [
                    {"name": "package_name", "prompt": "Package name to check", "example": "vim", "required": True}
                ]
            },
            "time-management/stopwatch.sh": {
                "params": [
                    {"name": "command", "prompt": "Command (start/stop/status)", "example": "start", "required": True},
                    {"name": "name", "prompt": "Stopwatch name (optional for start)", "example": "coding-session", "required": False, "default": "stopwatch"}
                ]
            }
        }
    
    def collect_script_parameters(self, script_path):
        """Thu thập parameters từ người dùng cho script"""
        if script_path not in self.script_params:
            return []
        
        param_defs = self.script_params[script_path]["params"]
        collected_params = []
        
        # Create parameter collection dialog
        dialog = tk.Toplevel(self.root)
        dialog.title(f"Parameters for {script_path}")
        dialog.geometry("600x400")
        dialog.transient(self.root)
        dialog.grab_set()
        
        # Center dialog
        dialog.update_idletasks()
        x = (dialog.winfo_screenwidth() // 2) - (600 // 2)
        y = (dialog.winfo_screenheight() // 2) - (400 // 2)
        dialog.geometry(f"+{x}+{y}")
        
        # Title
        title_frame = ttk.Frame(dialog, padding="10")
        title_frame.pack(fill=tk.X)
        
        title_label = ttk.Label(
            title_frame,
            text=f"Enter parameters for: {script_path}",
            font=("Arial", 12, "bold")
        )
        title_label.pack()
        
        # Parameters frame
        params_frame = ttk.Frame(dialog, padding="10")
        params_frame.pack(fill=tk.BOTH, expand=True)
        
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
            label.grid(row=idx*2, column=0, sticky=tk.W, pady=(5, 0))
            
            # Entry with example placeholder
            entry = ttk.Entry(params_frame, width=50)
            entry.grid(row=idx*2+1, column=0, sticky=(tk.W, tk.E), pady=(0, 10))
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
        
        # Info label
        info_frame = ttk.Frame(dialog, padding="10")
        info_frame.pack(fill=tk.X)
        
        info_label = ttk.Label(
            info_frame,
            text="* Required parameters. Leave optional parameters empty to use default.",
            foreground="blue",
            font=("Arial", 9, "italic")
        )
        info_label.pack()
        
        # Buttons
        button_frame = ttk.Frame(dialog, padding="10")
        button_frame.pack(fill=tk.X)
        
        result = {"ok": False, "params": []}
        
        def on_ok():
            params = []
            error_msgs = []
            
            for param_def, entry in entries:
                value = entry.get()
                
                # Check if it's still the placeholder
                if value == param_def.get('example', ''):
                    value = ""
                
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
                    elif 'days' in param_def['name'] and script_path == 'file-management/cleanup.sh':
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
        
        ok_btn = ttk.Button(button_frame, text="OK", command=on_ok, width=15)
        ok_btn.pack(side=tk.LEFT, padx=5)
        
        cancel_btn = ttk.Button(button_frame, text="Cancel", command=on_cancel, width=15)
        cancel_btn.pack(side=tk.LEFT, padx=5)
        
        # Wait for dialog to close
        self.root.wait_window(dialog)
        
        return result["params"] if result["ok"] else None
        
    def setup_ui(self):
        """Thiết lập giao diện người dùng"""
        
        # Main container
        main_container = ttk.Frame(self.root, padding="10")
        main_container.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Configure grid weights
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_container.columnconfigure(0, weight=1)
        main_container.rowconfigure(1, weight=1)
        
        # Title
        title_label = ttk.Label(
            main_container, 
            text="Shell Scripting Launcher",
            font=("Arial", 18, "bold")
        )
        title_label.grid(row=0, column=0, pady=(0, 10), sticky=tk.W)
        
        # Buttons area
        buttons_frame = ttk.LabelFrame(main_container, text="Scripts", padding="10")
        buttons_frame.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=(0, 10))
        main_container.rowconfigure(1, weight=0)
        
        # Create notebook for categories
        self.notebook = ttk.Notebook(buttons_frame)
        self.notebook.pack(fill=tk.BOTH, expand=True)
        
        # Define script categories
        self.create_demo_tab()
        self.create_file_management_tab()
        self.create_time_management_tab()
        self.create_package_management_tab()
        self.create_task_scheduler_tab()
        
        # Output area
        output_frame = ttk.LabelFrame(main_container, text="Output (Kết quả thực thi)", padding="10")
        output_frame.grid(row=2, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        main_container.rowconfigure(2, weight=1)
        
        # Text output with scrollbar
        self.output_text = scrolledtext.ScrolledText(
            output_frame,
            wrap=tk.WORD,
            width=100,
            height=25,
            font=("Courier", 10),
            bg="#1e1e1e",
            fg="#d4d4d4",
            insertbackground="white"
        )
        self.output_text.pack(fill=tk.BOTH, expand=True)
        
        # Bind Enter key for interactive input
        self.output_text.bind('<Return>', self.handle_input)
        self.input_start_mark = None
        
        # Configure text tags for colored output
        self.output_text.tag_config("stdout", foreground="#4ec9b0")
        self.output_text.tag_config("stderr", foreground="#f48771")
        self.output_text.tag_config("success", foreground="#4ec9b0", font=("Courier", 10, "bold"))
        self.output_text.tag_config("error", foreground="#f48771", font=("Courier", 10, "bold"))
        self.output_text.tag_config("info", foreground="#569cd6", font=("Courier", 10, "bold"))
        
        # Control buttons
        control_frame = ttk.Frame(main_container)
        control_frame.grid(row=3, column=0, pady=(10, 0))
        
        self.clear_btn = ttk.Button(
            control_frame,
            text="Clear Output",
            command=self.clear_output
        )
        self.clear_btn.pack(side=tk.LEFT, padx=5)
        
        self.stop_btn = ttk.Button(
            control_frame,
            text="Stop Execution",
            command=self.stop_execution,
            state=tk.DISABLED
        )
        self.stop_btn.pack(side=tk.LEFT, padx=5)
        
        # Status bar
        self.status_var = tk.StringVar(value="Ready")
        status_bar = ttk.Label(
            main_container,
            textvariable=self.status_var,
            relief=tk.SUNKEN,
            anchor=tk.W
        )
        status_bar.grid(row=4, column=0, sticky=(tk.W, tk.E), pady=(10, 0))
        
    def create_demo_tab(self):
        """Tab cho demo scripts"""
        frame = ttk.Frame(self.notebook, padding="10")
        self.notebook.add(frame, text="Demo")
        
        scripts = [
            ("Main Demo (Menu)", "demo.sh", "Chạy menu demo tổng hợp"),
            ("Quick Test", "quick_test.sh", "Kiểm tra nhanh các chức năng"),
            ("File Management Demo", "demo/demo-file-management.sh", "Demo quản lý file"),
            ("Time Management Demo", "demo/demo-time-management.sh", "Demo quản lý thời gian"),
            ("Package Management Demo", "demo/demo-package-management.sh", "Demo quản lý package"),
            ("Task Scheduler Demo", "demo/demo-task-scheduler.sh", "Demo lập lịch tác vụ"),
            ("Advanced Workflows", "demo/demo-advanced.sh", "Demo các workflow nâng cao"),
        ]
        
        self.create_script_buttons(frame, scripts)
        
    def create_file_management_tab(self):
        """Tab cho file management scripts"""
        frame = ttk.Frame(self.notebook, padding="10")
        self.notebook.add(frame, text="File Management")
        
        scripts = [
            ("File Manager", "file-management/file_manager.sh", "Quản lý file và thư mục"),
            ("Backup", "file-management/backup.sh", "Sao lưu file tự động"),
            ("Find Duplicates", "file-management/find_duplicates.sh", "Tìm file trùng lặp"),
            ("Cleanup", "file-management/cleanup.sh", "Dọn dẹp file tạm"),
        ]
        
        self.create_script_buttons(frame, scripts)
        
    def create_time_management_tab(self):
        """Tab cho time management scripts"""
        frame = ttk.Frame(self.notebook, padding="10")
        self.notebook.add(frame, text="Time Management")
        
        scripts = [
            ("Time Tracker", "time-management/time_tracker.sh", "Theo dõi thời gian làm việc"),
            ("Stopwatch", "time-management/stopwatch.sh", "Đồng hồ bấm giờ"),
        ]
        
        self.create_script_buttons(frame, scripts)
        
    def create_package_management_tab(self):
        """Tab cho package management scripts"""
        frame = ttk.Frame(self.notebook, padding="10")
        self.notebook.add(frame, text="Package Management")
        
        scripts = [
            ("Package Manager", "package-management/package_manager.sh", "Quản lý cài đặt/gỡ package"),
            ("Dependency Checker", "package-management/dependency_checker.sh", "Kiểm tra dependencies"),
            ("Repo Manager", "package-management/repo_manager.sh", "Quản lý repository"),
        ]
        
        self.create_script_buttons(frame, scripts)
        
    def create_task_scheduler_tab(self):
        """Tab cho task scheduler scripts"""
        frame = ttk.Frame(self.notebook, padding="10")
        self.notebook.add(frame, text="Task Scheduler")
        
        scripts = [
            ("Cron Manager", "task-scheduler/cron_manager.sh", "Quản lý cron jobs"),
            ("Scheduled Tasks", "task-scheduler/scheduled_tasks.sh", "Các tác vụ định kỳ"),
        ]
        
        self.create_script_buttons(frame, scripts)
        
    def create_script_buttons(self, parent, scripts):
        """Tạo các nút cho scripts"""
        for idx, (name, script_path, description) in enumerate(scripts):
            btn_frame = ttk.Frame(parent)
            btn_frame.pack(fill=tk.X, pady=5)
            
            btn = ttk.Button(
                btn_frame,
                text=f"{name}",
                command=lambda sp=script_path, n=name: self.run_script(sp, n),
                width=30
            )
            btn.pack(side=tk.LEFT, padx=(0, 10))
            
            desc_label = ttk.Label(btn_frame, text=description, foreground="gray")
            desc_label.pack(side=tk.LEFT)
            
    def run_script(self, script_path, script_name):
        """Chạy script trong thread riêng"""
        if self.is_running:
            messagebox.showwarning("Warning", "Một script đang chạy. Vui lòng đợi hoặc dừng nó trước.")
            return
            
        full_path = self.script_dir / script_path
        
        if not full_path.exists():
            self.append_output(f"ERROR: Script không tồn tại: {full_path}\n", "error")
            return
        
        # Check if script needs parameters
        script_params = []
        if script_path in self.script_params:
            # Collect parameters from user
            script_params = self.collect_script_parameters(script_path)
            
            # User cancelled parameter input
            if script_params is None:
                self.append_output(f"Cancelled: {script_name}\n", "error")
                return
            
        self.clear_output()
        self.append_output(f"{'='*80}\n", "info")
        self.append_output(f"Đang chạy: {script_name}\n", "info")
        self.append_output(f"Path: {full_path}\n", "info")
        if script_params:
            self.append_output(f"Parameters: {' '.join(script_params)}\n", "info")
        self.append_output(f"{'='*80}\n\n", "info")
        
        self.is_running = True
        self.stop_btn.config(state=tk.NORMAL)
        self.status_var.set(f"Running: {script_name}")
        
        # Chạy script trong thread riêng
        thread = threading.Thread(
            target=self._execute_script,
            args=(full_path, script_name, script_params),
            daemon=True
        )
        thread.start()
        
    def _execute_script(self, script_path, script_name, script_params=None):
        """Thực thi script và capture output"""
        try:
            # Make script executable
            os.chmod(script_path, 0o755)
            
            # Build command with parameters
            command = ['bash', str(script_path)]
            if script_params:
                command.extend(script_params)
            
            # Run script
            self.current_process = subprocess.Popen(
                command,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                stdin=subprocess.PIPE,
                text=True,
                bufsize=1,
                universal_newlines=True
            )
            
            # Read output in real-time
            while True:
                output = self.current_process.stdout.readline()
                if output:
                    self.output_queue.put(('stdout', output))
                    
                error = self.current_process.stderr.readline()
                if error:
                    self.output_queue.put(('stderr', error))
                    
                # Check if process finished
                if output == '' and error == '' and self.current_process.poll() is not None:
                    break
                    
            # Get return code
            return_code = self.current_process.wait()
            
            self.output_queue.put(('info', f"\n{'='*80}\n"))
            if return_code == 0:
                self.output_queue.put(('success', f"Script hoàn thành thành công!\n"))
            else:
                self.output_queue.put(('error', f"Script kết thúc với lỗi (code: {return_code})\n"))
            self.output_queue.put(('info', f"{'='*80}\n"))
            
        except Exception as e:
            self.output_queue.put(('error', f"\nLỖI: {str(e)}\n"))
            
        finally:
            self.is_running = False
            self.current_process = None
            self.output_queue.put(('status', 'Ready'))
            self.output_queue.put(('button', 'enable_stop'))
            
    def check_output_queue(self):
        """Kiểm tra queue và cập nhật UI"""
        try:
            while True:
                msg_type, content = self.output_queue.get_nowait()
                
                if msg_type == 'status':
                    self.status_var.set(content)
                elif msg_type == 'button':
                    if content == 'enable_stop':
                        self.stop_btn.config(state=tk.DISABLED)
                else:
                    self.append_output(content, msg_type)
                    
        except queue.Empty:
            pass
        finally:
            self.root.after(100, self.check_output_queue)
            
    def handle_input(self, event):
        """Xử lý input từ người dùng khi nhấn Enter"""
        if not self.is_running or not self.current_process:
            return "break"
        
        # Lấy dòng hiện tại
        current_line = self.output_text.get("insert linestart", "insert lineend")
        
        # Gửi input đến process
        try:
            if self.current_process and self.current_process.stdin:
                self.current_process.stdin.write(current_line + "\n")
                self.current_process.stdin.flush()
        except Exception as e:
            self.append_output(f"\nLỗi khi gửi input: {str(e)}\n", "error")
        
        return "break"  # Ngăn chặn xử lý mặc định
            
    def append_output(self, text, tag="stdout"):
        """Thêm text vào output area"""
        self.output_text.insert(tk.END, text, tag)
        self.output_text.see(tk.END)
        
    def clear_output(self):
        """Xóa output area"""
        self.output_text.delete(1.0, tk.END)
        
    def stop_execution(self):
        """Dừng script đang chạy"""
        if self.current_process and self.is_running:
            try:
                self.current_process.terminate()
                self.append_output("\nScript đã bị dừng bởi người dùng.\n", "error")
                self.is_running = False
                self.stop_btn.config(state=tk.DISABLED)
                self.status_var.set("Stopped")
            except Exception as e:
                self.append_output(f"\nLỗi khi dừng script: {str(e)}\n", "error")


def main():
    """Main entry point"""
    root = tk.Tk()
    app = ShellScriptGUI(root)
    root.mainloop()


if __name__ == "__main__":
    main()

import subprocess
import tkinter as tk
from tkinter import messagebox, ttk
import os
import sys
from datetime import datetime
import re

def format_git_date(git_date_str):
    """将Git日期格式转换为更友好的格式"""
    try:
        # Git日期格式: "Sun Aug 17 07:17:29 2025 +0800"
        # 使用正则表达式解析
        pattern = r'(\w+)\s+(\w+)\s+(\d+)\s+(\d+):(\d+):(\d+)\s+(\d+)\s+([+-]\d{4})'
        match = re.match(pattern, git_date_str.strip())
        
        if not match:
            return git_date_str  # 如果解析失败，返回原始字符串
        
        weekday, month, day, hour, minute, second, year, timezone = match.groups()
        
        # 英文月份到数字的映射
        month_map = {
            'Jan': 1, 'Feb': 2, 'Mar': 3, 'Apr': 4, 'May': 5, 'Jun': 6,
            'Jul': 7, 'Aug': 8, 'Sep': 9, 'Oct': 10, 'Nov': 11, 'Dec': 12
        }
        
        # 英文星期到中文的映射
        weekday_map = {
            'Mon': '周一', 'Tue': '周二', 'Wed': '周三', 'Thu': '周四',
            'Fri': '周五', 'Sat': '周六', 'Sun': '周日'
        }
        
        # 格式化时区
        tz_sign = timezone[0]
        tz_hours = timezone[1:3]
        tz_minutes = timezone[3:5]
        if tz_minutes == '00':
            tz_formatted = f"UTC{tz_sign}{int(tz_hours)}"
        else:
            tz_formatted = f"UTC{tz_sign}{int(tz_hours)}:{tz_minutes}"
        
        # 构建格式化的日期字符串
        formatted_date = f"{year}-{month_map[month]:02d}-{int(day):02d} {hour}:{minute}:{second} ({tz_formatted}) {weekday_map[weekday]}"
        
        return formatted_date
        
    except Exception:
        return git_date_str  # 如果出现任何错误，返回原始字符串

def format_local_date(timestamp):
    """将本机时间戳格式化为友好格式，包含时区信息"""
    try:
        import time
        dt = datetime.fromtimestamp(timestamp)
        weekday_map = ['周一', '周二', '周三', '周四', '周五', '周六', '周日']
        weekday = weekday_map[dt.weekday()]
        
        # 获取时区信息
        if time.daylight:
            # 夏令时
            timezone_name = time.tzname[1]
            timezone_offset = time.altzone
        else:
            # 标准时间
            timezone_name = time.tzname[0]
            timezone_offset = time.timezone
        
        # 计算时区偏移（小时）
        offset_hours = -timezone_offset // 3600
        offset_minutes = (-timezone_offset % 3600) // 60
        
        if offset_minutes == 0:
            timezone_str = f"UTC{offset_hours:+d}"
        else:
            timezone_str = f"UTC{offset_hours:+d}:{offset_minutes:02d}"
        
        return f"{dt.strftime('%Y-%m-%d %H:%M:%S')} ({timezone_str}) {weekday}"
    except Exception:
        return "获取失败"

def get_local_file_dates(filepath):
    """获取本机文件的创建和修改日期"""
    try:
        stat = os.stat(filepath)
        # Windows系统中，st_ctime是创建时间，st_mtime是修改时间
        creation_time = stat.st_ctime
        modification_time = stat.st_mtime
        
        return {
            'creation': format_local_date(creation_time),
            'modification': format_local_date(modification_time)
        }
    except Exception:
        return {
            'creation': "获取失败",
            'modification': "获取失败"
        }

def get_git_creation_date(filepath):
    """获取文件在Git中的创建日期（包括改名跟踪）"""
    try:
        # 确保路径是相对仓库的
        repo_root = subprocess.check_output(
            ["git", "rev-parse", "--show-toplevel"],
            stderr=subprocess.DEVNULL
        ).decode("utf-8").strip()

        rel_path = os.path.relpath(filepath, repo_root)

        # 查找第一次提交日期
        result = subprocess.check_output(
            ["git", "log", "--follow", "--diff-filter=A", "--format=%ad", "--", rel_path],
            stderr=subprocess.DEVNULL
        ).decode("utf-8").strip()

        if not result:
            return "未找到提交记录（可能未纳入git）"

        # 取最后一行（最早的提交）
        raw_date = result.splitlines()[-1]
        return format_git_date(raw_date)

    except subprocess.CalledProcessError:
        return "不是Git仓库或文件不在仓库中"

def get_git_last_modified_date(filepath):
    """获取文件在Git中的最后修改日期"""
    try:
        # 确保路径是相对仓库的
        repo_root = subprocess.check_output(
            ["git", "rev-parse", "--show-toplevel"],
            stderr=subprocess.DEVNULL
        ).decode("utf-8").strip()

        rel_path = os.path.relpath(filepath, repo_root)

        # 查找最后一次修改日期
        result = subprocess.check_output(
            ["git", "log", "-1", "--format=%ad", "--", rel_path],
            stderr=subprocess.DEVNULL
        ).decode("utf-8").strip()

        if not result:
            return "未找到提交记录（可能未纳入git）"

        return format_git_date(result)

    except subprocess.CalledProcessError:
        return "不是Git仓库或文件不在仓库中"

def get_formatted_date_info(filepath):
    """获取格式化的创建和更新日期信息"""
    creation_date = get_git_creation_date(filepath)
    modified_date = get_git_last_modified_date(filepath)
    
    # 按照指定格式生成字符串
    formatted_info = f" * @date {creation_date}\n * @change {modified_date}"
    return formatted_info

class DateInfoApp:
    def __init__(self, root):
        self.root = root
        self.current_filepath = None
        self.date_vars = {}
        self.date_info = {}
        
        # 创建界面
        self.create_widgets()
        
    def create_widgets(self):
        # 主框架
        main_frame = tk.Frame(self.root, bg="lightgray")
        main_frame.pack(fill="both", expand=True, padx=10, pady=10)
        
        # 拖拽提示标签
        self.drop_label = tk.Label(main_frame, 
                                  text="请拖拽一个文件到此窗口\n（支持VSCode直接拖拽）", 
                                  font=("微软雅黑", 14), 
                                  bg="lightgray",
                                  wraplength=400)
        self.drop_label.pack(expand=True)
        
        # 文件信息框架（初始隐藏）
        self.info_frame = tk.Frame(main_frame, bg="white", relief="raised", bd=2)
        
        # 文件路径标签
        self.path_label = tk.Label(self.info_frame, 
                                  text="", 
                                  font=("微软雅黑", 10), 
                                  wraplength=580, 
                                  justify="left",
                                  bg="white")
        self.path_label.pack(pady=10, padx=10, fill="x")
        
        # 日期信息框架
        self.dates_frame = tk.Frame(self.info_frame, bg="white")
        self.dates_frame.pack(pady=10, padx=10, fill="x")
        
        # 创建四个日期的复选框
        date_types = [
            ("git_creation", "Git 创建日期"),
            ("local_creation", "本机创建日期"),
            ("git_modification", "Git 修改日期"),
            ("local_modification", "本机修改日期")
        ]
        
        for i, (key, label) in enumerate(date_types):
            frame = tk.Frame(self.dates_frame, bg="white")
            frame.pack(fill="x", pady=2)
            
            var = tk.BooleanVar()
            self.date_vars[key] = var
            
            checkbox = tk.Checkbutton(frame, 
                                      variable=var, 
                                      text=label,
                                      font=("微软雅黑", 10),
                                      bg="white",
                                      anchor="w")
            checkbox.pack(side="left")
            
            # 日期显示标签
            date_label = tk.Label(frame, 
                                text="", 
                                font=("微软雅黑", 10),
                                bg="white",
                                fg="blue")
            date_label.pack(side="left", padx=(10, 0))
            setattr(self, f"{key}_label", date_label)
        
        # 按钮框架
        button_frame = tk.Frame(self.info_frame, bg="white")
        button_frame.pack(pady=15)
        
        # 复制按钮
        self.copy_button = tk.Button(button_frame, 
                                   text="复制选中的日期", 
                                   command=self.copy_selected_dates,
                                   font=("微软雅黑", 11),
                                   bg="#4CAF50",
                                   fg="white",
                                   relief="flat",
                                   padx=20)
        self.copy_button.pack(side="left", padx=5)
        
        # 复制格式化信息按钮
        self.copy_formatted_button = tk.Button(button_frame, 
                                             text="复制含[@date @change]的日期", 
                                             command=self.copy_formatted_info,
                                             font=("微软雅黑", 11),
                                             bg="#2196F3",
                                             fg="white",
                                             relief="flat",
                                             padx=20)
        self.copy_formatted_button.pack(side="left", padx=5)
        
        # 清除按钮
        self.clear_button = tk.Button(button_frame, 
                                    text="清除", 
                                    command=self.clear_info,
                                    font=("微软雅黑", 11),
                                    bg="#f44336",
                                    fg="white",
                                    relief="flat",
                                    padx=20)
        self.clear_button.pack(side="left", padx=5)
        
        # 状态标签
        self.status_label = tk.Label(self.info_frame, 
                                   text="", 
                                   font=("微软雅黑", 9),
                                   bg="white",
                                   fg="green")
        self.status_label.pack(pady=5)
        
    def parse_vscode_path(self, raw_path):
        """专门处理VSCode拖拽的路径格式"""
        import urllib.parse
        
        # 移除各种可能的包装字符
        path = raw_path.strip('{}').strip('"').strip("'").strip()
        
        # 处理VSCode的URI格式
        if path.startswith('file:///'):
            # 移除 file:/// 前缀并解码URL编码
            path = urllib.parse.unquote(path[8:])
        elif path.startswith('file://'):
            # 移除 file:// 前缀并解码URL编码
            path = urllib.parse.unquote(path[7:])
        elif path.startswith('vscode-file://'):
            # VSCode特有的文件协议
            path = urllib.parse.unquote(path[14:])
        
        # 处理Windows路径格式
        path = path.replace('/', '\\')
        
        # 处理可能的编码问题
        try:
            # 尝试UTF-8解码
            if isinstance(path, bytes):
                path = path.decode('utf-8')
        except:
            pass
        
        return path

    def on_drop(self, event):
        # 处理多种拖拽数据格式
        data = event.data
        filepath = None
        
        print(f"拖拽事件触发，数据类型: {type(data)}, 数据内容: {repr(data)}")
        
        # 尝试多种数据解析方式
        try:
            # 方式1: 直接处理字符串
            if isinstance(data, str):
                # 处理多行文本（VSCode可能发送多行）
                lines = data.strip().split('\n')
                for line in lines:
                    line = line.strip()
                    if line:
                        potential_path = self.parse_vscode_path(line)
                        
                        # 检查是否是有效的文件路径
                        if os.path.isfile(potential_path):
                            filepath = potential_path
                            break
                
                # 如果没有找到有效路径，尝试整个字符串
                if not filepath:
                    filepath = self.parse_vscode_path(data)
                
            # 方式2: 如果是列表或元组，取第一个
            elif isinstance(data, (list, tuple)) and len(data) > 0:
                filepath = self.parse_vscode_path(str(data[0]))
                
            # 方式3: 尝试直接转换为字符串
            else:
                filepath = self.parse_vscode_path(str(data))
                
        except Exception as e:
            print(f"拖拽数据解析错误: {e}, 原始数据: {data}")
            
        # 验证文件路径
        if not filepath:
            messagebox.showerror("错误", f"无法解析拖拽的文件路径\n原始数据: {repr(data)}")
            return

        # 用当前系统的分隔符重新拼接路径
        filepath = os.path.normpath(filepath)
        print(f"解析后的文件路径: \"{filepath}\"")
        
        if not os.path.isfile(filepath):
            # 尝试相对路径
            if not os.path.isabs(filepath):
                abs_filepath = os.path.abspath(filepath)
                if os.path.isfile(abs_filepath):
                    filepath = abs_filepath
                else:
                    messagebox.showerror("错误", f"无效的文件路径: {filepath}\n绝对路径: {abs_filepath}\n原始数据: {repr(data)}")
                    return
            else:
                messagebox.showerror("错误", f"无效的文件路径: {filepath}\n原始数据: {repr(data)}")
                return
        
        self.load_file_info(filepath)
        
    def load_file_info(self, filepath):
        self.current_filepath = filepath
        
        # 显示文件路径
        self.path_label.config(text=f"文件: {filepath}")
        
        # 获取所有日期信息
        git_creation = get_git_creation_date(filepath)
        git_modification = get_git_last_modified_date(filepath)
        local_dates = get_local_file_dates(filepath)
        
        self.date_info = {
            "git_creation": git_creation,
            "git_modification": git_modification,
            "local_creation": local_dates['creation'],
            "local_modification": local_dates['modification']
        }
        
        # 更新日期显示
        self.git_creation_label.config(text=git_creation)
        self.git_modification_label.config(text=git_modification)
        self.local_creation_label.config(text=local_dates['creation'])
        self.local_modification_label.config(text=local_dates['modification'])
        
        # 智能默认选中逻辑
        # 检查Git日期是否有效（不包含"未找到"前缀或空字符串）
        git_creation_valid = git_creation and "未找到" not in git_creation and git_creation.strip()
        git_modification_valid = git_modification and "未找到" not in git_modification and git_modification.strip()
        
        # 重置所有复选框
        for var in self.date_vars.values():
            var.set(False)
        
        if git_creation_valid or git_modification_valid:
            # 如果Git日期存在，优先选择Git日期
            if git_creation_valid:
                self.date_vars["git_creation"].set(True)
            else:
                self.date_vars["local_creation"].set(True)
            if git_modification_valid:
                self.date_vars["git_modification"].set(True)
            else:
                self.date_vars["local_modification"].set(True)
        else:
            # 如果Git日期不存在，默认选择本机日期
            self.date_vars["local_creation"].set(True)
            self.date_vars["local_modification"].set(True)
        
        # 隐藏拖拽提示，显示信息框架
        self.drop_label.pack_forget()
        self.info_frame.pack(fill="both", expand=True)
        
        # 自动复制选中的日期
        self.copy_selected_dates(auto=True)
        
    def copy_selected_dates(self, auto=False):
        selected_dates = []
        for key, var in self.date_vars.items():
            if var.get() and key in self.date_info:
                date_names = {
                    "git_creation": "Git 创建日期",
                    "local_creation": "本机创建日期",
                    "git_modification": "Git 修改日期", 
                    "local_modification": "本机修改日期"
                }
                selected_dates.append(f"{date_names[key]}: {self.date_info[key]}")
        
        status_text = "\n可继续拖拽文件"
        if selected_dates:
            content = "\n".join(selected_dates)
            try:
                self.root.clipboard_clear()
                self.root.clipboard_append(content)
                self.root.update()
                if auto:
                    self.status_label.config(text="✓ 已自动复制选中的日期到剪贴板"+status_text, fg="green")
                else:
                    self.status_label.config(text="✓ 已复制选中的日期到剪贴板"+status_text, fg="green")
            except Exception as e:
                self.status_label.config(text=f"✗ 复制失败: {str(e)}"+status_text, fg="red")
        else:
            self.status_label.config(text="✗ 请至少选择一个日期"+status_text, fg="red")
    
    def copy_formatted_info(self):
        if not self.current_filepath:
            return
            
        try:
            formatted_info = get_formatted_date_info(self.current_filepath)
            self.root.clipboard_clear()
            self.root.clipboard_append(formatted_info)
            self.root.update()
            self.status_label.config(text="✓ 已复制格式化日期信息到剪贴板", fg="green")
        except Exception as e:
            self.status_label.config(text=f"✗ 复制失败: {str(e)}", fg="red")
    
    def clear_info(self):
        # 隐藏信息框架，显示拖拽提示
        self.info_frame.pack_forget()
        self.drop_label.pack(expand=True)
        self.current_filepath = None
        self.date_info = {}

def on_drop(event):
    """全局拖拽处理函数，用于兼容原有代码"""
    if hasattr(event.widget, 'master') and hasattr(event.widget.master, 'app'):
        event.widget.master.app.on_drop(event)
    else:
        # 查找app实例
        widget = event.widget
        while widget:
            if hasattr(widget, 'app'):
                widget.app.on_drop(event)
                break
            widget = widget.master

def main():
    # 启用拖拽
    try:
        from tkinterdnd2 import DND_FILES, DND_TEXT, TkinterDnD
        
        root = TkinterDnD.Tk()
        root.title("Git 文件日期查询工具")
        root.geometry("700x500")
        root.attributes("-topmost", True)  # 设置窗口置顶
        root.resizable(True, True)
        
        # 创建应用实例
        app = DateInfoApp(root)
        root.app = app  # 将app实例绑定到root，方便拖拽处理
        
        # 定义拖拽处理函数
        def handle_drop(event):
            print(f"拖拽事件触发，数据类型: {type(event.data)}, 数据内容: {repr(event.data)}")
            app.on_drop(event)
        
        # 为所有子组件递归注册拖拽
        def register_drop_target(widget):
            try:
                # 注册多种拖拽类型
                widget.drop_target_register(DND_FILES)
                widget.drop_target_register(DND_TEXT)
                widget.drop_target_register('DND_CF_HDROP')  # Windows文件拖拽
                widget.drop_target_register('CF_HDROP')      # Windows剪贴板格式
                widget.drop_target_register('text/uri-list') # URI列表格式
                widget.drop_target_register('text/plain')    # 纯文本格式
                
                widget.dnd_bind('<<Drop>>', handle_drop)
                # widget.dnd_bind('<<DropEnter>>', lambda e: print("拖拽进入"))
                # widget.dnd_bind('<<DropLeave>>', lambda e: print("拖拽离开"))
            except Exception as e:
                print(f"注册拖拽目标失败: {e}")
            
            # 递归处理子控件
            try:
                for child in widget.winfo_children():
                    register_drop_target(child)
            except:
                pass
        
        # 延迟注册子控件的拖拽，确保界面已经创建完成
        def delayed_register():
            register_drop_target(root)
            
        root.after(100, delayed_register)
        
        # 额外的事件绑定
        def on_drag_data(event):
            print(f"拖拽数据事件: {event}")
            if hasattr(event, 'data'):
                app.on_drop(event)
        
        # 绑定更多可能的拖拽事件
        root.bind('<Button-1>', lambda e: root.focus_set())  # 确保窗口获得焦点
        
        # 添加调试信息
        print("Git 文件日期查询工具已启动")
        
        root.mainloop()
        
    except ImportError:
        messagebox.showerror(
            "缺少依赖",
            "需要安装 tkinterdnd2 库\n请运行: pip install tkinterdnd2"
        )
        sys.exit(1)

if __name__ == "__main__":
    main()

"""C/C++ 源文件头部注释块一致性检查器。

@date 2025-10-05 22:05:43(UTC+8) 周日
@change 2025-10-05 23:32:12(UTC+8) 周日

强制执行的规则（非致命项目可以稍后扩展）：
 1. 第一个非空、非shebang行必须以Doxygen风格块开始：/** ... */
 2. 块必须包含：@file <确切文件名>、@brief <非空>、@date (YYYY-MM-DD ...)
 3. @file 文件名必须与真实文件名匹配（*nix上区分大小写，Windows默认不区分大小写）
 4. 可选标签：@author、@change（如果存在应有日期前缀）
 5. Brief行不应仅重复文件名或为空。

退出代码：
 0 = 所有文件通过检查
 1 = 一个或多个违规（打印到stdout/stderr）

可选的 --fix 参数将：
  * 如果缺少头部注释，插入最小的合规头部
  * 如果不匹配，纠正@file行
  * 为缺少的@brief添加占位符
  * 在插入的头部下方保留原始内容

该脚本故意保守 - 它不会尝试重新排版或重写现有的丰富描述。

使用方式：
  * 单次执行（推荐在项目根）
        cmake --build build --target check_headers
        cmake --build build --target fix_headers
  * 或直接调用：
        python tools/检查文件头/check_headers.py --root .
        python tools/检查文件头/check_headers.py --root . --fix
"""
from __future__ import annotations

import argparse
import os
import re
import sys
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import List, Optional

HEADER_RE = re.compile(r"^\s*/\*\*(?P<body>.*?)\*/", re.DOTALL)
TAG_RE = re.compile(r"@(?P<tag>file|brief|date|change|author)\b\s*(?P<value>.*)")
DATE_PREFIX_RE = re.compile(r"\d{4}-\d{2}-\d{2}")
# 匹配包含周数的完整日期格式：YYYY-MM-DD HH:MM:SS (UTC+8) 周X
DATE_WITH_WEEKDAY_RE = re.compile(r"(?P<date>\d{4}-\d{2}-\d{2})\s+(?P<time>\d{2}:\d{2}:\d{2})\s*\(UTC\+8\)\s*周(?P<weekday>[一二三四五六日])")

DEFAULT_AUTHOR = "Sakurakugu"

EXCLUDE_DIR_NAMES = {"build", "_deps", ".git", ".idea", ".vscode"} # 排除的目录名（不区分大小写）
FILE_EXT = {".h", ".hpp", ".hh", ".cpp", ".cxx", ".cc", ".c"} # 支持的文件扩展名（不区分大小写）


@dataclass
class Violation:
    file: Path
    code: str
    message: str
    fix_applied: bool = False


@dataclass
class WeekdayValidationResult:
    """周数验证结果"""
    is_valid: bool
    error_message: Optional[str] = None
    correct_weekday: Optional[str] = None
    declared_weekday: Optional[str] = None
    corrected_date_str: Optional[str] = None


def validate_date_weekday(date_str: str) -> WeekdayValidationResult:
    """验证日期字符串中的周数是否正确。
    
    Args:
        date_str: 包含日期和周数的字符串，格式如 "2025-09-15 20:55:22 (UTC+8) 周一"
        
    Returns:
        WeekdayValidationResult: 包含验证结果和修复信息的对象
    """
    match = DATE_WITH_WEEKDAY_RE.search(date_str)
    if not match:
        return WeekdayValidationResult(is_valid=True)  # 没有找到完整的日期+周数格式，认为有效
    
    date_part = match.group("date")
    declared_weekday = match.group("weekday")
    
    try:
        # 解析日期
        date_obj = datetime.strptime(date_part, "%Y-%m-%d")
        
        # 获取实际的星期几 (0=周一, 6=周日)
        actual_weekday_num = date_obj.weekday()
        
        # 中文周数映射
        weekday_map = {
            0: "一",  # 周一
            1: "二",  # 周二  
            2: "三",  # 周三
            3: "四",  # 周四
            4: "五",  # 周五
            5: "六",  # 周六
            6: "日"   # 周日
        }
        
        expected_weekday = weekday_map[actual_weekday_num]
        
        if declared_weekday != expected_weekday:
            # 生成修正后的日期字符串
            corrected_str = DATE_WITH_WEEKDAY_RE.sub(
                lambda m: f"{m.group('date')} {m.group('time')} (UTC+8) 周{expected_weekday}",
                date_str
            )
            
            return WeekdayValidationResult(
                is_valid=False,
                error_message=f"日期 {date_part} 应该是周{expected_weekday}，但标记为周{declared_weekday}",
                correct_weekday=expected_weekday,
                declared_weekday=declared_weekday,
                corrected_date_str=corrected_str
            )
        else:
            return WeekdayValidationResult(is_valid=True)
            
    except ValueError:
        return WeekdayValidationResult(
            is_valid=False,
            error_message=f"无法解析日期: {date_part}"
        )


def is_excluded(path: Path) -> bool:
    parts = set(p.lower() for p in path.parts)
    return any(d in parts for d in EXCLUDE_DIR_NAMES)


def scan_file(path: Path, fix: bool) -> List[Violation]:
    violations: List[Violation] = []
    try:
        content = path.read_text(encoding="utf-8")
    except Exception as e:  # pragma: no cover - IO 失败
        return [Violation(path, "io_error", f"无法读取文件: {e}")]

    m = HEADER_RE.search(content)
    if not m:
        if fix:
            new_header = build_header_template(path.name)
            path.write_text(new_header + content, encoding="utf-8")
            violations.append(Violation(path, "missing_header", "缺少头部注释，已自动插入", True))
        else:
            violations.append(Violation(path, "missing_header", "缺少头部注释块"))
        return violations

    header_body = m.group("body")
    tags = {match.group("tag"): match.group("value").strip() for match in TAG_RE.finditer(header_body)}

    # @file 检查
    actual = path.name
    file_tag = tags.get("file")
    if not file_tag:
        if fix:
            new_body = insert_or_replace_tag(header_body, "file", actual)
            content = content.replace(header_body, new_body)
            path.write_text(content, encoding="utf-8")
            violations.append(Violation(path, "missing_file_tag", "缺少@file，已补全", True))
        else:
            violations.append(Violation(path, "missing_file_tag", "缺少@file 标签"))
    else:
        # 只比较@file后的第一个标记（允许尾随注释）
        declared = file_tag.split()[0]
        if declared != actual:
            if fix:
                new_body = replace_file_name(header_body, declared, actual)
                content = content.replace(header_body, new_body)
                path.write_text(content, encoding="utf-8")
                violations.append(Violation(path, "file_name_mismatch", f"@file={declared} -> {actual} 已修正", True))
            else:
                violations.append(Violation(path, "file_name_mismatch", f"@file 标记文件名与实际不一致: {declared} != {actual}"))

    # @brief 检查
    brief = tags.get("brief", "").strip()
    if not brief:
        if fix:
            new_body = insert_or_replace_tag(header_body, "brief", f"TODO: 补充 {actual} 摘要")
            content = content.replace(header_body, new_body)
            path.write_text(content, encoding="utf-8")
            violations.append(Violation(path, "missing_brief", "缺少@brief，已插入占位", True))
        else:
            violations.append(Violation(path, "missing_brief", "缺少@brief 或内容为空"))
    else:
        if brief.lower().startswith(actual.lower()):  # 可能是冗余/重复
            violations.append(Violation(path, "weak_brief", "@brief 仅重复文件名，建议更精炼描述"))

    # @date 检查
    date = tags.get("date")
    if not date:
        violations.append(Violation(path, "missing_date", "缺少@date 标签"))
    elif not DATE_PREFIX_RE.search(date):
        violations.append(Violation(path, "bad_date", f"@date 格式不符合 YYYY-MM-DD 前缀: {date}"))
    else:
        # 检查日期中的周数是否正确
        weekday_result = validate_date_weekday(date)
        if not weekday_result.is_valid:
            if fix and weekday_result.corrected_date_str:
                # 自动修复周数错误
                new_body = replace_tag_value(header_body, "date", date, weekday_result.corrected_date_str)
                content = content.replace(header_body, new_body)
                path.write_text(content, encoding="utf-8")
                violations.append(Violation(path, "wrong_weekday", f"@date 周数错误已修复: 周{weekday_result.declared_weekday} -> 周{weekday_result.correct_weekday}", True))
            else:
                violations.append(Violation(path, "wrong_weekday", f"@date 周数错误: {weekday_result.error_message}"))

    # @change 检查（如果存在）
    change = tags.get("change")
    if change and not DATE_PREFIX_RE.search(change):
        violations.append(Violation(path, "bad_change", f"@change 未包含日期前缀: {change}"))
    elif change:
        # 检查change中的周数是否正确
        weekday_result = validate_date_weekday(change)
        if not weekday_result.is_valid:
            if fix and weekday_result.corrected_date_str:
                # 自动修复周数错误
                new_body = replace_tag_value(header_body, "change", change, weekday_result.corrected_date_str)
                content = content.replace(header_body, new_body)
                path.write_text(content, encoding="utf-8")
                violations.append(Violation(path, "wrong_weekday", f"@change 周数错误已修复: 周{weekday_result.declared_weekday} -> 周{weekday_result.correct_weekday}", True))
            else:
                violations.append(Violation(path, "wrong_weekday", f"@change 周数错误: {weekday_result.error_message}"))

    return violations


def insert_or_replace_tag(header_body: str, tag: str, value: str) -> str:
    lines = header_body.splitlines()
    tag_line_prefix = f"@{tag}"
    replaced = False
    for i, line in enumerate(lines):
        if tag_line_prefix in line:
            lines[i] = re.sub(rf"@{tag}.*", f"@{tag} {value}", line)
            replaced = True
            break
    if not replaced:
        # 在结束的 */ 之前插入（我们只在主体内操作，所以直接追加）
        lines.append(f" * @{tag} {value}")
    return "\n".join(lines)


def replace_file_name(header_body: str, old: str, new: str) -> str:
    return re.sub(rf"(@file)\s+{re.escape(old)}", fr"\1 {new}", header_body, count=1)


def replace_tag_value(header_body: str, tag: str, old_value: str, new_value: str) -> str:
    """替换文件头中指定标签的值"""
    # 使用更精确的替换，确保只替换指定标签的值
    pattern = rf"(@{tag}\s+){re.escape(old_value)}"
    replacement = rf"@{tag} {new_value}"
    return re.sub(pattern, replacement, header_body, count=1)


def build_header_template(filename: str) -> str:
    return f"""/**\n * @file {filename}\n * @brief TODO: 补充 {filename} 摘要\n *\n * (自动生成的头部；请完善 @brief 及详细描述)\n * @author {DEFAULT_AUTHOR}\n * @date YYYY-MM-DD HH:MM (UTC+8)\n */\n"""


def collect_files(root: Path) -> List[Path]:
    files: List[Path] = []
    for p in root.rglob("*"):
        if p.is_dir():
            if p.name in EXCLUDE_DIR_NAMES:
                # 快速跳过子树
                continue
        elif p.suffix in FILE_EXT and not is_excluded(p):
            files.append(p)
    return files


def main() -> int:
    parser = argparse.ArgumentParser(description="检查C/C++头部注释块的一致性")
    parser.add_argument("--root", type=str, default=str(Path.cwd()), help="项目根目录")
    parser.add_argument("--fix", action="store_true", help="尝试自动修复简单问题")
    parser.add_argument("--quiet", action="store_true", help="仅打印违规信息")
    args = parser.parse_args()

    root = Path(args.root).resolve()
    if not root.exists():
        print(f"Root path 不存在: {root}", file=sys.stderr)
        return 2

    files = collect_files(root)
    all_violations: List[Violation] = [] # 所有违规项
    for f in files:
        all_violations.extend(scan_file(f, args.fix))

    errors = [v for v in all_violations if v.code not in {"weak_brief"}]

    if not args.quiet:
        _ = f"扫描文件总数: {len(files)}"
        if all_violations:
            print(f"{_}, 发现问题: {len(all_violations)} 条 (致命: {len(errors)})")
        else:
            print(f"{_}, 所有文件头部注释符合规范")
    if all_violations:
        for v in all_violations:
            status = "(已修正)" if v.fix_applied else ""
            print(f"[{v.code}] {v.file}: {v.message} {status}")

    return 0 if not errors else 1


if __name__ == "__main__":  # pragma: no cover - 主程序入口
    sys.exit(main())

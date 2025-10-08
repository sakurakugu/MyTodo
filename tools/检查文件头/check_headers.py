#!/usr/bin/env python3
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
from pathlib import Path
from typing import List, Optional

# TODO: 如果文件头有日期和周数，检查周数是否正确

HEADER_RE = re.compile(r"^\s*/\*\*(?P<body>.*?)\*/", re.DOTALL)
TAG_RE = re.compile(r"@(?P<tag>file|brief|date|change|author)\b\s*(?P<value>.*)")
DATE_PREFIX_RE = re.compile(r"\d{4}-\d{2}-\d{2}")

DEFAULT_AUTHOR = "Sakurakugu"

EXCLUDE_DIR_NAMES = {"build", "_deps", ".git", ".idea", ".vscode"} # 排除的目录名（不区分大小写）
FILE_EXT = {".h", ".hpp", ".hh", ".cpp", ".cxx", ".cc", ".c"} # 支持的文件扩展名（不区分大小写）


@dataclass
class Violation:
    file: Path
    code: str
    message: str
    fix_applied: bool = False


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

    # @change 检查（如果存在）
    change = tags.get("change")
    if change and not DATE_PREFIX_RE.search(change):
        violations.append(Violation(path, "bad_change", f"@change 未包含日期前缀: {change}"))

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

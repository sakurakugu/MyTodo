#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TS文件处理脚本
功能：
1. 移除有内容的<translation type="unfinished">标签中的type="unfinished"属性
2. 对于中文到中文的翻译文件，提供选项自动填充中文翻译
"""

import os
import re
import argparse
import xml.etree.ElementTree as ET
from pathlib import Path


class TSFileProcessor:
    def __init__(self, i18n_dir="i18n"):
        self.i18n_dir = Path(i18n_dir)
        
    def find_ts_files(self):
        """查找所有的.ts文件"""
        if not self.i18n_dir.exists():
            print(f"错误：目录 {self.i18n_dir} 不存在")
            return []
        
        ts_files = list(self.i18n_dir.glob("*.ts"))
        return ts_files
    
    def process_unfinished_translations(self, file_path):
        """处理unfinished标记的移除"""
        print(f"处理文件: {file_path}")
        
        # 读取文件内容
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 备份原文件
        backup_path = file_path.with_suffix('.ts.backup')
        with open(backup_path, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"已创建备份文件: {backup_path}")
        
        # 统计处理的数量
        processed_count = 0
        
        # 使用正则表达式查找并处理有内容的unfinished翻译
        # 匹配模式：<translation type="unfinished">内容</translation>
        pattern = r'<translation\s+type="unfinished">([^<]+)</translation>'
        
        def replace_func(match):
            nonlocal processed_count
            content = match.group(1).strip()
            if content:  # 如果有内容
                processed_count += 1
                return f'<translation>{content}</translation>'
            else:
                return match.group(0)  # 保持原样
        
        # 执行替换
        new_content = re.sub(pattern, replace_func, content)
        
        # 写回文件
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(new_content)
        
        print(f"已处理 {processed_count} 个有内容的unfinished翻译")
        return processed_count
    
    def is_chinese_to_chinese(self, file_path):
        """检查是否是中文到中文的翻译文件"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            # 查找TS标签中的language和sourcelanguage属性
            ts_match = re.search(r'<TS[^>]*language="([^"]*)"[^>]*sourcelanguage="([^"]*)"', content)
            if ts_match:
                language = ts_match.group(1)
                sourcelanguage = ts_match.group(2)
                return language == "zh_CN" and sourcelanguage == "zh_CN"
            return False
        except Exception as e:
            print(f"检查文件 {file_path} 时出错: {e}")
            return False
    
    def fill_chinese_translations(self, file_path):
        """为中文到中文的翻译文件自动填充翻译"""
        print(f"为中文到中文翻译文件填充内容: {file_path}")
        
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 备份原文件
        backup_path = file_path.with_suffix('.ts(自动填充中文).backup')
        with open(backup_path, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"已创建备份文件: {backup_path}")
        
        filled_count = 0
        
        # 查找空的translation标签，并从对应的source标签获取内容
        def process_message_block(match):
            nonlocal filled_count
            message_block = match.group(0)
            
            # 在这个message块中查找source和translation
            source_match = re.search(r'<source>([^<]+)</source>', message_block)
            translation_match = re.search(r'<translation[^>]*></translation>', message_block)
            
            if source_match and translation_match:
                source_text = source_match.group(1)
                filled_count += 1
                # 替换空的translation标签
                new_message_block = re.sub(
                    r'<translation[^>]*></translation>',
                    f'<translation>{source_text}</translation>',
                    message_block
                )
                return new_message_block
            
            return message_block
        
        # 处理每个message块
        new_content = re.sub(
            r'<message>.*?</message>',
            process_message_block,
            content,
            flags=re.DOTALL
        )
        
        # 写回文件
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(new_content)
        
        print(f"已填充 {filled_count} 个空的中文翻译")
        return filled_count
    
    def process_all_files(self, auto_fill_chinese=False):
        """处理所有ts文件"""
        ts_files = self.find_ts_files()
        
        if not ts_files:
            print("未找到任何.ts文件")
            return
        
        print(f"找到 {len(ts_files)} 个.ts文件")
        
        total_processed = 0
        total_filled = 0
        
        for ts_file in ts_files:
            print(f"{'='*50}")
            
            # 处理unfinished标记
            processed = self.process_unfinished_translations(ts_file)
            total_processed += processed
            
            # 如果是中文到中文的文件且用户选择自动填充
            if auto_fill_chinese and self.is_chinese_to_chinese(ts_file):
                filled = self.fill_chinese_translations(ts_file)
                total_filled += filled
        
        print(f"\n{'='*50}")
        print(f"处理完成！")
        print(f"总共移除了 {total_processed} 个有内容的unfinished标记")
        if auto_fill_chinese:
            print(f"总共填充了 {total_filled} 个中文翻译")


def main():
    parser = argparse.ArgumentParser(description='处理TS翻译文件')
    parser.add_argument('--dir', '-d', default='i18n', 
                       help='i18n目录路径 (默认: i18n)')
    parser.add_argument('--auto-fill-chinese', '-c', action='store_true',
                       help='自动填充中文到中文的翻译')
    
    args = parser.parse_args()
    
    processor = TSFileProcessor(args.dir)
    
    print("TS文件处理工具")
    print("功能：")
    print("1. 移除有内容的<translation type=\"unfinished\">标签中的type=\"unfinished\"属性")
    if args.auto_fill_chinese:
        print("2. 自动填充中文到中文的翻译")
    
    # 确认操作
    response = input("\n是否继续？(y/N): ").strip().lower()
    if response not in ['y', 'yes']:
        print("操作已取消")
        return
    
    processor.process_all_files(auto_fill_chinese=args.auto_fill_chinese)


if __name__ == "__main__":
    main()
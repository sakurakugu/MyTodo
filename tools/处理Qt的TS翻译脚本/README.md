# TS文件处理脚本使用说明

## 功能描述

这个Python脚本用于处理Qt的TS翻译文件，主要功能包括：

1. **移除unfinished标记**：当`<translation type="unfinished"></translation>`标签中有内容时，自动移除`type="unfinished"`属性
2. **中文翻译自动填充**：对于目标语言和源语言都是中文的文件（`<TS language="zh_CN" sourcelanguage="zh_CN">`），提供选项自动填充中文翻译

## 使用方法

### 基本用法

```bash
# 在项目根目录运行，处理默认的i18n目录
python tools/处理Qt的TS翻译脚本/Qt_TS文件处理脚本.py

# 指定i18n目录路径
python tools/处理Qt的TS翻译脚本/Qt_TS文件处理脚本.py --dir path/to/i18n

# 同时启用中文翻译自动填充功能
python tools/处理Qt的TS翻译脚本/Qt_TS文件处理脚本.py --auto-fill-chinese

# 完整参数示例
python tools/处理Qt的TS翻译脚本/Qt_TS文件处理脚本.py --dir i18n --auto-fill-chinese
```

### 参数说明

- `--dir, -d`: 指定i18n目录路径（默认：i18n）
- `--auto-fill-chinese, -c`: 启用中文到中文翻译的自动填充功能

## 处理示例

### 处理前
```xml
<translation type="unfinished">确定</translation>
<translation type="unfinished"></translation>
```

### 处理后
```xml
<translation>确定</translation>
<translation type="unfinished"></translation>
```

## 安全特性

- **自动备份**：处理前会自动创建备份文件（.backup后缀）
- **确认提示**：运行前会显示将要执行的操作并要求确认
- **详细日志**：显示处理的文件数量和修改的条目数量

## 注意事项

1. 脚本会自动检测文件编码（UTF-8）
2. 只处理有实际内容的unfinished翻译，空的翻译保持不变
3. 中文翻译填充功能只对`language="zh_CN" sourcelanguage="zh_CN"`的文件生效
4. 建议在运行前先提交代码到版本控制系统

## 文件结构要求

```
项目根目录/
├── tools/
│   └── 处理Qt的TS翻译脚本
│       ├── Qt_TS文件处理脚本.py
│       └── README.md
└── i18n/
    ├── MyTodo_en.ts
    ├── MyTodo_zh_CN.ts
    └── MyTodo_ja.ts
```
@echo off
echo 代码质量检查工具
echo ================

REM 检查是否存在clang-format
clang-format --version >nul 2>&1
if %errorlevel% equ 0 (
    echo 正在检查代码格式...
    
    REM 检查C++文件格式
    for /r cpp %%f in (*.cpp *.h) do (
        echo 检查: %%f
        clang-format --dry-run --Werror %%f
        if %errorlevel% neq 0 (
            echo 格式问题发现在: %%f
            echo 运行以下命令修复: clang-format -i %%f
        )
    )
    
    for /r . %%f in (main.cpp) do (
        echo 检查: %%f
        clang-format --dry-run --Werror %%f
        if %errorlevel% neq 0 (
            echo 格式问题发现在: %%f
            echo 运行以下命令修复: clang-format -i %%f
        )
    )
    
    echo 代码格式检查完成
) else (
    echo 警告: clang-format 未安装，跳过代码格式检查
    echo 建议安装 LLVM/Clang 工具链以启用代码格式检查
)

echo.
echo 检查文件结构...
echo ==================

REM 检查是否有常见的代码质量问题
echo 检查TODO/FIXME注释...
findstr /s /i "TODO\|FIXME\|HACK\|XXX" cpp\*.cpp cpp\*.h main.cpp 2>nul
if %errorlevel% equ 0 (
    echo 发现待处理的注释，请检查上述文件
) else (
    echo 未发现待处理的注释
)

echo.
echo 检查空文件...
for /r cpp %%f in (*.cpp *.h) do (
    for /f %%i in ("%%f") do if %%~zi==0 echo 空文件: %%f
)

echo.
echo 检查长行（超过120字符）...
for /r cpp %%f in (*.cpp *.h) do (
    findstr /n "^.\{120\}" "%%f" >nul 2>&1
    if %errorlevel% equ 0 (
        echo 发现长行在: %%f
        findstr /n "^.\{120\}" "%%f"
    )
)

for %%f in (main.cpp) do (
    findstr /n "^.\{120\}" "%%f" >nul 2>&1
    if %errorlevel% equ 0 (
        echo 发现长行在: %%f
        findstr /n "^.\{120\}" "%%f"
    )
)

echo.
echo 代码质量检查完成！
echo 建议:
echo 1. 定期运行此脚本检查代码质量
echo 2. 安装并配置 clang-format 以自动格式化代码
echo 3. 考虑集成静态分析工具如 clang-static-analyzer
echo 4. 使用 cppcheck 进行更深入的静态分析

pause
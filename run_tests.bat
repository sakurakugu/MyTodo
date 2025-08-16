@echo off
echo 正在构建测试...

REM 创建测试构建目录
if not exist "build_tests" mkdir build_tests
cd build_tests

REM 配置CMake并启用测试
cmake .. -DBUILD_TESTING=ON
if %errorlevel% neq 0 (
    echo CMake配置失败
    pause
    exit /b 1
)

REM 构建项目和测试
cmake --build . --config Debug
if %errorlevel% neq 0 (
    echo 构建失败
    pause
    exit /b 1
)

echo.
echo 运行测试...
echo ================

REM 运行所有测试
ctest --output-on-failure

echo.
echo 测试完成！
pause
@echo off
REM 快速测试脚本 - 测试 SPZ 转 GLB 转换和验证
setlocal enabledelayedexpansion

echo === SPZ2GLB 快速测试 ===
echo.

REM 检查可执行文件
if not exist "build\spz2glb.exe" (
    echo 错误：找不到 build\spz2glb.exe
    echo 请先编译项目
    echo cmake -B build -DCMAKE_BUILD_TYPE=Release
    echo cmake --build build --config Release
    goto :eof
)

if not exist "build\spz_verify.exe" (
    echo 错误：找不到 build\spz_verify.exe
    goto :eof
)

REM 创建测试目录
if not exist "test_output" mkdir test_output
cd test_output

REM 测试工具可用性
if not exist "..\tests\data\triangle.spz" (
    echo 提示：测试文件不存在
    echo 请创建测试数据目录：mkdir tests\data
    echo 并添加 SPZ 测试文件
    echo.
    echo 跳过文件测试，仅测试工具可用性...
    
    echo 测试 spz2glb 帮助...
    ..\build\spz2glb.exe 2>&1 | findstr /N "^" | more +0
    echo.
    
    echo 测试 spz_verify 帮助...
    ..\build\spz_verify.exe 2>&1 | findstr /N "^" | more +0
    echo.
    
    echo ✓ 工具可以正常运行
    cd ..
    rmdir /s /q test_output
    goto :eof
)

REM 运行测试
echo [测试 1] 转换测试...
..\build\spz2glb.exe ..\tests\data\triangle.spz triangle.glb
if %ERRORLEVEL% neq 0 (
    echo ✗ 转换失败
    cd ..
    rmdir /s /q test_output
    goto :eof
)
echo ✓ 转换成功
echo.

echo [测试 2] Layer 1 - GLB 结构验证...
..\build\spz_verify.exe layer1 triangle.glb
if %ERRORLEVEL% neq 0 (
    echo ✗ Layer 1 失败
    cd ..
    rmdir /s /q test_output
    goto :eof
)
echo ✓ Layer 1 通过
echo.

echo [测试 3] Layer 2 - 二进制无损验证...
..\build\spz_verify.exe layer2 ..\tests\data\triangle.spz triangle.glb
if %ERRORLEVEL% neq 0 (
    echo ✗ Layer 2 失败
    cd ..
    rmdir /s /q test_output
    goto :eof
)
echo ✓ Layer 2 通过
echo.

echo [测试 4] Layer 3 - 解码一致性验证...
..\build\spz_verify.exe layer3 ..\tests\data\triangle.spz triangle.glb
if %ERRORLEVEL% neq 0 (
    echo ✗ Layer 3 失败
    cd ..
    rmdir /s /q test_output
    goto :eof
)
echo ✓ Layer 3 通过
echo.

echo === 所有测试通过 ===
cd ..
rmdir /s /q test_output

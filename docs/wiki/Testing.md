# 测试

本文档介绍 spz2glb 的测试框架和使用方法。

## 📋 概述

spz2glb 使用 CTest 测试框架，支持自动化测试。

### 测试类型

- **单元测试**: 测试单个函数/模块
- **集成测试**: 测试模块间交互
- **回归测试**: 确保旧问题不复发
- **性能测试**: 基准测试和性能回归

### 测试结构

```
tests/
├── CMakeLists.txt      # CTest 配置
├── README.md           # 测试文档
├── quick_test.sh       # Linux/macOS 快速测试
├── quick_test.bat      # Windows 快速测试
└── data/               # 测试数据
    ├── sample.spz      # 示例 SPZ 文件
    └── expected.glb    # 期望输出
```

## 🚀 快速开始

### 启用测试构建

```bash
# 配置时启用测试
cmake -B build -S tools/spz_to_glb -DBUILD_TESTS=ON

# 构建
cmake --build build --config Release -j4

# 运行所有测试
cd build
ctest --output-on-failure
```

### 快速测试脚本

#### Windows

```powershell
# quick_test.bat
@echo off
echo 快速测试 spz2glb

# 检查工具
if not exist "build\Release\spz2glb.exe" (
    echo 错误：spz2glb.exe 不存在
    exit /b 1
)

# 简单转换
build\Release\spz2glb.exe tests\data\sample.spz test_output.glb

# 验证
build\Release\spz_verify.exe all tests\data\sample.spz test_output.glb

if %ERRORLEVEL% EQU 0 (
    echo 测试通过
) else (
    echo 测试失败
    exit /b 1
)
```

#### Linux/macOS

```bash
#!/bin/bash
# quick_test.sh

echo "快速测试 spz2glb"

# 检查工具
if [ ! -f "build/spz2glb" ]; then
    echo "错误：spz2glb 不存在"
    exit 1
fi

# 简单转换
./build/spz2glb tests/data/sample.spz test_output.glb

# 验证
./build/spz_verify all tests/data/sample.spz test_output.glb

if [ $? -eq 0 ]; then
    echo "测试通过"
else
    echo "测试失败"
    exit 1
fi
```

## 📝 编写测试

### 测试模板

```cpp
#include <gtest/gtest.h>
#include "spz_to_glb.h"

TEST(SPZToGLBTest, BasicConversion) {
    // 准备输入
    const std::string inputSpz = "tests/data/sample.spz";
    const std::string outputGlb = "test_output.glb";
    
    // 执行转换
    int result = convertSPZToGLB(inputSpz, outputGlb);
    
    // 验证结果
    EXPECT_EQ(result, 0);
    EXPECT_TRUE(fileExists(outputGlb));
}

TEST(SPZVerifyTest, Layer1) {
    const std::string glbFile = "tests/data/expected.glb";
    
    // Layer 1 验证
    bool passed = verifyLayer1(glbFile);
    
    EXPECT_TRUE(passed);
}

TEST(SPZVerifyTest, Layer2) {
    const std::string spzFile = "tests/data/sample.spz";
    const std::string glbFile = "test_output.glb";
    
    // Layer 2 验证
    bool passed = verifyLayer2(spzFile, glbFile);
    
    EXPECT_TRUE(passed);
}

TEST(SPZVerifyTest, Layer3) {
    const std::string spzFile = "tests/data/sample.spz";
    const std::string glbFile = "test_output.glb";
    
    // Layer 3 验证
    bool passed = verifyLayer3(spzFile, glbFile);
    
    EXPECT_TRUE(passed);
}
```

### CMakeLists.txt

```cmake
enable_testing()

# 添加测试
add_test(NAME BasicConversion
    COMMAND spz2glb ${CMAKE_SOURCE_DIR}/tests/data/sample.spz test_output.glb
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_test(NAME Layer1Verification
    COMMAND spz_verify layer1 test_output.glb
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_test(NAME Layer2Verification
    COMMAND spz_verify layer2 
        ${CMAKE_SOURCE_DIR}/tests/data/sample.spz 
        test_output.glb
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_test(NAME Layer3Verification
    COMMAND spz_verify layer3 
        ${CMAKE_SOURCE_DIR}/tests/data/sample.spz 
        test_output.glb
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_test(NAME FullVerification
    COMMAND spz_verify all 
        ${CMAKE_SOURCE_DIR}/tests/data/sample.spz 
        test_output.glb
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# 设置测试属性
set_tests_properties(BasicConversion PROPERTIES
    DEPENDS "Layer1Verification"
)
```

## 🧪 运行测试

### 基本用法

```bash
# 运行所有测试
ctest

# 详细输出
ctest --output-on-failure

# 并行运行
ctest -j4

# 仅运行特定测试
ctest -R "Layer.*"

# 跳过特定测试
ctest -E "Performance"

# 重复运行
ctest --repeat until-pass:3
```

### 输出示例

```
Test project /path/to/build
    Start 1: BasicConversion
1/5 Test #1: BasicConversion .................   Passed    0.50 sec
    Start 2: Layer1Verification
2/5 Test #2: Layer1Verification ..............   Passed    0.10 sec
    Start 3: Layer2Verification
3/5 Test #3: Layer2Verification ..............   Passed    1.00 sec
    Start 4: Layer3Verification
4/5 Test #4: Layer3Verification ..............   Passed   10.00 sec
    Start 5: FullVerification
5/5 Test #5: FullVerification ................   Passed   11.00 sec

100% tests passed, 0 tests failed out of 5

Total Test time (real) =  11.50 sec
```

## 📊 测试覆盖率

### 使用 gcov

```bash
# 配置（带覆盖率）
cmake -B build -S tools/spz_to_glb \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="--coverage"

# 构建
cmake --build build

# 运行测试
cd build
ctest

# 生成报告
gcovr -r .. --html-details coverage.html

# 查看报告
open coverage.html  # macOS
xdg-open coverage.html  # Linux
start coverage.html  # Windows
```

### 使用 lcov

```bash
# 清理
lcov --zerocounters --directory .

# 运行测试
ctest

# 捕获覆盖率
lcov --capture --directory . --output-file coverage.info

# 生成 HTML
genhtml coverage.info --output-directory coverage

# 查看
open coverage/index.html
```

## 📈 性能测试

### 基准测试

```cpp
#include <benchmark/benchmark.h>

static void BM_Conversion(benchmark::State& state) {
    for (auto _ : state) {
        convertSPZToGLB("sample.spz", "output.glb");
    }
}

BENCHMARK(BM_Conversion)->Unit(benchmark::kMillisecond);

static void BM_Layer1Verify(benchmark::State& state) {
    for (auto _ : state) {
        verifyLayer1("output.glb");
    }
}

BENCHMARK(BM_Layer1Verify)->Unit(benchmark::kMicrosecond);

static void BM_Layer2Verify(benchmark::State& state) {
    for (auto _ : state) {
        verifyLayer2("sample.spz", "output.glb");
    }
}

BENCHMARK(BM_Layer2Verify)->Unit(benchmark::kMillisecond);

static void BM_Layer3Verify(benchmark::State& state) {
    for (auto _ : state) {
        verifyLayer3("sample.spz", "output.glb");
    }
}

BENCHMARK(BM_Layer3Verify)->Unit(benchmark::kMillisecond);
```

### 运行基准测试

```bash
# 构建基准测试
cmake -DBUILD_BENCHMARKS=ON ...
cmake --build build

# 运行
./build/benchmarks

# 输出示例
# ------------------------------------------------
# Benchmark              Time           CPU Iterations
# ------------------------------------------------
# BM_Conversion       1050 ms       1045 ms          1
# BM_Layer1Verify       10 ms          9 ms        100
# BM_Layer2Verify      100 ms         98 ms         10
# BM_Layer3Verify    10000 ms       9950 ms          1
```

## 📦 测试数据

### 创建测试数据

```bash
# 1. 准备原始 SPZ 文件
# 使用 SPZ 编码器生成

# 2. 生成期望输出
spz2glb sample.spz expected.glb

# 3. 验证
spz_verify all sample.spz expected.glb

# 4. 添加到 tests/data/
cp sample.spz expected.glb tests/data/
```

### 测试数据要求

- **小文件**: < 1 MB（快速测试）
- **中等文件**: 1-10 MB（常规测试）
- **大文件**: 10-100 MB（性能测试）
- **覆盖所有属性**: position, color, scaling, etc.

## 🔍 调试测试

### 详细日志

```bash
# 设置日志级别
export SPZ_LOG_LEVEL=debug

# 运行测试
ctest -V
```

### 单步调试

```bash
# GDB 调试
gdb --args ./spz2glb sample.spz output.glb

# LLDB 调试
lldb -- ./spz2glb sample.spz output.glb
```

### 内存调试

```bash
# Valgrind（Linux/macOS）
valgrind --leak-check=full ./spz2glb sample.spz output.glb

# AddressSanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address" ...
cmake --build build
ctest
```

## 📋 测试清单

### 提交前

- [ ] 所有单元测试通过
- [ ] 所有集成测试通过
- [ ] 覆盖率 > 80%
- [ ] 无内存泄漏
- [ ] 性能无回归

### 发布前

- [ ] 所有平台测试通过（Windows/Linux/macOS）
- [ ] 性能基准测试
- [ ] 兼容性测试
- [ ] 文档测试

## ❓ 常见问题

### Q: 测试失败？

A:
```bash
# 查看详细输出
ctest -V

# 仅运行失败的测试
ctest --rerun-failed

# 清理并重新测试
rm -rf build/
cmake -B build -DBUILD_TESTS=ON ...
cmake --build build
ctest
```

### Q: 测试太慢？

A:
- 使用小文件测试
- 跳过 L3 验证（快速模式）
- 并行运行测试

### Q: 如何添加新测试？

A:
1. 创建测试文件
2. 添加到 CMakeLists.txt
3. 运行测试
4. 提交 PR

## 🔗 相关资源

- [Building](Building) - 构建指南
- [Verification](Verification) - 验证工具
- [Contributing](Contributing) - 贡献指南

---

**最后更新**: 2026-02-28  
**版本**: v1.0.1

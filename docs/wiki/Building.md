# 构建指南

本文档介绍如何从源码构建 spz2glb。

## 📋 概述

spz2glb 使用 CMake 构建系统，支持跨平台编译。

### 构建目标

- **spz2glb**: SPZ 到 GLB 转换器
- **spz_verify**: 三层验证工具
- **测试**: 可选构建测试套件

## 🔧 前置要求

### 必需组件

| 组件 | 版本 | 说明 |
|------|------|------|
| CMake | ≥ 3.14 | 构建系统 |
| C++ 编译器 | C++17 | MSVC/GCC/Clang |

### 可选组件

| 组件 | 版本 | 说明 |
|------|------|------|
| ZLIB | 任意 | 压缩库（Windows 需要） |
| Git | 任意 | 克隆仓库 |
| CTest | - | 运行测试 |

### 依赖管理

项目使用嵌入式依赖：
- **fastgltf**: 修改版（支持 SPZ_2 扩展）
- **simdjson**: v4.3.1（嵌入式）
- **ZLIB**: 系统库（Windows 使用 vcpkg）

## 🪟 Windows 构建

### 方法 1: Visual Studio

```powershell
# 1. 安装 Visual Studio 2019/2022
# 选择 "桌面 C++ 开发" 工作负载

# 2. 安装 vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install zlib:x64-windows

# 3. 克隆仓库
git clone https://github.com/spz-ecosystem/spz2glb.git
cd spz2glb

# 4. 配置
cmake -B build -S tools/spz_to_glb `
    -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

# 5. 构建
cmake --build build --config Release -j4

# 6. 验证
.\build\Release\spz2glb.exe --version
```

### 方法 2: 命令行（MinGW）

```powershell
# 1. 安装 MSYS2
# 下载：https://www.msys2.org/

# 2. 安装工具链
pacman -S mingw-w64-x86_64-toolchain
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-zlib

# 3. 克隆仓库
git clone https://github.com/spz-ecosystem/spz2glb.git
cd spz2glb

# 4. 配置
cmake -B build -S tools/spz_to_glb \
    -G "MinGW Makefiles"

# 5. 构建
cmake --build build --config Release -j4

# 6. 验证
.\build\spz2glb.exe --version
```

## 🐧 Linux 构建

### Ubuntu/Debian

```bash
# 1. 安装依赖
sudo apt-get update
sudo apt-get install -y cmake g++ zlib1g-dev git

# 2. 克隆仓库
git clone https://github.com/spz-ecosystem/spz2glb.git
cd spz2glb

# 3. 配置
cmake -B build -S tools/spz_to_glb

# 4. 构建
cmake --build build --config Release -j$(nproc)

# 5. 验证
./build/spz2glb --version
```

### Fedora/RHEL

```bash
# 1. 安装依赖
sudo dnf install -y cmake gcc-c++ zlib-devel git

# 2. 克隆仓库
git clone https://github.com/spz-ecosystem/spz2glb.git
cd spz2glb

# 3. 配置
cmake -B build -S tools/spz_to_glb

# 4. 构建
cmake --build build --config Release -j$(nproc)

# 5. 验证
./build/spz2glb --version
```

### Arch Linux

```bash
# 1. 安装依赖
sudo pacman -S cmake gcc zlib git

# 2. 克隆仓库
git clone https://github.com/spz-ecosystem/spz2glb.git
cd spz2glb

# 3. 配置
cmake -B build -S tools/spz_to_glb

# 4. 构建
cmake --build build --config Release -j$(nproc)

# 5. 验证
./build/spz2glb --version
```

## 🍎 macOS 构建

```bash
# 1. 安装 Xcode Command Line Tools
xcode-select --install

# 2. 安装 Homebrew（可选）
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 3. 安装依赖
brew install cmake

# 4. 克隆仓库
git clone https://github.com/spz-ecosystem/spz2glb.git
cd spz2glb

# 5. 配置
cmake -B build -S tools/spz_to_glb

# 6. 构建
cmake --build build --config Release -j$(sysctl -n hw.ncpu)

# 7. 验证
./build/spz2glb --version
```

## 🧪 构建测试

### 启用测试

```bash
# 配置时启用测试
cmake -B build -S tools/spz_to_glb -DBUILD_TESTS=ON

# 构建
cmake --build build --config Release -j4

# 运行测试
cd build
ctest --output-on-failure

# 详细输出
ctest -V
```

### 测试结构

```
tests/
├── CMakeLists.txt      # 测试配置
├── README.md           # 测试文档
├── quick_test.sh       # Linux/macOS 快速测试
├── quick_test.bat      # Windows 快速测试
└── data/               # 测试数据（需手动添加）
```

## ⚙️ 构建选项

### CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `BUILD_TESTS` | OFF | 构建测试套件 |
| `CMAKE_BUILD_TYPE` | Release | 构建类型（Debug/Release） |
| `CMAKE_INSTALL_PREFIX` | /usr/local | 安装路径 |

### 示例

```bash
# Debug 构建
cmake -B build -S tools/spz_to_glb -DCMAKE_BUILD_TYPE=Debug

# 安装到自定义路径
cmake -B build -S tools/spz_to_glb \
    -DCMAKE_INSTALL_PREFIX=/opt/spz2glb

# 启用所有选项
cmake -B build -S tools/spz_to_glb \
    -DBUILD_TESTS=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/opt/spz2glb
```

## 🔨 高级构建

### 优化构建

```bash
# GCC/Clang - 性能优化
export CXXFLAGS="-O3 -march=native -mtune=native"
cmake -B build -S tools/spz_to_glb

# MSVC - 性能优化
cmake -B build -S tools/spz_to_glb \
    -DCMAKE_CXX_FLAGS="/O2 /arch:AVX2"
```

### 链接时优化 (LTO)

```cmake
# CMakeLists.txt 中添加
include(CheckIPOSupported)
check_ipo_supported(RESULT result OUTPUT output)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
```

### 配置文件生成优化 (PGO)

```bash
# 1. 生成配置文件
cmake -B build -S tools/spz_to_glb \
    -DCMAKE_CXX_FLAGS="-fprofile-generate"

cmake --build build

# 2. 运行典型负载
./build/spz2glb large.spz large.glb

# 3. 使用配置文件
cmake -B build -S tools/spz_to_glb \
    -DCMAKE_CXX_FLAGS="-fprofile-use"

cmake --build build
```

## 📦 安装

### Linux/macOS

```bash
# 构建
cmake --build build --config Release

# 安装
sudo cmake --install build

# 或安装到自定义路径
cmake --install build --prefix /opt/spz2glb
```

### Windows

```powershell
# 构建
cmake --build build --config Release

# 安装（需要管理员权限）
cmake --install build --prefix "C:\Program Files\spz2glb"
```

## 🧹 清理构建

```bash
# 删除构建目录
rm -rf build/

# 或
cmake --build build --target clean

# 完全清理（包括缓存）
rm -rf build/ CMakeCache.txt CMakeFiles/
```

## ❓ 常见问题

### Q: CMake 找不到 ZLIB

**Windows**:
```powershell
vcpkg install zlib:x64-windows
cmake -DCMAKE_TOOLCHAIN_FILE=[vcpkg]/scripts/buildsystems/vcpkg.cmake ...
```

**Linux**:
```bash
sudo apt-get install zlib1g-dev
```

### Q: 编译器不支持 C++17

**解决方案**: 升级编译器
- **GCC**: ≥ 7
- **Clang**: ≥ 5
- **MSVC**: ≥ 2017

### Q: 链接错误

**解决方案**:
```bash
# 清理并重新构建
rm -rf build/
cmake -B build -S tools/spz_to_glb
cmake --build build -j4
```

### Q: 测试失败

**解决方案**:
```bash
# 查看详细输出
ctest -V

# 仅运行特定测试
ctest -R test_name

# 跳过测试
cmake -DBUILD_TESTS=OFF ...
```

## 🔗 相关资源

- [Installation](Installation) - 安装指南
- [Testing](Testing) - 测试说明
- [Performance](Performance) - 性能优化
- [Contributing](Contributing) - 贡献指南

---

**最后更新**: 2026-02-28  
**版本**: v1.0.1

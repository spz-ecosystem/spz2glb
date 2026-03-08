# 安装指南

本文档介绍如何安装 spz2glb 转换器。

## 📦 方式一：下载预编译二进制（推荐）

### Windows

```powershell
# 下载
curl -L https://github.com/spz-ecosystem/spz2glb/releases/download/v1.0.1/spz2glb-windows-x64.exe -o spz2glb.exe

# 验证（可选）
certutil -hashfile spz2glb.exe SHA256

# 使用
.\spz2glb.exe input.spz output.glb
```

### Linux

```bash
# 下载
curl -L https://github.com/spz-ecosystem/spz2glb/releases/download/v1.0.1/spz2glb-linux-x64 -o spz2glb
chmod +x spz2glb

# 验证（可选）
sha256sum spz2glb

# 使用
./spz2glb input.spz output.glb
```

### macOS

```bash
# 下载
curl -L https://github.com/spz-ecosystem/spz2glb/releases/download/v1.0.1/spz2glb-macos-x64 -o spz2glb
chmod +x spz2glb

# 验证（可选）
shasum -a 256 spz2glb

# 使用
./spz2glb input.spz output.glb
```

## 🔨 方式二：从源码构建

### 前置要求

| 组件 | 版本要求 | 说明 |
|------|----------|------|
| CMake | ≥ 3.14 | 构建系统 |
| C++ 编译器 | C++17 支持 | MSVC/GCC/Clang |
| ZLIB | 任意 | 压缩库（Windows 需单独安装） |

### Windows

```powershell
# 1. 安装依赖
# 使用 vcpkg 安装 ZLIB
vcpkg install zlib:x64-windows

# 2. 克隆仓库
git clone https://github.com/spz-ecosystem/spz2glb.git
cd spz2glb

# 3. 配置
cmake -B build -S tools/spz_to_glb -DCMAKE_TOOLCHAIN_FILE=[vcpkg 根目录]/scripts/buildsystems/vcpkg.cmake

# 4. 构建
cmake --build build --config Release -j4

# 5. 验证
.\build\Release\spz2glb.exe --version
```

### Linux

```bash
# 1. 安装依赖
# Ubuntu/Debian
sudo apt-get install cmake g++ zlib1g-dev

# Fedora/RHEL
sudo dnf install cmake gcc-c++ zlib-devel

# 2. 克隆仓库
git clone https://github.com/spz-ecosystem/spz2glb.git
cd spz2glb

# 3. 配置
cmake -B build -S tools/spz_to_glb

# 4. 构建
cmake --build build --config Release -j4

# 5. 验证
./build/spz2glb --version
```

### macOS

```bash
# 1. 安装依赖
brew install cmake

# 2. 克隆仓库
git clone https://github.com/spz-ecosystem/spz2glb.git
cd spz2glb

# 3. 配置
cmake -B build -S tools/spz_to_glb

# 4. 构建
cmake --build build --config Release -j4

# 5. 验证
./build/spz2glb --version
```

## 🧪 构建验证工具

```bash
# 启用测试构建
cmake -B build -S tools/spz_to_glb -DBUILD_TESTS=ON

# 构建
cmake --build build --config Release -j4

# 运行测试
cd build
ctest --output-on-failure
```

## ⚙️ 环境变量（可选）

### Windows

```powershell
# 添加到 PATH（可选）
$env:Path += ";C:\path\to\spz2glb"

# 永久添加（管理员权限）
[Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\path\to\spz2glb", "Machine")
```

### Linux/macOS

```bash
# 添加到 ~/.bashrc 或 ~/.zshrc
export PATH="$PATH:/path/to/spz2glb"

# 生效
source ~/.bashrc
```

## 📋 验证安装

```bash
# 检查版本
spz2glb --version

# 检查帮助
spz2glb --help

# 简单转换测试
spz2glb test.spz test.glb

# 验证转换
spz_verify all test.spz test.glb
```

## ❓ 常见问题

### Windows: 找不到 VCRUNTIME140.dll

安装 Visual C++ Redistributable：
- 下载：https://aka.ms/vs/17/release/vc_redist.x64.exe

### Linux: 找不到 zlib

```bash
# Ubuntu/Debian
sudo apt-get install zlib1g

# Fedora/RHEL
sudo dnf install zlib
```

### macOS: 无法验证开发者

```bash
# 移除隔离属性
xattr -d com.apple.quarantine spz2glb
```

## 🔗 相关资源

- [Releases](https://github.com/spz-ecosystem/spz2glb/releases)
- [Quick Start](Quick-Start)
- [Building](Building)

---

**最后更新**: 2026-02-28  
**版本**: v1.0.1

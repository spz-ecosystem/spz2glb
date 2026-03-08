# spz2glb - SPZ to GLB Converter

将 SPZ (Gaussian Splatting Compression) 文件转换为 glTF 2.0 GLB 格式的工具。

## 📚 文档

- **Wiki**: https://github.com/spz-ecosystem/spz2glb/wiki
  - [安装指南](https://github.com/spz-ecosystem/spz2glb/wiki/Installation)
  - [快速开始](https://github.com/spz-ecosystem/spz2glb/wiki/Quick-Start)
  - [使用方法](https://github.com/spz-ecosystem/spz2glb/wiki/Usage)
  - [三层验证](https://github.com/spz-ecosystem/spz2glb/wiki/Verification)
  - [批量处理](https://github.com/spz-ecosystem/spz2glb/wiki/Batch-Processing)
  - [性能优化](https://github.com/spz-ecosystem/spz2glb/wiki/Performance)
  - [故障排除](https://github.com/spz-ecosystem/spz2glb/wiki/Troubleshooting)
  - [常见问题](https://github.com/spz-ecosystem/spz2glb/wiki/FAQ)
  - [构建指南](https://github.com/spz-ecosystem/spz2glb/wiki/Building)
  - [贡献指南](https://github.com/spz-ecosystem/spz2glb/wiki/Contributing)

## 特性

- **SPZ 转 GLB**: 支持将压缩的 SPZ 文件转换为标准 GLB 格式
- **KHR_gaussian_splatting_compression_spz_2**: 集成 SPZ_2 压缩扩展
- **无损转换**: 压缩流模式，保持原始 SPZ 数据完整
- **跨平台**: 支持 Windows、Linux、macOS (x64 + ARM)
- **自动构建**: GitHub Actions 提供预编译二进制
- **三层验证**: 完整的 C++ 验证工具确保转换正确性

## 🎬 演示

### 基本转换

```bash
# 转换 SPZ 到 GLB
./build/spz2glb model.spz model.glb
```

### 三层验证

```bash
# 运行所有验证（提供你自己的 SPZ 和 GLB 文件）
./build/spz_verify all input.spz output.glb

# 输出：
# Layer 1: GLB Structure & SPZ_2 Specification Validation - PASSED (7/7)
# Layer 2: Binary Lossless Verification - PASSED (100% MD5 match)
# Layer 3: Decoding Consistency Verification - PASSED (Size match)
# [SUCCESS] All verifications PASSED!
```

> **注意**: 路径应该是相对路径或绝对路径指向你的文件。不要使用硬编码路径。

> **Demo**: Demo 演示视频将于第一个稳定版发布之后放出，敬请期待！

### 批量处理

```bash
# 批量转换所有 SPZ 文件
for file in *.spz; do
    ./build/spz2glb "$file" "${file%.spz}.glb"
done
```

## 快速开始

### 方式一：下载预编译版本

从 [Releases](https://github.com/spz-ecosystem/spz2glb/releases) 下载对应平台的二进制文件：

- Windows: `spz2glb-windows-x64.exe`
- Linux: `spz2glb-linux-x64`
- macOS: `spz2glb-macos-x64`

### 方式二：从源码编译（一键编译）

```bash
# 1. 克隆仓库
git clone https://github.com/spz-ecosystem/spz2glb.git
cd spz2glb

# 2. 一键编译（自动处理所有依赖）
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)

# 3. 运行
./build/spz2glb input.spz output.glb
```

**平台特定依赖安装**（编译前）：

```bash
# Ubuntu/Debian
sudo apt-get install -y zlib1g-dev

# macOS
brew install zlib

# Windows
# 无需手动安装，CI 使用 vcpkg 自动安装
```

## 使用方法

### 转换器 (spz2glb)

```bash
spz2glb <input.spz> <output.glb>
```

**完整示例**：

```bash
# 转换单个文件
./build/spz2glb model.spz model.glb

# 批量转换
for file in *.spz; do
    ./build/spz2glb "$file" "${file%.spz}.glb"
done
```

**输出示例**：

```
[INFO] Loading SPZ: model.spz
[INFO] SPZ version: 2
[INFO] Num points: 100000
[INFO] SH degree: 3
[INFO] SPZ size (raw compressed): 15 MB
[INFO] Creating glTF Asset with KHR extensions
[INFO] Exporting GLB...
[SUCCESS] GLB exported: model.glb
[INFO] GLB size: 16 MB
```

### 三层验证工具 (spz_verify)

> **重要说明**:
> - **独立工具**: spz_verify 是独立的验证工具，不是生产转换流程的一部分
> - **开发/测试用途**: 设计用于质量保证、调试和测试工作流
> - **日常使用不需要**: 一旦转换被验证，生产环境只需要 spz2glb
> - **Layer 2 会解压**: Layer 2 验证会提取并解压数据来计算 MD5 哈希值（比 Layer 1/3 慢）

```bash
spz_verify <command> [options]
```

**命令**：

```bash
# 运行全部三层验证
spz_verify all <input.spz> <output.glb>

# 单独运行某层验证
spz_verify layer1 <output.glb>              # GLB 结构验证 (快速)
spz_verify layer2 <input.spz> <output.glb>  # 二进制无损验证 (MD5, 较慢)
spz_verify layer3 <input.spz> <output.glb>  # 解码一致性验证 (快速)
```

**完整示例**：

```bash
# 1. 转换文件
./build/spz2glb model.spz model.glb

# 2. 运行所有验证
./build/spz_verify all model.spz model.glb

# 或者单独验证
./build/spz_verify layer1 model.glb
./build/spz_verify layer2 model.spz model.glb
./build/spz_verify layer3 model.spz model.glb
```

**验证输出**：

```
Layer 1: GLB Structure Validation
  ✓ Magic number: 0x46546C67 ("glTF")
  ✓ Version: 2
  ✓ extensionsUsed contains KHR_gaussian_splatting
  ✓ extensionsUsed contains KHR_gaussian_splatting_compression_spz_2
  ✓ buffers configuration correct
  ✓ Compression stream mode (attributes empty)
  [PASS] Layer 1 validation passed

Layer 2: Lossless Binary Validation
  ✓ Original SPZ MD5: abc123...
  ✓ Extracted data MD5: abc123...
  ✓ MD5 match confirmed
  [PASS] Layer 2 validation passed

Layer 3: Decode Consistency Validation
  ✓ GLB structure valid
  ✓ Extension integrity check passed
  [PASS] Layer 3 validation passed

[SUCCESS] All 3 layers validation passed!
```

## 自动化验证脚本（推荐）

创建 `verify.sh` 或 `verify.bat` 脚本自动执行转换 + 验证：

**Linux/macOS** (`verify.sh`)：

```bash
#!/bin/bash
set -e

if [ $# -ne 1 ]; then
    echo "Usage: $0 <input.spz>"
    exit 1
fi

INPUT="$1"
OUTPUT="${INPUT%.spz}.glb"
SPZ2GLB="./build/spz2glb"
VERIFY="./build/spz_verify"

echo "=== SPZ to GLB Conversion & Verification ==="
echo "Input:  $INPUT"
echo "Output: $OUTPUT"
echo ""

# Step 1: Convert
echo "[1/2] Converting SPZ to GLB..."
$SPZ2GLB "$INPUT" "$OUTPUT"
echo ""

# Step 2: Verify
echo "[2/2] Running 3-layer verification..."
$VERIFY all "$INPUT" "$OUTPUT"
echo ""

echo "=== Complete ==="
```

**Windows** (`verify.bat`)：

```batch
@echo off
setlocal enabledelayedexpansion

if "%~1"=="" (
    echo Usage: %~0 ^<input.spz^>
    exit /b 1
)

set INPUT=%~1
set OUTPUT=%INPUT:.spz=.glb%
set SPZ2GLB=build\spz2glb.exe
set VERIFY=build\spz_verify.exe

echo === SPZ to GLB Conversion ^& Verification ===
echo Input:  %INPUT%
echo Output: %OUTPUT%
echo.

echo [1/2] Converting SPZ to GLB...
%SPZ2GLB% "%INPUT%" "%OUTPUT%"
echo.

echo [2/2] Running 3-layer verification...
%VERIFY% all "%INPUT%" "%OUTPUT%"
echo.

echo === Complete ===
```

**使用脚本**：

```bash
# Linux/macOS
chmod +x verify.sh
./verify.sh model.spz

# Windows
verify.bat model.spz
```

## 依赖

- CMake 3.15+
- C++17 编译器
- ZLIB (系统包管理器自动安装)

**依赖说明**：

| 工具 | 依赖 | 用途 |
|------|------|------|
| spz2glb | ZLIB, fastgltf, simdjson | SPZ 转 GLB |
| spz_verify | ZLIB only | 三层验证 |

## 项目结构

```
spz2glb/
├── CMakeLists.txt          # 构建配置
├── LICENSE                 # MIT 许可证
├── README.md               # 英文版文档
├── README-zh.md            # 中文版文档
├── src/
│   ├── spz_to_glb.cpp     # 转换器源码
│   └── spz_verify.cpp     # 三层验证工具源码
├── third_party/            # 定制版 fastgltf + simdjson
│   ├── CMakeLists.txt
│   ├── include/fastgltf/
│   ├── src/
│   └── deps/simdjson/     # simdjson v4.3.1 (内置)
└── .github/
    └── workflows/
        └── release.yml    # CI/CD 工作流
```

## 技术细节

### 压缩流模式

本工具使用 SPZ_2 规范的压缩流模式：
- SPZ 压缩数据直接存储在 bufferView 中
- 不定义 accessors 或 attributes
- 需要 SPZ 解码器的渲染器才能解析

**优势**：
- **无损**: 不重新编码，直接复制 SPZ 流
- **最小体积**: SPZ 压缩率约 10x
- **最快加载**: 无需额外编解码开销

**兼容性说明**：
> 任何 SPZ 衍生算法，只要 100% 兼容原版 SPZ 格式，严格遵守 SPZ_2 扩展规范，本转换器就完美支持。

### GLB 结构

```
GLB Header (12 bytes)
├── magic: 0x46546C67 ("glTF")
├── version: 2
└── length: total file size

JSON Chunk
├── chunkLength
├── chunkType: 0x4E4F534A ("JSON")
└── glTF JSON (padded to 4-byte boundary)
    └── KHR_gaussian_splatting_compression_spz_2 extension

BIN Chunk
├── chunkLength
├── chunkType: 0x004E4942 ("BIN\0")
└── Raw SPZ compressed data
```

## 免责声明

**本项目为个人独立开发项目。**

- 本项目由作者以个人身份独立开发
- 本项目**不隶属于**任何院校、机构或雇主
- 本项目**非职务成果**，非院校教学成果
- 本项目观点仅代表作者个人立场
- MIT License 授权 - 详见 [LICENSE](LICENSE)

## 许可证

MIT License - 详见 [LICENSE](LICENSE)

## 相关项目

- [fastgltf](https://github.com/spycrab/fastgltf) - 高性能 glTF 库
- [simdjson](https://github.com/simdjson/simdjson) - 极速 JSON 解析库 v4.3.1
- [KHR_gaussian_splatting](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_gaussian_splatting) - Khronos Gaussian Splatting 扩展

## 定制说明

本项目使用 **定制版 fastgltf**，包含以下修改：

1. **simdjson v4.3.1 内置**: 不查找系统库，不从网络下载，使用内置源码
2. **KHR_gaussian_splatting_compression_spz_2 扩展**: 支持 SPZ_2 压缩格式
3. **一键编译**: 只需 `cmake && cmake --build`，无需手动配置依赖

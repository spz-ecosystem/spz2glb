# spz2glb - SPZ to GLB Converter

将 SPZ (Gaussian Splatting Compression) 文件转换为 glTF 2.0 GLB 格式的工具。

## 特性

- **SPZ 转 GLB**: 支持将压缩的 SPZ 文件转换为标准 GLB 格式
- **KHR_gaussian_splatting_compression_spz_2**: 集成 SPZ_2 压缩扩展
- **无损转换**: 压缩流模式，保持原始 SPZ 数据完整
- **跨平台**: 支持 Windows、Linux、macOS (x64 + ARM)
- **自动构建**: GitHub Actions 提供预编译二进制
- **三层验证**: 完整的 C++ 验证工具确保转换正确性

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

```bash
spz_verify <command> [options]
```

**命令**：

```bash
# 运行全部三层验证
spz_verify all <input.spz> <output.glb>

# 单独运行某层验证
spz_verify layer1 <output.glb>              # GLB 结构验证
spz_verify layer2 <input.spz> <output.glb>  # 二进制无损验证 (MD5)
spz_verify layer3 <input.spz> <output.glb>  # 解码一致性验证
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
├── README.md               # 本文档
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

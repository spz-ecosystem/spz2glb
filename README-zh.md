# spz2glb - SPZ to GLB Converter

**无损打包 SPZ 为 GLB 格式** —— 保持 SPZ 压缩流完整，支持“网页轻量处理 + 本地重任务”双场景协同。

## 发布状态

- **v2.0.0 已发布** —— 以大规模重构为主线，统一 CLI/WASM 核心链路
- 核心定位：**无损打包**（SPZ 压缩流原封不动存入 GLB）
- 重点增强：WASM 内存与 API 能力（预分配、显式释放、统计与双档配置）
- 双端协同：按场景分工 —— 浏览器侧负责轻量预览/快速校验，本地 CLI 负责重任务转换/批处理/深度验证
- 验证闭环：内置三层验证（结构/无损/解码一致性）+ 云端 browser smoke

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

## 核心特性

- **无损打包**: SPZ 压缩流原封不动存入 GLB，100% 字节级保真
- **SPZ_2 扩展**: 使用 `KHR_gaussian_splatting_compression_spz_2` 标准扩展
- **大规模重构（v2.0）**: 统一 CLI/WASM 核心链路，减少双端分叉
- **WASM 增强**: 预分配输入、显式输出释放、内存统计、compat/perf-lite 双档
- **双端协同（双场景分工）**: 网页侧轻量交互与快速反馈；本地 CLI 侧重批处理、大文件与重验证
- **三层验证**: 结构验证 / 无损验证 / 解码一致性验证
- **跨平台**: Windows、Linux、macOS (x64 + ARM)
- **零依赖运行时**: C++17 + WASM，无额外运行时依赖

## 与 `splat-transform` 的对比

> 说明：这里强调的是**工具定位差异**，不是绝对优劣判断。

### 核心差异一句话

- **`spz2glb`**：把 SPZ **无损打包**为 GLB（SPZ 压缩流原封不动存入 GLB）
- **`splat-transform`**：读取 SPZ 后**解压并重建**为完整高斯数据，再写入 GLB（非无损打包）

### 详细对比

| 维度 | `spz2glb` (v2.0) | `splat-transform` (v1.10.1) |
|------|------------------|-----------------------------|
| **核心定位** | **无损打包 SPZ→GLB**（保持 SPZ 压缩流完整） | **数据变换/重建工具**（支持多种格式互转与编辑） |
| **SPZ 处理方式** | 不解压 SPZ，直接作为二进制流打包进 GLB | 读取 SPZ → 解压为完整高斯数据 → 重建 GLB |
| **GLB 产物** | 使用 `KHR_gaussian_splatting_compression_spz_2` 扩展，内含原始 SPZ 压缩流 | 使用标准 `KHR_gaussian_splatting` 扩展，内含解压后的高斯属性数据 |
| **数据保真** | 100% 无损（SPZ 字节级原样保留） | 解压后重建，经历编解码转换 |
| **功能范围** | 专注 SPZ↔GLB 转换与验证 | 支持 PLY/SOG/SPZ/KSPLAT/SPLAT 等格式的读取、变换、过滤、合并、生成 |
| **运行环境** | C++17 + WASM，无运行时依赖 | TypeScript/Node.js，依赖 WebGPU 进行 SOG 压缩 |
| **WASM 能力** | 预分配输入、显式释放、内存统计、双档配置 | 浏览器/Node 双端，但非以发布级内存治理为核心 |
| **验证闭环** | 内置三层验证（结构/无损/解码一致性）+ 云端 browser smoke | 单元测试 + fixture 验证 |

### 适用场景建议

| 场景 | 推荐工具 |
|------|----------|
| 需要把 SPZ **无损嵌入** GLB，保持原始压缩率 | `spz2glb` |
| 需要在 GLB 中直接存储可渲染的高斯数据（非压缩流） | `splat-transform` |
| 需要做 splat 变换、过滤、合并、生成等编辑操作 | `splat-transform` |
| 需要跨多格式（SOG/KSPLAT/SPLAT 等）批量处理 | `splat-transform` |
| 需要“网页轻量 + 本地重任务”的双场景协同与发布级验证闭环 | `spz2glb` |

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

## WebAssembly 构建

### 构建 WASM 版本

```bash
# 安装 Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

# 构建 WASM 模块
cd tools/spz_to_glb
emcmake cmake -B build_wasm -DSPZ2GLB_BUILD_WASM=ON -DSPZ2GLB_USE_EMSCRIPTEN_ZLIB=ON
emmake cmake --build build_wasm --config Release --target spz2glb-wasm
emmake cmake --build build_wasm --config Release --target spz_verify-wasm

# 输出在 build_wasm/dist/
# - spz2glb.js, spz2glb.wasm, spz2glb.data
# - spz_verify.js, spz_verify.wasm, spz_verify.data
```

### Web 使用

**重要**：WASM 版本需要下载所有文件：
- `spz2glb.js`
- `spz2glb.wasm`
- `spz2glb.data`

将它们放在同一目录，通过 HTTP 服务器加载。

### JavaScript API

```javascript
// 加载模块
const Module = await createSpz2GlbModule();

// 转换 SPZ 到 GLB
const spzBuffer = new Uint8Array([...]);  // 你的 SPZ 文件数据
const glbBuffer = Module.convertSpzToGlb(spzBuffer);

if (glbBuffer) {
    // 成功：glbBuffer 是 Uint8Array
    console.log('转换成功！');
} else {
    // 失败
    console.error('转换失败');
}

// 获取内存统计（可选）
const stats = Module.getMemoryStats();
console.log(`峰值内存: ${stats.peak_usage / 1024 / 1024} MB`);
```

### spz_verify JavaScript API

```javascript
const verifyModule = await createSpzVerifyModule();

// 第 1 层：GLB 结构验证
const layer1Result = verifyModule.layer1ValidateGlbStructure(glbBuffer);

// 第 2 层：二进制无损验证
const layer2Result = verifyModule.layer2ValidateLossless(spzBuffer, glbBuffer);

// 第 3 层：解码一致性验证
const layer3Result = verifyModule.layer3ValidateDecoding(spzBuffer, glbBuffer);
```

### WASM 内存配置

| 档位 | INITIAL_MEMORY | ALLOW_MEMORY_GROWTH | MAXIMUM_MEMORY | 说明 |
|------|----------------|---------------------|----------------|------|
| `compat` | 64MB | `1` | 1GB | 兼容性优先，适配更广设备 |
| `perf-lite` | 128MB | `0` | N/A | 轻中型输入稳定内存上限 |

### 智能内存分配（浏览器 + WASM）

该项目的“智能内存分配”是三层协同，不是单一固定阈值：

1. **设备分档**：基于 `navigator.deviceMemory`、`hardwareConcurrency`、UA 判断设备档位（low/medium/high）。
2. **文件预算**：按设备档位和浏览器可用堆上限，计算 `recommendedMaxFileSize` 与 `hardMaxFileSize`（例如 high 档默认到 64MB）。
3. **WASM 侧预分配**：转换前调用 `reserve_input` 预留整块输入区，前端分块写入该区，再执行 `convert_reserved_input`，输出由显式 `release_output` 回收。

这套机制的目标是：在不同设备上优先保证稳定性（避免 OOM/卡死），同时保留可观测指标（当前/峰值内存、分配失败、工作区使用）。

### 为什么默认不启用 WASM64（memory64）

当前默认不启用 `WASM64`，主要是工程稳定性与兼容性考虑：

- **ABI 约束**：现有 JS 绑定和 C API 以 32 位 `size_t` 为契约（`size_t == 4`），直接切到 memory64 会破坏现有 ABI。
- **浏览器/工具链一致性**：`memory64` 生态仍在推进中，不同浏览器版本与工具链组合的一致性不如 wasm32 稳定。
- **当前需求匹配**：本项目网页端定位是轻量预览与快速转换，默认阈值与 1GB 兼容档已覆盖当前目标场景。

如后续需要超大文件网页端处理，可在保持 ABI 兼容的前提下评估单独的 WASM64 构建产物。

### 性能优化

WASM 构建包含以下优化：
- **-O3 + 严格告警门禁**：优化构建并保持 warning clean
- **-fno-exceptions**：无异常开销
- **compat/perf-lite 双档**：按运行目标配置内存行为
- **内存池**：bump allocator 快速分配
- **热点对象池**：固定大小对象复用

> 示例：`dunhuang_000000.spz`（24.78 MB）在网页端转换成功，耗时约 `506 ms`，峰值内存约 `49.56 MB`。

![浏览器端转换成功截图](./docs/examples/images/dunhuang_000000_spz_web_success.png)

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
├── CMakeLists.txt              # 构建配置
├── LICENSE                     # MIT 许可证
├── README.md / README-zh.md    # 文档
├── src/
│   ├── spz2glb_core.cpp/.h     # 核心转换逻辑（v2.0 统一入口）
│   ├── spz2glb_wasm_c_api.cpp/.h  # WASM C API（预分配/释放/统计）
│   ├── memory_pool.cpp/.h      # 内存池与热点对象池
│   ├── spz_to_glb.cpp          # CLI 主入口
│   ├── spz_verify.cpp          # 验证工具主入口
│   ├── spz_verifier.cpp/.h     # 三层验证实现
│   └── base64.{h,cpp}          # Base64 编解码
├── third_party/                # 定制版 fastgltf + simdjson
│   ├── include/fastgltf/
│   ├── src/
│   └── deps/simdjson/         # simdjson v4.3.1 (内置)
├── tests/                      # 测试脚本与用例
└── .github/workflows/          # CI/CD 工作流
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

## 作者与版权

- 版权所有者：**Pu Junhan**
- 起始年份：**2026**

## 独立作品声明

本项目为作者以个人身份独立开发，不隶属于任何院校、机构或雇主。

本项目依赖以下公开技术规范：
- SPZ 文件格式（Niantic, Inc.）
- glTF 2.0 / KHR_gaussian_splatting（Khronos Group）
- KHR_gaussian_splatting_compression_spz_2 扩展草案（SPZ 生态公开规范）

本项目与 Niantic, Inc. 及其关联方不存在隶属、代言或关联关系。

## 许可证

MIT License - 详见 [LICENSE](LICENSE)

## 生态定位

`spz2glb` 是 **SPZ 生态下游项目**，与上游 `spz_gatekeeper`（门卫）的关系如下：

- **门卫（上游）**：负责 SPZ 格式合法性、扩展兼容性与治理规范（L2 校验、TLV 扩展登记、合规审查）。
- **`spz2glb`（下游）**：负责将合规的 SPZ 文件无损打包为 GLB，遵循门卫给出的兼容性约束。

> 一句话：**门卫管“准入与标准”，`spz2glb` 管“转换与交付”。**

相关项目：

- [spz_gatekeeper](https://github.com/spz-ecosystem/spz_gatekeeper) - SPZ 门卫：格式合法性校验与生态治理
- [fastgltf](https://github.com/spnda/fastgltf) - 高性能 glTF 库（作者：Sean Apeler，MIT 许可证）
- [simdjson](https://github.com/simdjson/simdjson) - 极速 JSON 解析库 v4.3.1
- [KHR_gaussian_splatting](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_gaussian_splatting) - Khronos Gaussian Splatting 扩展

## 定制说明

本项目使用 **定制版 fastgltf**，包含以下修改：

1. **simdjson v4.3.1 内置**: 不查找系统库，不从网络下载，使用内置源码
2. **KHR_gaussian_splatting_compression_spz_2 扩展**: 支持 SPZ_2 压缩格式
3. **一键编译**: 只需 `cmake && cmake --build`，无需手动配置依赖

# spz2glb - SPZ to GLB Converter

**无损打包 SPZ 为 GLB 格式** —— 保持 SPZ 压缩流完整，支持“网页轻量处理 + 本地重任务”双场景协同。

## 发布状态

- 当前稳定版本线：**v2.x**（具体版本请以 [Releases](https://github.com/spz-ecosystem/spz2glb/releases) 与仓库 tag 为准）
- 核心定位：**无损打包**（SPZ 压缩流原封不动存入 GLB）
- 重点增强：WASM 内存与 API 能力（预分配、显式释放、统计与双档配置）
- 双端协同：按场景分工 —— 浏览器侧负责轻量预览/快速校验，本地 CLI 负责重任务转换/批处理/深度验证
- 验证闭环：内置五层验证（结构/无损/解码一致性/元数据一致性/ILV 扩展完整性）+ 云端 browser smoke

## 职责边界（固定）

- `spz2glb` 只负责两件事：**SPZ→GLB 格式封装**与 **GLB 分发交付链路**。
- `spz2glb` 不承担压缩算法研发、渲染引擎能力扩展、通用 3D 编辑流水线等超出边界的职责。
- GLB 合规与正确性判定统一由**五层验证**负责（结构验证 / 无损验证 / 解码一致性验证 / 元数据一致性验证 / ILV 扩展完整性验证）。
- Web 侧默认面向轻量单文件演示；批量与重任务属于 CLI 路径，不在 Web 默认职责内。

## 核心特性

- **无损打包**: SPZ 压缩流原封不动存入 GLB，100% 字节级保真
- **SPZ_2 扩展**: 使用 `KHR_gaussian_splatting_compression_spz_2` 标准扩展
- **大规模重构（v2.0.3）**: 统一 CLI/WASM 核心链路，减少双端分叉
- **WASM 增强**: 预分配输入、显式输出释放、内存统计、compat/perf-lite 双档
- **双端协同（双场景分工）**: 网页侧轻量交互与快速反馈；本地 CLI 侧重批处理、大文件与重验证
- **五层验证**: 结构验证 / 无损验证 / 解码一致性验证 / 元数据一致性验证 / ILV 扩展完整性验证
- **跨平台**: Windows、Linux、macOS (x64 + ARM)
- **零依赖运行时**: C++17 + WASM，无额外运行时依赖

## 扩展支持状态

### `KHR_gaussian_splatting` 扩展
- **状态**：`KHR_gaussian_splatting` 已合入 glTF 仓库，但**尚未正式定稿**。
- **`KHR_gaussian_splatting_compression_spz_2`**：当前仍明确处于**草案阶段**。
- **当前实现**：
  - ✅ **导出**：会写出完整扩展链，包含外层 `KHR_gaussian_splatting` 与内层 `KHR_gaussian_splatting_compression_spz_2`。
  - ⚠️ **解析**：对草案扩展采用**安全忽略**策略，这是当前阶段的预期回退行为。
  - 🚫 **不硬编码 `Extensions` 枚举项**：不会把草案扩展直接写死进头文件枚举，避免过早锁定未定稿接口。
- **后续变化**：等 Khronos 侧定义正式定稿后，再补齐完整的解析期支持。

### 编译控制
- **CMake 选项**：`ENABLE_KHR_GAUSSIAN_SPLATTING`（默认：`ON`）控制是否在导出侧写入扩展数据。
- **关闭后的行为**：转换器仍可运行，但不会在输出 GLB 中写入 Gaussian Splatting 扩展数据。

### ILV 003 坐标系扩展
- **扩展 ID**: `0xADBE0003`
- **用途**: 在 SPZ ILV（Information-Label-Value）记录中映射 coordinateSystem（uint32）元数据。
- **实现**: spz2glb 读取并透传 003 元数据作为描述符（而非指令），坐标转换由渲染器的 `coordinateConverter()` 处理。
- **验证**: Layer 5（ILV 扩展完整性）校验 TLV 记录结构并强制 003 值域 [0, 16]。

## 与 `splat-transform` 的对比

> 说明：这里强调的是**工具定位差异**，不是绝对优劣判断。

| 维度 | `spz2glb` (v2.0.3) | `splat-transform` (v3.1.7) |
|------|-------------------|-----------------------------|
| **开发者** | 独立开发者（Pu Junhan） | PlayCanvas |
| **核心定位** | **无损 SPZ→GLB 打包**（SPZ 压缩流原封不动存入 GLB） | **多格式 splat 转换与编辑**（解压-重建管线） |
| **语言** | C++17 + WASM | TypeScript (ESM/CJS 双入口) |
| **GLB 产物** | `KHR_gaussian_splatting_compression_spz_2` 扩展（原始 SPZ 流，字节级一致） | 标准 `KHR_gaussian_splatting` 扩展（解压后的 float32 属性） |
| **`spz_2` 扩展** | ✅ 支持（嵌套在 KHR_gaussian_splatting 内） | ❌ 不支持 |
| **SPZ 处理** | 不解压，直接二进制流存入 GLB | 纯 JS 读取器（v2-v4, gzip/zstd）+ @adobe/spz WASM 写入器 |
| **数据保真** | 100% 无损（SPZ 字节级原样保留在 GLB） | 解压-重建循环，浮点精度因属性编码而异 |
| **功能范围** | 专注 SPZ↔GLB 转换 + 五层验证 | 9 种输入格式、12 种输出格式、变换/过滤/合并/简化/生成 |
| **流式处理** | 基于文件（加载完整 SPZ，输出完整 GLB） | ChunkSource 流式管线（处理 1 亿+ 点无需完整加载） |
| **GPU 加速** | N/A（C++ CPU，无 GPU 依赖） | WebGPU 用于 SOG 压缩、体素化、渲染 |
| **运行时依赖** | 无（独立二进制 + WASM） | Node.js ≥22、@adobe/spz WASM、webgpu |
| **WASM 能力** | 预分配输入、显式释放、内存统计、双档配置 | 仅通过 @adobe/spz 写入 SPZ（读取为纯 JS） |
| **验证闭环** | 内置五层验证（结构/无损/解码/元数据/ILV）+ CI browser smoke | 45+ 测试文件（格式往返、GLB 合规、CLI） |
| **跨平台** | Windows/Linux/macOS (x64 + ARM) 原生二进制 | 跨平台（Node.js） |
| **许可证** | MIT | MIT |

### 适用场景建议

| 场景 | 推荐工具 |
|------|----------|
| 需要**无损嵌入** SPZ 到 GLB，保持原始压缩流按字节不变 | `spz2glb` |
| 需要 GLB 中直接存储可渲染的 float32 高斯属性（无需 SPZ 解码器） | `splat-transform` |
| 需要多格式批量处理（PLY/SOG/SPLAT/KSPLAT/SPZ/CSV/HTML/Voxel） | `splat-transform` |
| 需要流式/超大场景处理（1 亿+ 点） | `splat-transform` |
| 需要 splat 变换、过滤、合并、简化、生成 | `splat-transform` |
| 需要 GPU 加速操作（SOG 压缩、体素化） | `splat-transform` |
| 需要专注、轻量的转换器与发布级验证闭环 | `spz2glb` |

## 基本转换

```bash
# 转换 SPZ 到 GLB
spz2glb model.spz model.glb
```

### 五层验证

```bash
# 运行所有验证（提供你自己的 SPZ 和 GLB 文件）
spz_verify all input.spz output.glb

# 输出：
# Layer 1: GLB Structure & SPZ_2 Specification Validation - PASSED (7/7)
# Layer 2: Binary Lossless Verification - PASSED (byte-identical)
# Layer 3: Decoding Consistency Verification - PASSED (Size match)
# Layer 4: Metadata Consistency Verification - PASSED (SPZ↔GLB metadata)
# Layer 5: ILV Extension Integrity Verification - PASSED (TLV structure)
# [SUCCESS] All 5 verifications PASSED!
```

> **注意**: 请使用你实际构建产物中的可执行文件路径（例如 `build/spz2glb` / `build/spz_verify`），或先加入 `PATH`。

### 批量处理

```bash
# 批量转换所有 SPZ 文件
for file in *.spz; do
    spz2glb "$file" "${file%.spz}.glb"
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

# 3. 运行（使用实际二进制路径或 PATH 命令）
spz2glb input.spz output.glb
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
spz2glb <input.spz> <output.glb> [--verify]
```

**标志**：

| 标志 | 说明 |
|------|------|
| `--verify` | 转换完成后立即运行五层验证（内部调用 spz_verify） |

**完整示例**：

```bash
# 转换单个文件
spz2glb model.spz model.glb

# 转换并验证
spz2glb model.spz model.glb --verify

# 批量转换
for file in *.spz; do
    spz2glb "$file" "${file%.spz}.glb"
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

### 五层验证工具 (spz_verify)

> **重要说明**:
> - **独立工具**: spz_verify 是独立的验证工具，不是生产转换流程的一部分
> - **开发/测试用途**: 设计用于质量保证、调试和测试工作流
> - **日常使用不需要**: 一旦转换被验证，生产环境只需要 spz2glb
> - **Layer 2 进行字节级比较**: Layer 2 验证会从 GLB 中提取 SPZ 负载，并与原始 SPZ 文件进行逐字节比较（比 Layer 1/3/4/5 慢）

```bash
spz_verify <command> [options]
```

**命令**：

```bash
# 运行全部五层验证
spz_verify all <input.spz> <output.glb>

# 单独运行某层验证
spz_verify layer1 <output.glb>              # GLB 结构验证 (快速)
spz_verify layer2 <input.spz> <output.glb>  # 二进制无损验证 (逐字节对比, 较慢)
spz_verify layer3 <input.spz> <output.glb>  # 解码一致性验证 (快速)
spz_verify layer4 <input.spz> <output.glb>  # 元数据一致性验证 (快速)
spz_verify layer5 <input.spz>              # ILV 扩展完整性验证 (快速)
```

**完整示例**：

```bash
# 1. 转换文件
spz2glb model.spz model.glb

# 2. 运行所有验证
spz_verify all model.spz model.glb

# 或者单独验证
spz_verify layer1 model.glb
spz_verify layer2 model.spz model.glb
spz_verify layer3 model.spz model.glb
spz_verify layer4 model.spz model.glb
spz_verify layer5 model.spz
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

Layer 2: Payload Extraction & Byte Equality
  ✓ SPZ input bytes: 15728640
  ✓ Extracted bytes: 15728640
  ✓ Extracted payload is byte-identical to input SPZ
  [PASS] Layer 2 validation passed

Layer 3: Decode Consistency Validation
  ✓ GLB structure valid
  ✓ Extension integrity check passed
  [PASS] Layer 3 validation passed

Layer 4: GLB Extension Metadata vs SPZ Header Consistency
  ✓ SPZ version consistent: 2
  ✓ GLB coordinateSystem recorded in metadata
  [PASS] Layer 4 validation passed

Layer 5: ILV Extension Completeness
  ✓ ILV 0xADBE0003 coordinateSystem=1 (valid, range [0,16])
  ✓ All ILV records pass integrity checks
  [PASS] Layer 5 validation passed

[SUCCESS] All 5 layers validation passed!
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
SPZ2GLB="spz2glb"
VERIFY="spz_verify"

echo "=== SPZ to GLB Conversion & Verification ==="
echo "Input:  $INPUT"
echo "Output: $OUTPUT"
echo ""

# Step 1: Convert
echo "[1/2] Converting SPZ to GLB..."
$SPZ2GLB "$INPUT" "$OUTPUT"
echo ""

# Step 2: Verify
echo "[2/2] Running 5-layer verification..."
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

echo [2/2] Running 5-layer verification...
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
./emsdk install 6.0.3
./emsdk activate 6.0.3
source ./emsdk_env.sh

# 构建 WASM 模块
cd tools/spz_to_glb
emcmake cmake -B build_wasm -DSPZ2GLB_BUILD_WASM=ON
emmake cmake --build build_wasm --config Release --target spz2glb-wasm
emmake cmake --build build_wasm --config Release --target spz_verify-wasm

# 输出在 build_wasm/dist/（具体文件取决于 profile/工具链）
# - spz2glb.js, spz2glb.wasm
# - spz_verify.js, spz_verify.wasm
# - 可能出现附加侧文件；部署时请保持同一次构建产物一致
```

### Web 使用

**重要**：请确保 `spz2glb.js` 与 `spz2glb.wasm` 来自同一次构建并放在同一目录，通过 HTTP 服务器加载。若构建产物包含附加侧文件，也应按同一版本集一起部署。

### JavaScript API

```javascript
import { loadSpz2Glb } from './spz2glb_bindings.js';

const api = await loadSpz2Glb('./spz2glb.wasm');
const result = api.convert(spzUint8Array);

if (!result) throw new Error('转换失败');

const glbBytes = result.bytes;        // 指向 WASM 内存的 Uint8Array 视图
const glbBlob = result.toBlob('model/gltf-binary');
const stats = api.getMemoryStats();
console.log('峰值内存(MB):', (stats.peakUsageBytes / 1024 / 1024).toFixed(2));

result.release(); // 必须释放 WASM 输出缓冲
```

> 说明：`spz_verify` 当前主要作为 CLI/WASM 产物用于验证流程，README 不再声明稳定的浏览器 JS API 接口面。

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
- **-Oz**：WASM 特定尺寸优化（减小 .wasm 二进制体积）
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
- ZSTD（v4 SPZ 格式支持）

**依赖说明**：

| 工具 | 依赖 | 用途 |
|------|------|------|
| spz2glb | ZLIB, ZSTD, fastgltf, simdjson | SPZ 转 GLB |
| spz_verify | ZLIB, ZSTD | 五层验证 |

## 项目结构

```
spz2glb/
├── CMakeLists.txt              # 构建配置
├── LICENSE                     # MIT 许可证
├── README.md / README-zh.md    # 文档
├── src/
│   ├── spz2glb_core.cpp/.h     # 核心转换逻辑（v2.0.3 统一入口）
│   ├── spz2glb_wasm_c_api.cpp/.h  # WASM C API（预分配/释放/统计）
│   ├── memory_pool.cpp/.h      # 内存池与热点对象池
│   ├── spz_to_glb.cpp          # CLI 主入口
│   ├── spz_verify.cpp          # 验证工具主入口
│   ├── spz_verifier.cpp/.h     # 五层验证实现
│   └── base64.{h,cpp}          # Base64 编解码
├── third_party/                # 定制版 fastgltf + simdjson
│   ├── include/fastgltf/
│   ├── src/
│   └── deps/simdjson/         # simdjson v4.3.1 (内置)
├── tests/
│   ├── gen_fixture.mjs           # 合成夹具生成器
│   ├── data/
│   │   └── bench/                # 基准测试数据集
│   └── ...                       # 测试脚本与用例
├── scripts/
│   ├── wasm-pre-check.sh         # WASM 预构建环境检查
│   └── ...                       # 实用脚本
└── .github/workflows/            # CI/CD 工作流
```

## 测试数据

`tests/` 目录包含：

- **合成夹具**（`tests/gen_fixture.mjs`）：生成最小有效 SPZ 文件用于单元测试，覆盖 v3/v4 SPZ 变体及边界情况（空文件、截断头部、畸形扩展）。v2 格式支持需要扩展生成器（当前未实现）。
- **基准数据集**（`tests/data/bench/`）：一组不同大小和压缩配置的代表性 SPZ 文件，用于性能基准测试和回归测试。

两者均可重新生成，不捆绑真实用户数据。

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

### GLB 结构示例

```
GLB Header (12 bytes)
├── magic: 0x46546C67 ("glTF")
├── version: 2
└── length: total file size

JSON Chunk
├── chunkLength
├── chunkType: 0x4E4F534A ("JSON")
└── glTF JSON（按 4 字节对齐）
    └── 外层 `KHR_gaussian_splatting` 扩展
        └── 内层 `KHR_gaussian_splatting_compression_spz_2` 扩展

BIN Chunk
├── chunkLength
├── chunkType: 0x004E4942 ("BIN\0")
└── 原始 SPZ 压缩数据
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

## 引用

如在研究中使用本项目，请引用：

> Pu Junhan. Zero-Trust HL Harness: Governing Autonomous Agents through Self-Referential Evolution. 中国科学院科技论文预发布平台, ChinaXiv:202607.00158V1.

## 许可证

MIT License - 详见 [LICENSE](LICENSE)

## 生态定位

`spz2glb` 是 **SPZ 生态下游项目**，与上游 `spz_gatekeeper`（门卫）的关系如下：

- **门卫（上游）**：负责 SPZ 格式合法性、扩展兼容性与治理规范（L2 校验、TLV 扩展登记、合规审查）。
- **`spz2glb`（下游）**：负责将合规的 SPZ 文件无损打包为 GLB，遵循门卫给出的兼容性约束。

> 一句话：**门卫管“准入与标准”，`spz2glb` 管“转换与交付”。**

相关项目：

- [spz_gatekeeper](https://github.com/spz-ecosystem/spz_gatekeeper) - SPZ 门卫：格式合法性校验与生态治理
- [spz-anime-text2scene-bench](https://github.com/spz-ecosystem/spz-anime-text2scene-bench) - 动漫风格文生场景基准数据集（SPZ 格式）
- [fastgltf](https://github.com/spnda/fastgltf) - 高性能 glTF 库（作者：Sean Apeler，MIT 许可证）
- [simdjson](https://github.com/simdjson/simdjson) - 极速 JSON 解析库 v4.3.1
- [KHR_gaussian_splatting](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_gaussian_splatting) - Khronos Gaussian Splatting 扩展

## 定制说明

本项目使用 **定制版 fastgltf**，包含以下修改：

1. **simdjson v4.3.1 内置**: 不查找系统库，不从网络下载，使用内置源码
2. **KHR_gaussian_splatting_compression_spz_2 扩展**: 支持 SPZ_2 压缩格式
3. **一键编译**: 只需 `cmake && cmake --build`，无需手动配置依赖

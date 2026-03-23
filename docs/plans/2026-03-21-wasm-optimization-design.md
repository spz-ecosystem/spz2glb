# SPZ2GLB WASM 工程优化设计方案

**日期**: 2026-03-21
**版本**: v1.0
**状态**: 设计中

---

## 1. 背景与目标

### 1.1 项目现状

spz2glb 是一个纯前端的 WASM 工具，用于将 SPZ 格式转换为 GLB 格式。当前 WASM 构建存在以下问题：

| 问题类别 | 具体问题 |
|---------|---------|
| 编译优化 | 缺失 C++ 编译时优化 (-O3 -flto) |
| 链接优化 | 缺失链接时优化 (LTO) |
| 内存配置 | 无内存限制配置 |
| 异常处理 | 未禁用异常捕获 |
| 文件系统 | 未禁用不必要的文件系统支持 |
| 部署架构 | 只有 spz2glb，缺少 spz_verify |

### 1.2 设计目标

1. **工程级别优化**：让 WASM 构建达到生产级别
2. **支持全场景**：A/B/C/D 四种文件大小场景（100MB - 1GB+）
3. **三个独立模块**：SPZ 编码器 + spz2glb + spz_verify
4. **现代浏览器兼容**：Chrome 94+, Firefox 94+, Safari 16.4+, Edge 94+
5. **完整 CI/CD**：GitHub Actions 完整矩阵

---

## 2. 架构设计

### 2.1 三个独立 WASM 模块

```
┌─────────────────────────────────────────────────────────────┐
│                    浏览器环境                                │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │
│  │  SPZ WASM    │  │ spz2glb WASM │  │spz_verify    │    │
│  │  (编码/解码)  │  │  (转换器)     │  │   WASM       │    │
│  └──────────────┘  └──────────────┘  └──────────────┘    │
├─────────────────────────────────────────────────────────────┤
│                    JavaScript 胶水层                        │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 模块职责

| 模块 | 输入 | 输出 | 依赖 |
|------|------|------|------|
| SPZ WASM | PLY/二进制 | SPZ 二进制 | zlib |
| spz2glb WASM | SPZ 二进制 | GLB 二进制 | fastgltf, zlib |
| spz_verify WASM | SPZ + GLB | 验证结果 | fastgltf |

### 2.3 部署策略

- **独立部署**：每个模块独立版本迭代
- **按需加载**：用户只加载需要的模块
- **无预览**：只做数据转换，渲染交给专业渲染器

---

## 3. WASM 构建优化

### 3.1 编译优化配置

```cmake
# C++ 编译时优化
target_compile_options(spz2glb-wasm PRIVATE
  "-O3"        # 最高优化级别
  "-flto"      # 链接时优化
  "-fno-exceptions"  # 禁用异常
  "-fno-rtti"        # 禁用 RTTI
)
```

### 3.2 链接优化配置

```cmake
# Emscripten 链接选项
target_link_options(spz2glb-wasm PRIVATE
  # 优化级别
  "-O3"
  "-flto"

  # ZLIB 支持
  "-sUSE_ZLIB=1"
  "--use-port=zlib"

  # Embind
  "--bind"

  # 模块化
  "-sMODULARIZE=1"
  "-sEXPORT_NAME=createSpz2GlbModule"

  # 内存配置
  "-sALLOW_MEMORY_GROWTH=1"
  "-sINITIAL_MEMORY=33554432"    # 32MB 初始
  "-sMAXIMUM_MEMORY=1073741824"   # 1GB 最大
  "-sSTACK_SIZE=10485760"         # 10MB 栈

  # Web 环境
  "-sENVIRONMENT=web"
  "-sEXPORT_ES6=1"

  # 文件系统（禁用）
  "-sFILESYSTEM=0"

  # 生产环境优化
  "-sASSERTIONS=0"
  "-sNO_EXIT_RUNTIME=1"
  "-sNO_DYNAMIC_EXECUTION=1"
  "-sERROR_ON_UNDEFINED_SYMBOLS=1"
  "-sWARN_ON_UNDEFINED_SYMBOLS=1"

  # 性能优化
  "-sWASM_BIGINT=1"
  "-sDISABLE_EXCEPTION_CATCHING=1"
  "-sDISALLOW_EXCEPTION_THROWING=1"
  "-sMALLOC=emmalloc"
  "-sOPTIMIZE=3"

  # 文件拆分（SINGLE_FILE=0 减小 30%）
  "-sSINGLE_FILE=0"
)
```

### 3.3 内存管理策略

#### 自适应内存分配

| 文件大小 | 策略 | 初始内存 | 最大内存 |
|---------|------|---------|---------|
| A. < 100MB | 一次性加载 | 32MB | 256MB |
| B. 100-500MB | 智能预分配 | 64MB | 1GB |
| C. 500MB-1GB | 渐进扩容 | 64MB | 2GB |
| D. > 1GB | 流式处理 | 128MB | 4GB |

#### 渐进式扩容代码示例

```cpp
// 智能内存检测
size_t detectOptimalMemory() {
  // 检测浏览器可用内存
  size_t available = EmscriptenSystem::getAvailableMemory();

  // 根据可用内存和文件大小选择策略
  if (fileSize < 100 * 1024 * 1024) {
    return 256 * 1024 * 1024;  // 256MB
  } else if (fileSize < 500 * 1024 * 1024) {
    return 1 * 1024 * 1024 * 1024;  // 1GB
  } else {
    return std::min(available, 4LL * 1024 * 1024 * 1024);  // 4GB 或可用内存
  }
}
```

---

## 4. 三个 WASM 模块详细设计

### 4.1 模块清单

| 模块名 | 源文件 | Embind 导出 | 输出文件 |
|--------|--------|------------|---------|
| spz_encoder-wasm | spz_encoder.cpp | encode(), decode() | spz_encoder.js + .wasm |
| spz2glb-wasm | spz_to_glb.cpp | convertSpzToGlb() | spz2glb.js + .wasm |
| spz_verify-wasm | spz_verify.cpp | layer1/2/3Validate() | spz_verify.js + .wasm |

### 4.2 Embind 导出接口

#### spz2glb Module

```cpp
#include <emscripten/bind.h>
#include "spz_to_glb.h"

using namespace emscripten;

EMSCRIPTEN_BINDINGS(spz2glb) {
  class_<Spz2GlbModule>("Spz2GlbModule")
    .constructor<>()
    .function("convertSpzToGlb", &convertSpzToGlb)
    .function("getMemoryUsage", &getMemoryUsage)
    .function("getVersion", &getVersion);

  function("createSpz2GlbModule", &createSpz2GlbModule);
}
```

#### spz_verify Module

```cpp
#include <emscripten/bind.h>
#include "spz_verify.h"

using namespace emscripten;

EMSCRIPTEN_BINDINGS(spz_verify) {
  class_<SpzVerifyModule>("SpzVerifyModule")
    .constructor<>()
    .function("layer1ValidateGlbStructure", &layer1ValidateGlbStructure)
    .function("layer2ValidateLossless", &layer2ValidateLossless)
    .function("layer3ValidateDecoding", &layer3ValidateDecoding)
    .function("validateAll", &validateAll);

  function("createSpzVerifyModule", &createSpzVerifyModule);
}
```

### 4.3 TypeScript 类型定义

```typescript
// spz2glb.d.ts
export interface Spz2GlbModule {
  convertSpzToGlb(spzData: Uint8Array): Uint8Array | null;
  getMemoryUsage(): { current: number; peak: number; max: number };
  getVersion(): string;
}

export default function createSpz2GlbModule(): Promise<Spz2GlbModule>;
```

```typescript
// spz_verify.d.ts
export interface SpzVerifyModule {
  layer1ValidateGlbStructure(glbData: Uint8Array): ValidationResult;
  layer2ValidateLossless(spzData: Uint8Array, glbData: Uint8Array): ValidationResult;
  layer3ValidateDecoding(spzData: Uint8Array, glbData: Uint8Array): ValidationResult;
  validateAll(spzData: Uint8Array, glbData: Uint8Array): AllLayersResult;
}

export interface ValidationResult {
  passed: boolean;
  layer: 1 | 2 | 3;
  message?: string;
  details?: Record<string, any>;
}

export interface AllLayersResult {
  overall: boolean;
  layers: ValidationResult[];
  totalTime: number;
}

export default function createSpzVerifyModule(): Promise<SpzVerifyModule>;
```

---

## 5. CI/CD 配置

### 5.1 GitHub Actions Workflow

```yaml
name: Release

on:
  push:
    branches:
      - main
    tags:
      - 'v*'
  workflow_dispatch:

jobs:
  # 桌面版构建矩阵
  build:
    strategy:
      fail-fast: false
      matrix:
        include:
          - os: windows-latest
            artifact: spz2glb-windows-x64.exe
          - os: ubuntu-latest
            artifact: spz2glb-linux-x64
          - os: macos-latest
            artifact: spz2glb-macos-x64
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v5
        with:
          submodules: recursive

      - name: Install dependencies (Ubuntu)
        if: matrix.os == 'ubuntu-latest'
        run: sudo apt-get update && sudo apt-get install -y zlib1g-dev

      - name: Install dependencies (macOS)
        if: matrix.os == 'macos-latest'
        run: brew install zlib

      - name: Install dependencies (Windows)
        if: matrix.os == 'windows-latest'
        run: |
          git clone https://github.com/microsoft/vcpkg.git C:/vcpkg --depth 1
          C:/vcpkg/bootstrap-vcpkg.bat -disableMetrics
          C:/vcpkg/vcpkg install zlib:x64-windows

      - name: Configure
        run: cmake -B build -DCMAKE_BUILD_TYPE=Release

      - name: Build
        run: cmake --build build --config Release -j4

      - name: Upload artifact
        uses: actions/upload-artifact@v5
        with:
          name: ${{ matrix.artifact }}
          path: ${{ matrix.artifact }}

  # WASM 构建（3 个独立模块）
  wasm-build:
    name: Build WASM Modules
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v5
        with:
          submodules: recursive

      - name: Setup Emscripten
        uses: mymindstorm/setup-emsdk@v14
        with:
          version: latest

      - name: Configure spz2glb WASM
        run: |
          cd tools/spz_to_glb
          emcmake cmake -S ../../ -B build_wasm -DSPZ2GLB_BUILD_WASM=ON -DSPZ2GLB_USE_EMSCRIPTEN_ZLIB=ON

      - name: Build spz2glb WASM
        run: |
          cd tools/spz_to_glb
          emmake cmake --build build_wasm --config Release --target spz2glb-wasm

      - name: Build spz_verify WASM
        run: |
          cd tools/spz_to_glb
          emmake cmake --build build_wasm --config Release --target spz_verify-wasm

      - name: Copy outputs
        run: |
          mkdir -p dist
          cp tools/spz_to_glb/dist/*.wasm dist/
          cp tools/spz_to_glb/dist/*.js dist/

      - name: Upload WASM artifacts
        uses: actions/upload-artifact@v5
        with:
          name: wasm-modules
          path: dist/

  release:
    needs: [build, wasm-build]
    runs-on: ubuntu-latest
    if: startsWith(github.ref, 'refs/tags/')

    steps:
      - name: Download all artifacts
        uses: actions/download-artifact@v5
        with:
          path: artifacts

      - name: Create Release
        uses: softprops/action-gh-release@v1
        with:
          files: artifacts/**/*
        env:
          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
```

### 5.2 构建产物命名规范

```
spz2glb/
├── spz2glb-windows-x64.exe    # Windows 桌面版
├── spz2glb-linux-x64          # Linux 桌面版
├── spz2glb-macos-x64           # macOS 桌面版
├── spz2glb.js                  # WASM JS 胶水
├── spz2glb.wasm                # WASM 二进制
├── spz2glb.d.ts                # TypeScript 声明
├── spz_verify.js               # 验证模块 JS
├── spz_verify.wasm             # 验证模块 WASM
└── spz_verify.d.ts            # 验证模块声明
```

---

## 6. 浏览器兼容性

### 6.1 最低版本要求

| 浏览器 | 最低版本 | 关键特性 |
|--------|---------|---------|
| Chrome | 94+ | SIMD, BigInt, 改进 GC |
| Firefox | 94+ | SIMD, BigInt |
| Safari | 16.4+ | WASM 性能改进 |
| Edge | 94+ | Chromium 内核 |

### 6.2 特性检测

```javascript
// 浏览器能力检测
async function detectCapabilities() {
  const capabilities = {
    simd: false,
    bigint: false,
    sharedArrayBuffer: false,
    maxMemory: 0
  };

  // SIMD 检测
  try {
    const wasm = await WebAssembly.instantiate(simpleSimdModule);
    capabilities.simd = true;
  } catch (e) {
    capabilities.simd = false;
  }

  // BigInt 检测
  capabilities.bigint = typeof BigInt64Array !== 'undefined';

  // SharedArrayBuffer 检测（需要 COOP/COEP）
  capabilities.sharedArrayBuffer = typeof SharedArrayBuffer !== 'undefined';

  // 内存检测
  if (navigator.deviceMemory) {
    capabilities.maxMemory = navigator.deviceMemory * 1024 * 1024 * 1024;
  }

  return capabilities;
}
```

### 6.3 回退策略

| 能力缺失 | 回退方案 |
|---------|---------|
| SIMD | 使用标量 WASM（性能降级 2-5x）|
| BigInt | 使用 Number 替代（精度降级）|
| SharedArrayBuffer | 使用 Web Workers + postMessage |

---

## 7. 文件结构

### 7.1 仓库结构

```
spz_ecosystem_simplified/
├── tools/
│   └── spz_to_glb/
│       ├── CMakeLists.txt           # 主构建配置
│       ├── src/
│       │   ├── spz_to_glb.cpp       # 主转换器
│       │   ├── spz_verify.cpp       # 三层验证器
│       │   ├── spz_encoder.cpp       # SPZ 编码器
│       │   ├── emscripten_utils.h   # Emscripten 工具
│       │   └── emscripten/          # TypeScript 类型定义
│       │       ├── spz2glb.d.ts.in
│       │       └── spz_verify.d.ts.in
│       ├── .github/
│       │   └── workflows/
│       │       └── release.yml       # CI/CD 配置
│       └── dist/                     # 构建输出
├── third_party/
│   └── fastgltf/                     # GLTF 解析库
└── docs/
    └── plans/
        └── 2026-03-21-wasm-optimization-design.md
```

### 7.2 构建产物结构

```
dist/
├── desktop/
│   ├── spz2glb-windows-x64.exe
│   ├── spz2glb-linux-x64
│   ├── spz2glb-macos-x64
│   ├── spz_verify-windows-x64.exe
│   ├── spz_verify-linux-x64
│   └── spz_verify-macos-x64
├── wasm/
│   ├── spz2glb/
│   │   ├── spz2glb.js
│   │   ├── spz2glb.wasm
│   │   └── spz2glb.d.ts
│   └── spz_verify/
│       ├── spz_verify.js
│       ├── spz_verify.wasm
│       └── spz_verify.d.ts
└── README.md
```

---

## 8. 实施计划

### Phase 1: CMakeLists.txt 优化
- [ ] 添加 C++ 编译优化 (-O3 -flto)
- [ ] 添加完整链接优化配置
- [ ] 添加 spz_verify-wasm 构建目标
- [ ] 添加 TypeScript 类型生成

### Phase 2: Workflow 修复
- [ ] 修复 WASM 构建路径
- [ ] 添加 spz_verify WASM 构建
- [ ] 修正输出文件列表
- [ ] 测试完整构建流程

### Phase 3: 内存管理
- [ ] 实现智能内存检测
- [ ] 实现渐进式扩容
- [ ] 添加内存使用统计

### Phase 4: 验证与测试
- [ ] 单元测试
- [ ] 集成测试
- [ ] 浏览器兼容性测试

---

## 9. 附录

### 9.1 参考资料

- [Emscripten 优化指南](https://emscripten.org/docs/optimizing-cc/Optimizing-Code.html)
- [WebAssembly 最佳实践](https://web.dev/articles/webassembly-caching-with-workers)
- [fastgltf GitHub](https://github.com/spz-ecosystem/fastgltf)

### 9.2 术语表

| 术语 | 定义 |
|------|------|
| LTO | Link-Time Optimization，链接时优化 |
| SIMD | Single Instruction Multiple Data，单指令多数据 |
| Embind | Emscripten 的 JavaScript/C++ 绑定 |
| COOP/COEP | Cross-Origin-* 安全响应头 |

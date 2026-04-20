# spz2glb 项目演进日志 (Project Evolution Log)

> **维护者**: Pu Junhan  
> **起始日期**: 2026-02-xx  
> **当前稳定版**: v2.0.2  
> **许可证**: MIT  

---

## 概述

本文档记录 `spz2glb` 从 v1.0.0 到 v2.0.2 的完整版本演进历程，包括：
- 每个版本的核心变更与 CI/CD 流水线能力
- 重构决策与动机
- 技术债务处理记录
- 与 Khronos SPZ_2 标准草案的对应关系

**用途**：
- 推动标准草案落地的重要参考材料
- 顶会论文写作的项目背景依据
- 新贡献者的快速上手指南

---

## 版本时间线与 CI/CD 演进总览

```
v1.0.0 ──→ v1.0.1 ──→ v1.0.2 ──→ v1.1.0 ──→ v2.0.0 ──→ v2.0.1 ──→ v2.0.2
 │         │          │          │          │          │          │
 ├ MVP     ├ 验证修复   ├ 文档      ├ WASM      ├ 大重构    ├ 收口      ├ 版本收口
 │ 基础功能  ├ Layer1   ├ 注释      ├ 内存管理   ├ 统一核心  ├ 稳定化    ├ 文档降噪
 │          │          │          ├ 浏览器集成  ├ 三层验证  ├ 演示就绪  ├ 版权头
 ├──────────┤          │          ├ CI 扩展    ├ 发布链    │          │
 │release.yml           │          │          ├ 回滚      │          │
 │3平台构建              │          │          │          │          │
 │(Win/Linux/Mac)       │          │          │          │          │
```

### CI/CD 能力演进表

| 版本 | CI 流水线 | 构建目标 | 平台 | WASM | 测试 | 发布 | 回滚 |
|------|-----------|----------|------|------|------|------|------|
| **v1.0.0** | `release.yml` (v1) | spz2glb + spz_verify | Win/Linux/Mac | ❌ | ❌ | ✅ 3 平台 binary Artifact | ❌ |
| **v1.0.1** | `release.yml` (v1) | 同上 | 同上 | ❌ | ❌ | ✅ 同上 | ❌ |
| **v1.0.2** | `release.yml` (v1) | 同上 | 同上 | ❌ | ❌ | ✅ 同上 | ❌ |
| **v1.1.0** | `release.yml` (v2) + `test-wasm-build.yml` | 同上 + WASM 双 profile | 同上 + Browser | ✅ compat/perf-lite | Playwright smoke | ✅ 同上 + GitHub Pages + Release | ❌ |
| **v2.0.0** | `release.yml` (v3, 6-job) + `test-wasm-build.yml` (增强) | 全量 | 同上 | ✅ | ctest + smoke + hash matrix | ✅ 同上 + SHA256 审计清单 | ✅ |
| **v2.0.1** | `release.yml` (v3, 修复) | 全量 | 同上 | ✅ | 同上 | ✅ 同上 + 产物命名去冲突 | ✅ |
| **v2.0.2** | `release.yml` (v3, 稳定化) | 全量 | 同上 | ✅ | 同上 | ✅ 同上 + 版本口径统一 | ✅ |

---

## v1.0.0 — 初始发布 (MVP)

**发布日期**: ~2026-02-28  
**定位**: 最小可行产品，证明 SPZ→GLB 无损打包概念  
**Tag**: `v1.0.0`

### 核心能力

| 能力 | 状态 | 说明 |
|------|------|------|
| SPZ 解析 | ✅ | 读取 SPZ v2 格式头部和压缩流 |
| GLB 封装 | ✅ | 使用 KHR_gaussian_splatting_compression_spz_2 扩展 |
| CLI 转换 | ✅ | `spz2glb input.spz output.glb` 基本命令行工具 |
| 三层验证 | ✅ | Layer 1(结构) + Layer 2(无损) + Layer 3(解码一致性) |

### CI/CD 流水线: release.yml v1

**从第一个版本起就具备多平台自动化构建能力**：

```yaml
# 触发条件
on:
  push:
    branches: [main]
    tags: ['v*']        # tag 推送自动触发发布
  workflow_dispatch      # 支持手动触发

# 构建矩阵 (3 平台)
matrix:
  include:
    - os: windows-latest    → spz2glb-windows-x64.exe
    - os: ubuntu-latest     → spz2glb-linux-x64
    - os: macos-latest      → spz2glb-macos-x64

# 构建步骤
steps:
  - actions/checkout@v4     # v4 版本 Actions
  - 平台依赖安装 (zlib/vcpkg)
  - CMake 配置 + 构建
  - fastgltf (warnings suppressed)
  - spz2glb (strict warnings)
  - spz_verify (strict warnings)
  - Artifact 上传
```

**CI 能力总结**:
- ✅ 3 平台自动构建 (Windows x64 / Linux x64 / macOS x64)
- ✅ Tag 推送自动触发
- ✅ Artifact 上传至 GitHub
- ❌ 无测试门禁 (ctest)
- ❌ 无 WASM 构建
- ❌ 无 GitHub Pages 部署
- ❌ 无自动 Release 创建

### 架构决策

1. **压缩流模式**: 将 SPZ 原始字节直接嵌入 GLB bufferView，而非解压重建高斯属性
2. **依赖选择**: 
   - fastgltf（定制版）：高性能 glTF 库
   - simdjson v4.3.1：内置源码
3. **构建系统**: CMake + C++17

### 关键文件结构

```
src/
├── spz_to_glb.cpp        # CLI 主入口
├── spz_verify.cpp        # 验证工具入口
├── spz_verifier.cpp/.h   # 三层验证实现
├── spz2glb_core.cpp/.h   # 核心转换逻辑
└── base64.{h,cpp}        # Base64 编解码
third_party/
└── fastgltf/             # 定制版 glTF 库
.github/workflows/
└── release.yml           # ⬅️ CI/CD: 多平台构建 (v1)
```

---

## v1.0.1 — 验证修复与文档完善

**发布日期**: ~2026-03-05 ~ 2026-03-08  
**Tag**: `v1.0.1`  
**主要变更**: 修复 Layer 1 验证逻辑错误，完善社区基础设施

### 变更清单

| 类型 | 内容 | 动机 |
|------|------|------|
| **Bug Fix** | Layer 1 验证修正为 7/7 checks PASSED | 原 attributes check 逻辑有误 |
| **Docs** | README 添加 Demo section 和验证输出示例 | 提升用户首次使用体验 |
| **Docs** | 添加 Contributor Covenant 行为准则 | 社区规范化 |
| **Chore** | 更新 issue templates (bug_report/feature_request/custom) | 改进问题反馈流程 |
| **Chore** | GitHub Actions 升级到 v5 (Node.js 24 兼容) | 保持 CI 工具链最新 |

### CI/CD: release.yml v1 (延续)

流水线配置无结构性变化，仅升级 Actions 版本：
- `actions/checkout`: v4 → v5
- 其他构建步骤保持不变
- 仍然只有 3 平台 CLI 构建，无测试门禁

### 标准草案关联

此版本的验证修复确保了 SPZ_2 扩展的以下字段被正确校验：
- magic (`0x46546C67`)
- version (`2`)
- extensionsUsed: `KHR_gaussian_splatting`
- extensionsUsed: `KHR_gaussian_splatting_compression_spz_2`
- buffers 配置
- compression stream mode (attributes 为空)

---

## v1.0.2 — 文档国际化

**发布日期**: ~2026-03-08  
**Tag**: `v1.0.2`  
**主要变更**: 完善中文注释和代码文档质量

### 变更清单

| 类型 | 内容 |
|------|------|
| **Docs** | 核心源码 (`src/*.cpp`) 添加详细中文注释 |
| **Docs** | 三层验证工具 (`spz_verifier`) 添加中文维护注释 |
| **License** | 版权所有者更新为 Pu Junhan (2026) |
| **License** | 所有源文件添加 SPDX MIT 标识 |
| **Wiki** | 批量创建 GitHub Wiki 页面 (后迁移至 GitHub Wiki 并从仓库移除) |

### CI/CD: release.yml v1 (无变化)

此版本为纯文档迭代，CI 流水线未修改。

---

## v1.1.0 — WASM 支持里程碑 ⭐

**发布日期**: ~2026-03-19 ~ 2026-03-24  
**Tag**: `v1.1.0`  
**定位**: 从纯 CLI 工具升级为双端（CLI + Web）工具  
**重要程度**: ⭐⭐⭐ 项目架构的重大转折点

### Phase S1-S4: WASM 基础建设

#### S1: 构建系统重构 (~40+ commits)
- **问题**: 原有 CMake 不区分桌面/WASM 目标
- **解决**: 引入 `SPZ2GLB_BUILD_WASM` 开关，条件编译桌面 vs WASM 目标
- **关键修复**:
  - `CMAKE_SOURCE_DIR` 路径修正 (从 tools/spz_to_glb 迁移至根目录构建)
  - ZLIB 跨平台支持 (`find_package` + emscripten port)
  - MSVC 编译选项修复
  - 条件编译包装 (`if(NOT SPZ2GLB_BUILD_WASM)`)

#### S2: Core Extraction
- **决策**: 将 `spz2glb_core` 提取为独立静态库
- **目的**: CLI 和 WASM 共享同一套转换逻辑，消除双端分叉风险
- **关键文件**: `src/spz2glb_core.cpp/.h`, `src/spz2glb_wasm_c_api.cpp/.h`

#### S3: WASM C API 设计
```cpp
struct Spz2GlbResult {
    uint8_t* bytes;
    size_t size;
};
Spz2GlbApi* loadSpz2Glb(const char* wasm_path);
Spz2GlbResult convert(Spz2GlbApi* api, const uint8_t* spz_data, size_t spz_size);
void release(Spz2GlbResult* result);
MemoryStats getMemoryStats(Spz2GlbApi* api);
```

#### S4: 内存管理策略
- 预分配模式: `reserve_input()` → 分块写入 → `convert_reserved_input()` → `release_output()`
- 双档配置: compat (64MB) / perf-lite (128MB)
- 设备分档: 基于 `navigator.deviceMemory` / `hardwareConcurrency` / UA

### Phase S5-S7: 浏览器集成与 CI 扩展

#### S5: 浏览器生命周期
- WASM 加载方式多次迭代（Emscripten 胶水 → 原生 WebAssembly.instantiate）
- Embind 版本废弃，保留纯 C API WASM
- import name minification 处理 (`--minify=0`, `-sLEGALIZE_JS_FFI=0`)
- smart_memory.js 内存管理模块

#### S6: 性能优化
- `-O3` + warning-clean 门禁
- `-fno-exceptions` 移除异常开销
- Bump allocator + 热点对象池

#### S7: CI/CD 大幅扩展 ⬅️ **关键变化**

**release.yml v2 升级**:

```yaml
# 新增能力
- actions/checkout@v5 with submodules: recursive  # 子模块递归检出
- Artifact 重命名 + 上传 (Windows/Linux/Mac 各平台)
- 新增 wasm-build job: Emscripten 构建 (compat + perf-lite 双 profile)
- 新增 deploy-pages job: GitHub Pages 自动部署
  ├── 从 docs/examples/ 复制 HTML/JS 入口
  ├── 下载 WASM 产物 (compat profile)
  ├── 生成 pages-artifact-sha256.txt 审计清单
  ├── actions/upload-pages-artifact@v3
  └── actions/deploy-pages@v4
- 新增 release job: tag 推送时自动创建 GitHub Release
  ├── 下载 CLI artifacts (spz2glb-* + *-verify)
  ├── 下载 WASM artifacts (wasm-modules-*)
  ├── Normalize WASM artifact names (按 profile 重命名)
  ├── Generate release-sha256.txt
  └── softprops/action-gh-release@v1 (generate_release_notes)
```

**新增 test-wasm-build.yml** (独立的 WASM 测试流水线):

```yaml
name: Test WASM Build
on:
  workflow_dispatch:            # 手动触发 (含 private samples 开关)
  push:                         # main 分支推送
    branches: [main]
    paths:                      # 仅在以下文件变更时触发
      - 'CMakeLists.txt'
      - 'src/**'
      - '.github/workflows/test-wasm-build.yml'

jobs:
  test-wasm-build:              # WASM 构建详细日志 + 分析
    matrix: wasm_profile: [compat, perf-lite]
    steps:
      - Emscripten setup
      - wabt tools install (wasm-objdump 分析)
      - Build spz2glb WASM (verbose logging)
      - Locate WASM artifacts (智能搜索)
      - Analyze WASM imports/exports/memory sections
      - Validate WASM hash matrix (4 fixed samples via Playwright)
      - Test browser glue loading (smoke test)
      - Upload build logs
```

**CI 能力总结 (v1.1.0)**:
- ✅ 3 平台 CLI 自动构建
- ✅ WASM 双 profile 构建 (compat/perf-lite)
- ✅ GitHub Pages 自动部署
- ✅ 自动 GitHub Release (tag 触发)
- ✅ Playwright browser smoke test
- ✅ WASM hash matrix 校验 (4 固定样本)
- ✅ WASM 二进制分析 (imports/exports/memory)
- ✅ Private fixed-samples validation (self-hosted runner)
- ❌ 无 ctest 门禁
- ❌ 无 verify-native gate
- ❌ 无回滚机制

### 技术债务记录

| 债务项 | 产生原因 | 后续处理 |
|--------|----------|----------|
| WASM 加载方式多次反复 | Emscripten 版本兼容性问题 | v2.0 统一为原生 API |
| 条件编译散落各处 | 渐进式添加 WASM 支持 | v2.0 通过 core extraction 解决 |
| CI 中部分路径硬编码 | dist 目录位置不确定 | v2.0 统一产物路径规范 |

---

## v2.0.0 — 大规模重构 (Major Refactor) 🏗️

**发布日期**: 2026-03-30  
**Tag**: `v2.0.0` (含 rc1, docs-sync 前置 tag)  
**定位**: 生产级质量，统一架构，面向标准草案落地

### 重构目标

1. **统一核心链路**: CLI 和 WASM 共享 `spz2glb_core`，消除双端行为差异
2. **三层验证闭环**: 结构验证 / 无损验证 / 解码一致性验证全部通过自动化
3. **职责边界固定**: 明确 spz2glb 只做两件事——SPZ→GLB 打包 + GLB 交付
4. **WASM 发布级治理**: 内存预算、显式释放、统计可观测性

### 核心变更

#### 1. 架构统一 (S2 完成)
```
Before (v1.x):
  CLI: spz_to_glb.cpp → 直接调用转换逻辑
  WASM: wasm_glue.cpp → 独立实现转换逻辑
  
After (v2.0):
  CLI: spz_to_glb.cpp → spz2glb_core (静态库)
  WASM: spz2glb_wasm_c_api.cpp → spz2glb_core (同一静态库)
```

#### 2. 验证器重构
- 统一 3 层检查入口
- 添加中文维护注释
- CI 集成 verify-native gate

#### 3. 工程债收口 (S3.5)
- 统一前置流程（参数解析 → 输入校验 → 转换 → 输出）
- 错误传播标准化
- CLI 入口统一

#### 4. 浏览器端完善
- embind registration mismatch 修复
- Pages assets 同步
- cache-busting 策略（后因稳定性回退）

### CI/CD: release.yml v3 — 完整的 6-Job 流水线体系 ⬅️ **重大升级**

```yaml
permissions:
  actions: read
  contents: write
  pages: write          # ⬅️ 新增 Pages 权限
  id-token: write       # ⬅️ 新增 OIDC 权限

jobs:
  
  # ═══════════════════════════════════════
  # Job 1: build — 多平台 CLI 构建 (继承自 v1, 增强)
  # ═══════════════════════════════════════
  build:
    strategy:
      fail-fast: false
      matrix:
        include:
          - windows-latest → spz2glb-windows-x64.exe
          - ubuntu-latest  → spz2glb-linux-x64
          - macos-latest   → spz2glb-macos-x64
    outputs:
      - spz2glb-{platform} (Artifact)
      - {platform}-verify (spz_verify Artifact)

  # ═══════════════════════════════════════
  # Job 2: wasm-build — 双 Profile WASM 构建
  # ═══════════════════════════════════════
  wasm-build:
    matrix: wasm_profile: [compat, perf-lite]  # ⬅️ 双 profile
    steps:
      - Setup Emscripten (mymindstorm/setup-emsdk@v14)
      - emcmake cmake (SPZ2GLB_BUILD_WASM=ON)
      - Build spz2glb-wasm + spz_verify-wasm
      - Validate WASM outputs (.wasm + .js × 2 targets)
      - Upload: wasm-modules-{profile}

  # ═══════════════════════════════════════
  # Job 3: verify-native ⬅️ 【新增】原生测试门禁
  # ═══════════════════════════════════════
  verify-native:
    needs: [build]                              # 依赖 build 完成
    steps:
      - CMake 构建 (Release)
      - Build spz2glb + spz_verify
      - Run ctest                               # ⬅️ 单元测试门禁
      - Run tests/quick_test.sh                 # ⬅️ 集成测试门禁

  # ═══════════════════════════════════════
  # Job 4: deploy-pages — GitHub Pages 部署
  # ═══════════════════════════════════════
  deploy-pages:
    needs: [wasm-build, verify-native]          # ⬅️ 依赖两个前置 job
    if: (push to main || manual) && !rollback
    environment:
      name: github-pages                        # ⬅️ 正式环境保护
    steps:
      - Prepare site root (HTML + JS bindings)
      - Download WASM (compat profile)
      - Place & validate WASM modules
      - Generate pages-artifact-sha256.txt      # ⬅️ Pages 审计清单
      - Upload Pages audit artifact
      - actions/upload-pages-artifact@v3
      - actions/deploy-pages@v4

  # ═══════════════════════════════════════
  # Job 5: release — 自动 Release 创建
  # ═══════════════════════════════════════
  release:
    needs: [build, wasm-build, verify-native]   # ⬅️ 依赖全部构建+测试
    if: startsWith(github.ref, 'refs/tags/')   # 仅 tag 触发
    steps:
      - Download CLI artifacts
      - Download Verify artifacts
      - Download WASM artifacts (双 profile)
      - Normalize WASM names ({base}-{profile}.{ext})
      - Generate release-sha256.txt             # ⬅️ Release 审计清单
      - Create Release (softprops/action-gh-release@v1, auto-generate notes)

  # ═══════════════════════════════════════
  # Job 6: rollback-manifest ⬅️ 【新增】失败回滚记录
  # ═══════════════════════════════════════
  rollback-manifest:
    if: always() && any failure in upstream
    needs: [build, wasm-build, verify-native, deploy-pages, release]
    steps:
      - Find previous stable run (via GitHub API)
      - Generate rollback-manifest.txt
        ├── current_run_id
        ├── current_sha
        ├── previous_stable_run_id
        ├── previous_stable_commit
        └── failed_jobs
      - Upload rollback manifest

  # ═══════════════════════════════════════
  # Bonus: rollback-artifact-entry ⬅️ 【新增】手动回滚入口
  # ═══════════════════════════════════════
  rollback-artifact-entry:
    if: workflow_dispatch && inputs.rollback_run_id != ''
    steps:
      - Download artifacts from specified stable run-id
      - Generate rollback execution manifest
      - Upload restored artifacts
```

### test-wasm-build.yml 增强 (v2.0)

```yaml
# 新增/增强内容
on:
  push:
    paths:
      - 'docs/examples/**'   # ⬅️ 新增：前端文件变更也触发
      - 'tests/**'           # ⬅️ 新增：测试文件变更也触发
    branches:
      - phase5-browser-lifecycle-20260325  # ⬅️ 新增：功能分支触发

jobs:
  # 新增 private-fixed-samples-validation job
  private-fixed-samples-validation:
    if: workflow_dispatch && inputs.run_private_fixed_samples == true
    runs-on: [self-hosted, linux, x64]     # ⬅️ 自托管 runner
    steps:
      - Verify 4 fixed samples mounted (triangle/cube/near_limit/v4_ext)
      - Build native CLI tools
      - Validate Layer1/2/3 on fixed samples (CLI path)
      - Build WASM
      - Validate WASM hash matrix (fixed samples, browser runtime)
      - Upload verification logs
```

**CI 能力总结 (v2.0.0)**:
- ✅ 3 平台 CLI 自动构建 (Win/Linux/Mac)
- ✅ WASM 双 profile 构建 (compat/perf-lite)
- ✅ **verify-native gate** (ctest + quick_test.sh) ← 新增
- ✅ **GitHub Pages 自动部署** (sha256 审计) ← 增强
- ✅ **自动 GitHub Release** (SHA256 manifest + auto-notes) ← 增强
- ✅ **Playwright browser smoke test** (版本检测)
- ✅ **WASM hash matrix** (4 样本确定性校验)
- ✅ **Rollback manifest** (失败自动记录) ← 新增
- ✅ **Manual rollback entrypoint** (指定 run-id 恢复) ← 新增
- ✅ **Private fixed-samples validation** (self-hosted) ← 新增

### 性能基准

| 测试用例 | 文件大小 | 转换耗时 | 峰值内存 |
|----------|----------|----------|----------|
| dunhuang_000000.spz | 24.78 MB | ~506 ms (WASM) | ~49.56 MB |
| hornedlizard.spz | ~18 MB | <400 ms (WASM) | ~40 MB |

### 标准草案关联

| 规范要求 | spz2glb 实现 | 状态 |
|----------|-------------|------|
| SPZ compressed stream in bufferView | ✅ 直接存储原始 SPZ 字节 | 符合 |
| KHR_gaussian_splatting_compression_spz_2 | ✅ 正确声明扩展名 | 符合 |
| No accessors/attributes in stream mode | ✅ attributes 为空 | 符合 |
| Lossless guarantee | ✅ MD5 byte-level verification | 超出规范 |
| Browser-side processing | ✅ WASM + memory governance | 超出规范 |

---

## v2.0.2 — 版本收口与文档降噪

**发布日期**: 2026-04-20  
**Tag**: `v2.0.2`  
**定位**: 版本口径统一、文档修正、版权头更新

### 变更清单

| 类型 | 内容 | 说明 |
|------|------|------|
| **Fix** | 统一 CMakeLists.txt 工程版本 | 从 1.0.0 更新为 2.0.2 |
| **Fix** | 统一 spz2glb_get_version() 返回值 | 从 1.0.0 更新为 2.0.2 |
| **Docs** | 修正 README.md 中 Layer 2 描述 | 从 "100% MD5 match" 改为 "byte-identical" |
| **Docs** | 修正 README-zh.md 中 Layer 2 描述 | 从 "100% MD5 match" 改为 "byte-identical" |
| **Docs** | 更新 Layer 2 验证说明 | 明确为字节级比较而非 MD5 验证 |
| **Docs** | 更新源码版权头 | 添加黄帝纪年 4723 年丙午年文化注记 |
| **Docs** | 更新 CHANGELOG 版本范围 | 扩展到 v2.0.2 |

### 已知限制

- **三层验证路径约束**: L2/L3 层目前未强约束 `primitive.extensions.KHR_gaussian_splatting.extensions.KHR_gaussian_splatting_compression_spz_2` 的完整嵌套路径
- **Wiki 文档过时**: 多个 wiki 文件存在描述不一致，后续版本将重写

### 后续计划

- **v2.0.3**: 三层验证补强版，专门修复 L2/L3 路径约束
- **Wiki 重写**: 使用 graphify 工具重写 wiki 文档

---

## v2.0.1 — 稳定化与文档同步

**发布日期**: 2026-03-30 ~ 2026-04-02  
**Tag**: `v2.0.1`  
**定位**: 当前最新稳定版，文档对齐，演示站点就绪

### 变更清单

| 类型 | 内容 | 说明 |
|------|------|------|
| **Docs** | README 添加"职责边界"固定声明 | 明确不做压缩算法研发/渲染引擎扩展 |
| **Docs** | README 添加生态定位说明 | 与 spz_gatekeeper 上下游关系 |
| **Docs** | 与 splat-transform 对比表 | 定位差异清晰化 |
| **Docs** | README 刷新为 v2 current API and runtime guidance | 移除过时的 v2.0.0 固定表述 |
| **Fix** | WASM init cache-busting 回退 | 确保浏览器加载稳定性 (revert commit) |
| **Fix** | WASM embind registration mismatch 修复 + Pages assets 同步 | 解决浏览器端加载不一致问题 |
| **CI** | release workflow 产物命名去冲突 | 避免 tag release 时 wasm-modules asset 覆盖 |
| **CI** | unblock wasm verify build + rollback manifest lookup | 修复 verify-wasm 构建阻塞 |
| **Docs** | 记录成功 S4 verification run ID | 可复现性证据 |

### CI/CD: release.yml v3 (修复迭代)

此版本主要是 CI bug fix 和稳定化：
- 产物命名规范化: `{base}-{profile}.{ext}` 避免 collisions
- verify-native gate 修复: 解锁 wasm verify 构建
- rollback-manifest lookup 修复: 确保失败时可正确查找历史 stable run
- deploy-pages 条件收紧: 排除 rollback 场景

### 当前状态总结

```
✅ 核心功能: SPZ→GLB 无损打包 (100% byte-level fidelity)
✅ 双端协同: CLI (重任务) + WASM (轻量网页)
✅ 三层验证: 结构 / 无损 / 解码一致性 全部自动化
✅ CI/CD: 6-Job 流水线 (build/wasm-build/verify-native/deploy-pages/release/rollback)
✅ 文档: 中英双语 README + Wiki + CHANGELOG + 演示截图
✅ 演示: GitHub Pages 在线转换 (https://spz-ecosystem.github.io/spz2glb/)
✅ 标准: KHR_gaussian_splatting_compression_spz_2 兼容
✅ 安全: Release SHA256 manifest + Pages sha256 audit + Rollback mechanism
```

---

## 重大架构决策记录

### 决策 1: 压缩流模式 vs 解压重建

**时间**: v1.0.0 设计阶段  
**选项**:
- A: 解压 SPZ → 重建高斯属性 → 写入标准 glTF（如 splat-transform）
- B: 保持 SPZ 原始字节 → 作为二进制流嵌入 GLB（本项目选择）

**理由**:
- 100% 数据保真，无编解码损失
- 保留未来解码器优化的可能性
- 文件体积最小（SPZ 本身已压缩约 10x）

**代价**:
- 需要 SPZ 解码器的渲染器才能使用
- 无法在通用 glTF 查看器中查看内容

### 决策 2: WASM 原生 API vs Emscripten 胶水

**时间**: v1.1.0 S4 阶段  
**最终选择**: 原生 WebAssembly API（`WebAssembly.instantiate`）+ 手写 JS bindings  
**理由**:
- 更小的产物体积
- 更好的内存控制
- 避免 Emscripten 运行时依赖

### 决策 3: Core Extraction 统一双端

**时间**: v2.0 S2 阶段  
**影响**: 消除了 v1.x 中 CLI/WASM 两套独立实现的维护负担  
**收益**: 
- 单一真相来源 (SSOT)
- Bug fix 只需改一处
- 未来新增功能自动覆盖两端

### 决策 4: CI/CD 渐进式建设策略

**时间**: 贯穿 v1.0.0 → v2.0.1  
**策略**: 从 Day 1 就建立 CI，随版本逐步扩展而非事后补救

| 阶段 | 策略 | 结果 |
|------|------|------|
| v1.0.0 | 先建立多平台构建基线 | 每个 version 都有可分发的 binary |
| v1.1.0 | 扩展 WASM + Pages + Release | 双端覆盖 + 自动发布 |
| v2.0.0 | 补齐测试门禁 + 回滚 + 审计 | 生产级质量保障 |

---

## 技术债务追踪

| ID | 债务描述 | 引入版本 | 计划修复 | 状态 |
|----|----------|----------|----------|------|
| TD-01 | third_party/fastgltf 为定制版，无法上游更新 | v1.0.0 | v2.5 或按需 | Open |
| TD-02 | simdjson 内嵌源码 (v4.3.1) | v1.0.0 | 维持现状 | Open |
| TD-03 | Wiki 文档曾内嵌仓库后迁移至 GitHub Wiki | v1.0.x | 已完成 | Closed |
| TD-04 | WASM 加载方式历史遗留多种方案 | v1.1.0 | v2.0 统一为原生 API | Closed |
| TD-05 | CI 中部分路径硬编码 | v1.1.0 | v2.0 统一产物路径规范 | Improved |
| TD-06 | 测试 fixture 未提交（仅本地/private runner） | v1.0.0 | 持续 (安全 Policy) | Policy |
| TD-07 | 无 benchmark 自动化回归 | v1.0.0 | v3.0 | Planned |

---

## 论文写作参考索引

### 可引用的工程贡献

1. **无损打包范式**: 证明 SPZ→GLB 可以在不解压的情况下保持 100% 保真
2. **双端统一架构**: Core Extraction 模式可作为 WASM/CLI 共享库的设计模式
3. **智能内存分配**: 三层设备分档 + WASM 预分配 + 显式释放的完整方案
4. **三层验证体系**: 结构/无损/解码一致性的分层验证方法论
5. **SPZ_2 扩展实践**: 首个实现 Khronos SPZ_2 压缩扩展的开源工具
6. **渐进式 CI/CD**: 从 MVP 到生产级的 CI 演进案例 (632 commits, 7 releases)

### 性能数据摘要

| 指标 | 数值 | 测试环境 |
|------|------|----------|
| 转换吞吐 (WASM) | ~49 MB/s | Chrome, dunhuang_000000.spz |
| 内存效率 | 2x 输入文件大小 | perf-lite profile |
| 验证速度 L1 | <10ms | 7 项结构检查 |
| 验证速度 L2 | 取决于文件大小 | MD5 全量计算 |
| 产物体积增量 | ~12 bytes (GLB header) | 相对于原始 SPZ |

### CI/CD 作为工程贡献的亮点

- **632+ commits** 映射到 **7 个版本标签**，每个版本都有对应的 CI 能力增量
- **从 v1.0.0 起**就有 3 平台自动化构建，不是事后补充
- **v1.1.0** 一次性引入 WASM 全栈 CI（构建/测试/部署/发布），体现架构前瞻性
- **v2.0.0** 的 6-Job 流水线包含 **verify-native gate** + **rollback mechanism** + **SHA256审计**，达到生产级标准

---

## 附录

### A. Git Tags 完整列表

```
v1.0.0              # 初始 MVP (含基础 CI)
v1.0.1              # 验证修复 + 社区设施
v1.0.2              # 文档国际化
v1.1.0              # WASM 支持 (CI 大幅扩展)
v2.0.0              # 大规模重构 (完整 6-Job CI)
v2.0.0-rc1          # v2.0 候选版
v2.0.0-docs-sync    # 文档同步预发布
v2.0.1              # 当前稳定版 (CI 稳定化)
```

### B. 仓库统计 (截至 v2.0.1)

- 总提交数: 632+
- 贡献者: 1 (Pu Junhan)
- 代码行数: ~5000+ (C++) + ~2000 (JS/CMake/Docs)
- 支持平台: Windows / Linux / macOS (x64) + Browser (WASM)
- CI Jobs: 6 (build, wasm-build, verify-native, deploy-pages, release, rollback)
- CI Targets: 3 platform CLI + 2 WASM profile + browser smoke + hash matrix

### C. 外部依赖版本锁定

| 依赖 | 版本 | 许可证 |
|------|------|--------|
| fastgltf (定制版) | 基于 Sean Apeler 版本 | MIT |
| simdjson | v4.3.1 (内置) | MIT |
| ZLIB | 系统/emsdk 提供 | zlib License |
| Emscripten | latest (CI) | MIT/Apache-2.0 |
| Playwright | 1.53.0 (CI) | Apache-2.0 |

### D. CI/CD 流水线文件清单

| 文件 | 引入版本 | 用途 |
|------|----------|------|
| `.github/workflows/release.yml` | v1.0.0 | 主构建+发布流水线 (历经 v1→v2→v3 三次大改) |
| `.github/workflows/test-wasm-build.yml` | v1.1.0 | WASM 构建+测试+验证流水线 |
| `.github/ISSUE_TEMPLATE/bug_report.md` | v1.0.1 | Bug 反馈模板 |
| `.github/ISSUE_TEMPLATE/feature_request.md` | v1.0.1 | 功能请求模板 |
| `.github/ISSUE_TEMPLATE/custom.md` | v1.0.1 | 自定义 issue 模板 |

---

*本文档随项目版本持续更新。每次发版时请同步更新相关章节。*

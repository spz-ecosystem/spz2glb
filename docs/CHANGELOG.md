# spz2glb 项目演进日志 (Project Evolution Log)

> **维护者**: Pu Junhan  
> **起始日期**: 2026-02-xx  
> **当前稳定版**: v2.0.1  
> **许可证**: MIT  

---

## 概述

本文档记录 `spz2glb` 从 v1.0.0 到 v2.0.1 的完整版本演进历程，包括：
- 每个版本的核心变更
- 重构决策与动机
- 技术债务处理记录
- 与 Khronos SPZ_2 标准草案的对应关系

**用途**：
- 推动标准草案落地的重要参考材料
- 顶会论文写作的项目背景依据
- 新贡献者的快速上手指南

---

## 版本时间线

```
v1.0.0 ──→ v1.0.1 ──→ v1.0.2 ──→ v1.1.0 ──→ v2.0.0 ──→ v2.0.1
 │         │          │          │          │          │
 ├ MVP     ├ 验证修复   ├ 文档      ├ WASM      ├ 大重构    ├ 收口
 │ 基础功能  ├ Layer1   ├ 注释      ├ 内存管理   ├ 统一核心  ├ 稳定化
 │          ├ 文档      │          ├ CI/CD     ├ 三层验证  ├ 浏览器生命周期
```

---

## v1.0.0 — 初始发布 (MVP)

**发布日期**: ~2026-02  
**定位**: 最小可行产品，证明 SPZ→GLB 无损打包概念

### 核心能力

| 能力 | 状态 | 说明 |
|------|------|------|
| SPZ 解析 | ✅ | 读取 SPZ v2 格式头部和压缩流 |
| GLB 封装 | ✅ | 使用 KHR_gaussian_splatting_compression_spz_2 扩展 |
| CLI 转换 | ✅ | `spz2glb input.spz output.glb` 基本命令行工具 |
| 三层验证 | ✅ | Layer 1(结构) + Layer 2(无损) + Layer 3(解码一致性) |

### 架构决策

1. **压缩流模式**: 选择将 SPZ 原始字节直接嵌入 GLB bufferView，而非解压重建高斯属性。这是与 `splat-transform` 的根本差异。
2. **依赖选择**: 
   - fastgltf（定制版）：高性能 glTF 库
   - simdjson v4.3.1：内置源码，不依赖系统库
3. **构建系统**: CMake + C++17，一键编译

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
```

### 已知限制

- 仅支持桌面端 CLI
- 无 WASM/Web 支持
- 无内存预算控制
- 无 CI/CD 流水线

---

## v1.0.1 — 验证修复与文档完善

**发布日期**: ~2026-02  
**主要变更**: 修复 Layer 1 验证逻辑错误，完善文档

### 变更清单

| 类型 | 内容 | 动机 |
|------|------|------|
| **Bug Fix** | Layer 1 验证修正为 7/7 checks PASSED | 原 attributes check 逻辑有误 |
| **Docs** | README 添加 Demo section 和验证输出示例 | 提升用户首次使用体验 |
| **Docs** | 添加 Contributor Covenant 行为准则 | 社区规范化 |
| **Chore** | 更新 issue templates | 改进问题反馈流程 |

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

**发布日期**: ~2026-03  
**主要变更**: 完善中文注释和文档

### 变更清单

| 类型 | 内容 |
|------|------|
| **Docs** | 核心源码添加详细中文注释 |
| **Docs** | 三层验证工具添加中文维护注释 |
| **License** | 版权所有者更新为 Pu Junhan (2026) |
| **License** | 所有源文件添加 SPDX MIT 标识 |

---

## v1.1.0 — WASM 支持里程碑

**发布日期**: ~2026-03  
**定位**: 从纯 CLI 工具升级为双端（CLI + Web）工具  
**重要程度**: ⭐⭐⭐ 这是项目架构的重大转折点

### Phase S1-S4: WASM 基础建设

#### S1: 构建系统重构
- **问题**: 原有 CMake 不区分桌面/WASM 目标，WASM 编译会尝试链接桌面 ZLIB
- **解决**: 引入 `SPZ2GLB_BUILD_WASM` 开关，条件编译桌面 vs WASM 目标
- **提交量**: ~40+ commits（大量路径修复、条件判断调整）

#### S2: 核心代码提取
- **决策**: 将 `spz2glb_core` 提取为独立静态库
- **目的**: CLI 和 WASM 共享同一套转换逻辑，消除双端分叉风险
- **关键文件**: `src/spz2glb_core.cpp/.h`, `src/spz2glb_wasm_c_api.cpp/.h`

#### S3: WASM C API 设计
```cpp
// 核心 API 接口设计
struct Spz2GlbResult {
    uint8_t* bytes;       // 指向 WASM 内存的输出
    size_t size;           // 输出大小
};

// 关键函数
Spz2GlbApi* loadSpz2Glb(const char* wasm_path);
Spz2GlbResult convert(Spz2GlbApi* api, const uint8_t* spz_data, size_t spz_size);
void release(Spz2GlbResult* result);  // 必须调用释放
MemoryStats getMemoryStats(Spz2GlbApi* api);
```

#### S4: 内存管理策略
- **预分配模式**: `reserve_input()` → 分块写入 → `convert_reserved_input()` → `release_output()`
- **双档配置**: compat (64MB) / perf-lite (128MB)
- **设备分档**: 基于 `navigator.deviceMemory` / `hardwareConcurrency` / UA

### Phase S5-S7: 浏览器集成与 CI

#### S5: 浏览器生命周期
- WASM 加载方式迭代（从 Emscripten 胶水 → 原生 WebAssembly.instantiate）
- embind 导出保护
- 缓存失效重试机制（后因稳定性回退）

#### S6: 性能优化
- `-O3` + warning-clean 门禁
- `-fno-exceptions` 移除异常开销
- Bump allocator + 热点对象池
- import name minification 处理

#### S7: CI/CD 建设
- GitHub Actions 多平台构建
- Playwright browser smoke test
- WASM hash matrix 自动化校验
- Release workflow with artifact upload

### 技术债务记录

| 债务项 | 产生原因 | 后续处理 |
|--------|----------|----------|
| WASM 加载方式多次反复 | Emscripten 版本兼容性问题 | v2.0 统一为原生 API |
| 条件编译散落各处 | 渐进式添加 WASM 支持 | v2.0 通过 core extraction 解决 |
| CI 路径硬编码 | dist 目录位置不确定 | v2.0 统一产物路径规范 |

---

## v2.0.0 — 大规模重构 (Major Refactor)

**发布日期**: 2026-03-30  
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
- rollback entrypoint（发布失败自动回滚）

#### 3. 工程债收口 (S3.5)
- 统一前置流程（参数解析 → 输入校验 → 转换 → 输出）
- 错误传播标准化
- CLI 入口统一

#### 4. 浏览器端完善
- embind registration mismatch 修复
- Pages assets 同步
- cache-busting 策略（后回退）

### 性能基准

| 测试用例 | 文件大小 | 转换耗时 | 峰值内存 |
|----------|----------|----------|----------|
| dunhuang_000000.spz | 24.78 MB | ~506 ms (WASM) | ~49.56 MB |
| hornedlizard.spz | ~18 MB | <400 ms (WASM) | ~40 MB |

### 标准草案关联

v2.0 的架构决策直接影响 Khronos SPZ_2 扩展提案：

| 规范要求 | spz2glb 实现 | 状态 |
|----------|-------------|------|
| SPZ compressed stream in bufferView | ✅ 直接存储原始 SPZ 字节 | 符合 |
| KHR_gaussian_splatting_compression_spz_2 | ✅ 正确声明扩展名 | 符合 |
| No accessors/attributes in stream mode | ✅ attributes 为空 | 符合 |
| Lossless guarantee | ✅ MD5 byte-level verification | 超出规范 |
| Browser-side processing | ✅ WASM + memory governance | 超出规范 |

---

## v2.0.1 — 稳定化与文档同步

**发布日期**: 2026-03-30 ~ 2026-04-02  
**定位**: 当前最新稳定版，文档对齐，演示站点就绪

### 变更清单

| 类型 | 内容 | 说明 |
|------|------|------|
| **Docs** | README 添加"职责边界"固定声明 | 明确不做压缩算法研发/渲染引擎扩展 |
| **Docs** | README 添加生态定位说明 | 与 spz_gatekeeper 上下游关系 |
| **Docs** | 与 splat-transform 对比表 | 定位差异清晰化 |
| **Docs** | GitHub Pages 演示链接修正 | https://spz-ecosystem.github.io/spz2glb/ |
| **Fix** | WASM init cache-busting 回退 | 确保浏览器加载稳定性 |
| **CI** | release workflow 产物命名去冲突 | 避免 tag release 时 asset 覆盖 |

### 当前状态总结

```
✅ 核心功能: SPZ→GLB 无损打包 (100% byte-level fidelity)
✅ 双端协同: CLI (重任务) + WASM (轻量网页)
✅ 三层验证: 结构 / 无损 / 解码一致性 全部自动化
✅ CI/CD: GitHub Actions 多平台 + Playwright smoke
✅ 文档: 中英双语 README + Wiki + CHANGELOG
✅ 演示: GitHub Pages 在线转换
✅ 标准: KHR_gaussian_splatting_compression_spz_2 兼容
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

---

## 技术债务追踪

| ID | 债务描述 | 引入版本 | 计划修复 | 状态 |
|----|----------|----------|----------|------|
| TD-01 | third_party/fastgltf 为定制版，无法上游更新 | v1.0.0 | v2.5 或按需 | Open |
| TD-02 | simdjson 内嵌而非 submodule | v1.0.0 | v2.5 | Open |
| TD-03 | Wiki 文档曾内嵌仓库后迁移至 GitHub Wiki | v1.0.x | 已完成 | Closed |
| TD-04 | WASM 加载方式历史遗留多种方案 | v1.1.0 | v2.0 统一 | Closed |
| TD-05 | CI 中部分路径硬编码 | v1.1.0 | v2.0 改善 | Improved |
| TD-06 | 测试 fixture 未提交（仅本地） | v1.0.0 | 持续 | Policy |
| TD-07 | 无 benchmark 自动化回归 | v1.0.0 | v3.0 | Planned |

---

## 论文写作参考索引

### 可引用的工程贡献

1. **无损打包范式**: 证明 SPZ→GLB 可以在不解压的情况下保持 100% 保真
2. **双端统一架构**: Core Extraction 模式可作为 WASM/CLI 共享库的设计模式
3. **智能内存分配**: 三层设备分档 + WASM 预分配 + 显式释放的完整方案
4. **三层验证体系**: 结构/无损/解码一致性的分层验证方法论
5. **SPZ_2 扩展实践**: 首个实现 Khronos SPZ_2 压缩扩展的开源工具

### 性能数据摘要

| 指标 | 数值 | 测试环境 |
|------|------|----------|
| 转换吞吐 (WASM) | ~49 MB/s | Chrome, dunhuang_000000.spz |
| 内存效率 | 2x 输入文件大小 | perf-lite profile |
| 验证速度 L1 | <10ms | 7 项结构检查 |
| 验证速度 L2 | 取决于文件大小 | MD5 全量计算 |
| 产物体积增量 | ~12 bytes (GLB header) | 相对于原始 SPZ |

---

## 附录

### A. Git Tags 完整列表

```
v1.0.0          # 初始 MVP
v1.0.1          # 验证修复
v1.0.2          # 文档国际化
v1.1.0          # WASM 支持
v2.0.0          # 大规模重构
v2.0.0-rc1      # v2.0 候选版
v2.0.0-docs-sync # 文档同步
v2.0.1          # 当前稳定版
```

### B. 仓库统计 (截至 v2.0.1)

- 总提交数: 631+
- 贡献者: 1 (Pu Junhan)
- 代码行数: ~5000+ (C++) + ~2000 (JS/CMake/Docs)
- 支持平台: Windows / Linux / macOS (x64 + ARM) + Browser (WASM)

### C. 外部依赖版本锁定

| 依赖 | 版本 | 许可证 |
|------|------|--------|
| fastgltf (定制版) | 基于 Sean Apeler 版本 | MIT |
| simdjson | v4.3.1 (内置) | MIT |
| ZLIB | 系统/emsdk 提供 | zlib License |
| Emscripten | latest (CI) | MIT/Apache-2.0 |

---

*本文档随项目版本持续更新。每次发版时请同步更新相关章节。*

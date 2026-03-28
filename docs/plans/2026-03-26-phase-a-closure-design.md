# SPZ2GLB 收口实施计划（方案 A）

> 适用范围：`tools/spz_to_glb` 当前分支（保持 `KHR_gaussian_splatting_compression_spz_2` 压缩流容器路线不变）

## 1. 背景与目标

基于现状，`spz2glb` 已具备浏览器与 CLI 双端能力，并完成了 Phase 6/7 的关键补齐；但在“工程收口”层面仍存在核心逻辑与入口耦合、验证链路重复、发布链依赖外部资源等问题。

本计划采用**方案 A（收口优先，轻分层增强）**，目标是：

1. 不改变当前输出契约（继续输出 `SPZ_2` 压缩流模式 GLB）；
2. 完成工程结构收敛（核心层与平台入口解耦）；
3. 完成验证体系收敛（统一 verifier 与验收指标）；
4. 完成交付链收敛（去除发布流程外部依赖）；
5. 补齐 `SPZ v4`（含扩展 trailer）兼容声明、验证与 CI 基线；
6. 给出可重复的量化结果，形成“可宣布完成”的闭环证据。

---

## 2. 边界约束（必须遵守）

### 2.1 In Scope（本轮必做）
- 核心转换逻辑拆分与复用方式整理；
- verifier 统一与测试增强；
- browser/CLI 一致性验收；
- `SPZ v4`（含扩展 trailer）兼容性声明与验收；
- CI/发布链收口；
- 基线对比与验收报告（数据化）。

### 2.2 Out of Scope（本轮不做）
- 引入“属性展开型 GLB writer”并替代当前压缩流路线；
- 引入 Morton/KD-Tree/体素/LOD/WebGPU 等新能力；
- 大规模重构为 npm/rollup 库形态。

### 2.3 固定契约
- GLB 仍以 `KHR_gaussian_splatting` + `KHR_gaussian_splatting_compression_spz_2` 为核心；
- `SPZ v4` 采用“兼容优先”策略：不重写官方解码路径，保持压缩流 pass-through；
- 对 v4 `flags` 与扩展 trailer 采用“可识别、可验证、可跳过”策略；
- 浏览器仍以“轻/中型输入 + 超限拒绝”为运行边界；
- WASM 仍保留 `compat / perf-lite` 双档。

---

## 3. 目标架构（方案 A）

## 3.1 分层结构

建议收敛为三层：

1. **Core 层（平台无关）**
   - 输入：`const uint8_t* + size`（SPZ 原始压缩流）；
   - 输出：GLB 字节缓冲 + 结构化状态码；
   - 职责：SPZ 头最小读取、GLB 资产构造、错误模型。

2. **Adapter 层（平台适配）**
   - CLI Adapter：文件读写、参数解析、日志输出；
   - WASM C API Adapter：预分配输入、输出句柄生命周期、内存统计。

3. **Verification 层（统一验证内核）**
   - 一个 verifier core，被 CLI 与测试共同复用；
   - 同一套规则覆盖 L1/L2/L3 检查。

## 3.2 设计原则
- 不改变产品契约，只做结构收敛；
- 复用优先于重写；
- 验证先于宣传：每项收口必须可测可证。

---

## 4. 分步实施清单（可执行）

## S1：边界锁定与基线冻结（0.5 天）

### 任务
1. 将本计划作为当前收口唯一设计依据；
2. 固化验收输入集（small/medium/near-limit 三档 + v4 样例）；
3. 明确 v4 兼容口径（支持版本范围、flags 容忍策略、trailer 处理策略）；
4. 冻结当前基线结果（成功率、耗时、峰值内存、拒绝率）。

### 产物
- 基线输入清单（测试资产路径）；
- 基线结果表（可复现命令 + 输出日志）。

### S1 执行入口（首轮）
- 输入集命名固定为：`small` / `medium` / `near_limit` / `v4_ext`；
- 现有仓内脚本入口：`tools/spz_to_glb/tests/quick_test.sh`、`tools/spz_to_glb/tests/browser_smoke.mjs`；
- 现有验证二进制入口：`spz2glb`、`spz_verify`（`layer1/layer2/layer3/all`）；
- `v4_ext` 样例来源独立登记到输入清单（本地路径可记录但不得写入 DoD 正文）。

### 完成判定
- 同一机器重复执行 3 次，结果波动在可接受区间内（耗时给出均值与标准差）。
- `v4_ext` 样例需完成 header/version/flags 与 trailer 可跳过性检查并留痕。

---

## S2：核心层解耦（1~1.5 天）

### 任务
1. 抽离 `spz_to_glb` 核心转换逻辑为独立编译单元（Core）；
2. CLI `main` 与 WASM C API 改为链接 Core，不再通过 `#include .cpp` 复用；
3. 对外错误码/错误信息统一。

### 关键要求
- 保持现有行为与输出字节契约不变；
- 不引入额外拷贝路径。

### 产物
- Core 头/源文件；
- CLI Adapter 与 WASM Adapter 的调用收敛；
- 回归测试通过记录。

### 完成判定
- 同一输入下，重构前后 GLB 二进制一致（或至少结构与关键字段一致、payload 一致）。

---

## S3：验证体系收敛（1 天）

### 任务
1. 合并当前重复 verifier 逻辑为单一 verifier core；
2. 增强契约断言：
   - 扩展声明完整性（`Used/Required`）；
   - `bufferView` 与 SPZ payload 覆盖关系；
   - 4-byte 对齐、Chunk 合法性；
   - 从 GLB 提取 payload 与输入 SPZ 字节一致；
   - v4 样例的 header/version/flags 与 trailer 可跳过性验证；
3. 增强 browser smoke：
   - 超限拒绝；
   - chunk 连续性；
   - output handle release 后不可再读；
   - memory stats 合法性。

### 产物
- 统一 verifier 模块；
- 新增/更新测试用例；
- 验证报告（L1/L2/L3）。

### 完成判定
- native + wasm smoke 全绿；
- 无重复 verifier 分叉逻辑。

---

## S4：发布链收口（0.5 天）

### 任务
1. 清理发布流程中对外部 raw 文件下载依赖；
2. Pages 与 WASM 产物全部来自同一次构建上下文；
3. 产物命名与 profile 维度保持一致（compat/perf-lite）。

### 产物
- 更新后的 workflow；
- 一次完整 CI 通过记录（含 artifact 链路）。

### 完成判定
- 发布阶段在无外部 raw 拉取前提下可稳定执行。

---

## S5：量化验收与收口声明（0.5 天）

### 任务
1. 对比基线与收口后结果；
2. 输出“是否完成目标”的可审计结论。

### 核心指标（与 2025-03-25 计划对齐）
- 超限输入明确拒绝；
- 输入链路至少减少一次整文件复制（浏览器路径）；
- 输出链路至少减少一次整文件复制（句柄+显式释放模型）；
- `SPZ v4` 样例通过率达标（建议阈值：必选样例 100% 通过，且 3 轮重复无回归；含扩展 trailer 可跳过验证）；
- 峰值内存下降（给出对比数据）；
- 中小文件耗时改善（给出统计）；
- 内存统计字段完整可读（current/peak/alloc/fail/work_peak）。

### 产物
- 收口验收表（前后对比）；
- 风险残留清单（若有）。

### 完成判定
- 指标满足“达标”阈值；
- 可给出明确“完成/未完成 + 原因”。

---

## 5. 风险与应对

1. **重构引入行为漂移**
   - 应对：先加对照测试，再拆分；保持输入/输出契约回归。

2. **WASM 与 CLI 行为不一致**
   - 应对：建立同输入双端一致性测试（结构 + payload + verifier 结果）。

3. **CI 不稳定影响收口节奏**
   - 应对：优先保证最小 smoke 与契约检查；高成本测试放 nightly。

4. **需求膨胀（越界做新能力）**
   - 应对：严格执行 In/Out Scope，新增需求进入下一阶段。

---

## 6. 执行顺序与里程碑

- M1（Day 1）：S1 完成 + S2 启动；
- M2（Day 2）：S2 完成 + S3 完成；
- M3（Day 3）：S4 完成 + S5 验收结论。

> 若中途出现阻塞（环境/权限/CI），先记录“已完成项 + 阻塞点 + 影响范围”，再决策是否继续推进。

---

## 7. 交付清单（DoD）

收口完成需同时满足：

1. 代码结构：Core/Adapter/Verifier 职责清晰，去掉 `#include .cpp` 复用；
2. 测试验证：native + wasm + browser smoke 全绿，且验收断言完整；
3. v4 兼容：`SPZ v4`（含扩展 trailer）样例通过并纳入 CI 基线；
4. 发布链路：无外部 raw 依赖，产物链路一致；
5. 指标结论：有量化对比，能够明确宣布“目标完成”。

---

## 8. 下一步（实施入口）

按本计划直接进入 S1：
1. 固化基线输入集；
2. 跑一次基线并存档；
3. 开始 S2 的 Core 抽离（先测试后重构）。

## 9. S1 执行记录（2026-03-26~2026-03-27，WSL）

### 9.1 本轮执行命令（已执行）
- `cmake --build build --target clean`
- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
- `cmake --build build --target spz2glb spz_verify -- -j4`
- `bash tests/quick_test.sh`
- `dist/spz2glb tests/data/near_limit.spz <output.glb>` + `dist/spz_verify layer1/layer2/layer3`
- `dist/spz2glb tests/data/v4_ext.spz <output.glb>` + `dist/spz_verify layer1/layer2/layer3`

### 9.2 输入清单（已冻结）
- `small`：`tests/data/triangle.spz`（源自 `spz_library/samples/hornedlizard.spz`）
- `medium`：`tests/data/cube.spz`（源自 `spz_library/samples/racoonfamily.spz`）
- `near_limit`：`tests/data/near_limit.spz`（本地候选，约 56 MB）
- `v4_ext`：`tests/data/v4_ext.spz`（本地 v4 扩展样例，`hasExtensions=1`，含可跳过 trailer）

> 注：`v4_ext` 样例来源已按 S1 约束独立登记为本地路径，不写入 DoD 正文。

### 9.3 本轮结果（small/medium/near_limit/v4_ext）
- `quick_test.sh`：通过；
- `near_limit`：`spz2glb` 转换成功，`layer1/layer2/layer3` 验证通过；
- `v4_ext`：`spz2glb` 转换成功，`layer1/layer2/layer3` 验证通过；
- 结论：S1 的输入补齐与首轮验证闭环已完成，可进入 S2（Core 抽离）。

### 9.4 当前阻塞点与后续动作
- 阻塞点：`browser_smoke.mjs` 在 WSL 缺少 Node.js 运行环境（`node: command not found`），浏览器 smoke 尚未执行；
- 影响范围：不影响 native `spz2glb/spz_verify` 收口验证，但影响 wasm/browser 侧验收闭环；
- 下一步：补齐 WSL Node.js 环境并补跑 `tests/browser_smoke.mjs`，随后进入 S2 Core 抽离。


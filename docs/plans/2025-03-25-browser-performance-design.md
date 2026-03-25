# SPZ to GLB 浏览器性能版实施计划

> **目标：** 保留 Emscripten glue 加载方式，优先实现浏览器端轻量稳定与少拷贝优化

**架构方向：** 不重写加载器，重点改造内存路径（预分配输入缓冲 + 显式输出释放 + 分级限额），追求"更稳 + 更省内存 + 中等提速"的真实收益。

**技术栈：** Emscripten、C++17、WebAssembly、JavaScript (ES6)

---

## 目标边界

- **保留**现有 Emscripten glue 加载方式，不再追纯 `WebAssembly.instantiate()` 重构
- 浏览器端只做**轻量/中等输入**，超限直接拒绝
- 优先拿到三类真实收益：
  1. 峰值内存下降
  2. 浏览器稳定性提升  
  3. 转换耗时中等改善
- "流式"定义为：**分块导入到预分配 WASM 缓冲 + 有界内存运行**，不是彻底流式转换

---

## 实施分期

### Phase 0：基线与清理

**涉及文件：**
- `tools/spz_to_glb/CMakeLists.txt`
- `tools/spz_to_glb/src/*`
- `tools/spz_to_glb/docs/examples/*`

**动作：**
- 先对当前分支和 `main` 做一次差异审查，清理 AI 痕迹、无效防御式代码、类型绕过
- 建立性能基线：记录当前版本的
  - 最大可处理文件大小
  - 峰值内存
  - 转换耗时
  - 失败模式（OOM / 页面卡死 / growth 抖动）

> **注意：** 当前 Windows shell 里 `git` 不在 PATH，真正开工前这一步需要在可用 git 的环境里补做

---

### Phase 1：浏览器侧预检与分级限额

**涉及文件：**
- `tools/spz_to_glb/docs/examples/index.html`
- `tools/spz_to_glb/docs/examples/smart_memory.js`
- `tools/spz_to_glb/docs/examples/spz2glb_bindings.js`

**目标：** 在真正分配大块内存前，先判断"能不能转"

**具体做法：**
- 基于 `navigator.deviceMemory`、`hardwareConcurrency`、可选 UA 特征做设备分级
- 增加三档策略：`light` / `standard` / `reject`
- 结合 `.spz` 文件大小做硬阈值限制
- 只做**轻量头部预检**，不给 `.spz` 做错误的业务前置拦截
- UI 明确提示：当前设备档位、文件是否超限、建议改用 CLI

**建议初始阈值：**
- 低内存设备：`16-24 MB`
- 中档设备：`32-48 MB`
- 高档设备：`64 MB` 左右

后续按实测调优

---

### Phase 2：WASM C API 重构为"缓冲区/句柄"模型

**涉及文件：**
- `tools/spz_to_glb/src/spz2glb_wasm_c_api.h`
- `tools/spz_to_glb/src/spz2glb_wasm_c_api.cpp`

**目标：** 把现在"一次性传入 + 内部再拷贝"的接口，改成"**预分配输入缓冲 + 原地填充 + 显式释放输出**"

**新增接口：**
```c
// 预留输入缓冲区
size_t spz2glb_reserve_input(size_t size);
uint8_t* spz2glb_get_input_ptr(void);

// 使用预留缓冲区进行转换  
int spz2glb_convert_reserved_input(size_t size, uint8_t** out_ptr, size_t* out_size);

// 显式释放输出
void spz2glb_release_output(uint8_t* ptr);

// 内存统计
Spz2GlbMemoryStats spz2glb_get_memory_stats(void);
void spz2glb_reset_memory_stats(void);
```

**兼容策略：**
- 旧的 `spz2glb_convert(...)` 先保留，内部转调新路径，避免示例页一次性全坏

---

### Phase 3：核心转换链路去全量复制

**涉及文件：**
- `tools/spz_to_glb/src/spz_to_glb.cpp`
- `tools/spz_to_glb/src/spz2glb_wasm_c_api.cpp`

**目标：** 去掉当前主路径里的整文件 `vector` 复制

**核心改动：**
- 把核心接口从偏 `std::vector<uint8_t>` 风格，改成更底层的 `const uint8_t* + size` 或只读 span

**去掉的拷贝链：**
- ~~WASM heap -> std::vector input~~
- ~~std::vector output -> malloc result -> memcpy~~

**改成：**
- 结果由 WASM 侧统一持有
- JS 只拿 `ptr + size`
- 最后显式释放

> 这是性能收益最大的阶段

---

### Phase 4：把 `memory_pool.h` 真正接入主路径

**涉及文件：**
- `tools/spz_to_glb/src/memory_pool.h`
- `tools/spz_to_glb/src/spz_to_glb.cpp`
- `tools/spz_to_glb/src/spz2glb_wasm_c_api.cpp`

**目标：** 让"智能内存管理"从概念变成真实热路径能力

**做法：**
- 为短生命周期工作区引入 `arena / bump allocator`
- 将解压、临时拼装、校验中的 scratch buffer 尽量接入 arena
- 热点小对象走 fixed pool；大块输出仍走明确的大缓冲策略
- 统一内存统计口径，避免现在"统计值和真实占用脱节"

**注意：**
- 这里不追求过度抽象，先只接主热路径
- `memory_pool.h` 里占位的统计接口要和 WASM 实际统计统一

---

### Phase 5：浏览器侧"少拷贝"与有界加载

**涉及文件：**
- `tools/spz_to_glb/docs/examples/spz2glb_bindings.js`
- `tools/spz_to_glb/docs/examples/index.html`

**目标：** 浏览器端把整文件复制次数降到最低

**做法：**
- 文件读取改为分块写入预分配的 WASM 输入区
- JS 侧只保留一份必要的文件来源对象
- 输出优先暴露为 `HEAPU8.subarray(...)`
- 只有在最终下载/保存时再做一次必要复制

**关键约束：**
- 如果保留 `ALLOW_MEMORY_GROWTH=1`，转换后必须重新获取 `HEAPU8` 视图，不能缓存旧视图
- 所以"零拷贝"要按生命周期来做，不能假设视图永久有效

---

### Phase 6：构建参数与运行档位调优

**涉及文件：**
- `tools/spz_to_glb/CMakeLists.txt`

**目标：** 把"稳定版"和"性能版"的参数边界清晰化

**建议保留：**
- `MODULARIZE=1`
- `EXPORT_ES6=1`
- 纯 C API 导出

**评估两档配置：**
- `compat`：`ALLOW_MEMORY_GROWTH=1`
- `perf-lite`：固定 `INITIAL_MEMORY`，不增长，专供轻量文件

**其他：**
- `MALLOC` 暂不盲改，先 benchmark 比较 `emmalloc` 和默认实现
- 若无文件系统依赖，可考虑 `FILESYSTEM=0`

---

### Phase 7：验收与自动化

**涉及文件：**
- `tools/spz_to_glb/tests/*`
- GitHub Actions 中的 WASM 构建/浏览器 smoke

**验收指标：**
- [ ] 浏览器端对超限输入**明确拒绝**，不再直接炸标签页
- [ ] 输入链路减少至少一次整文件复制
- [ ] 输出链路减少至少一次整文件复制
- [ ] 峰值内存相比当前版本明显下降
- [ ] 中小文件转换耗时有可重复的改善
- [ ] 内存统计可输出：当前占用、峰值占用、分配次数、失败次数

---

## 推荐实施顺序

按收益/风险比，建议这样落地：

1. **Phase 1** 预检与限额
2. **Phase 2** 新 C API 骨架
3. **Phase 3** 去输入/输出全量复制
4. **Phase 4** 内存池接主路径
5. **Phase 6** 构建参数调优
6. **Phase 7** 指标验收

---

## 第一批落地范围

建议首批只做这 **3 件事**：
1. 浏览器设备分级 + 文件大小硬限制
2. 新的预分配输入缓冲 C API
3. JS 侧改成"分块写入 WASM + 输出显式释放"

这样能最快把"**稳定性**"和"**少拷贝**"先做出来，再继续深挖内存池。

---

## 附录：关键设计决策

### 为什么不追纯原生 API？

之前尝试过纯 `WebAssembly.instantiate()`，但被导入名压缩和运行时兼容性反复卡住。而且热路径性能大头不在"胶水 vs 原生 instantiate"，而在**多次拷贝 + 内存增长 + 分配方式**。

### 浏览器端"真正的大文件流式转换"能否实现？

**结论：不能完全实现**

原因是当前转换实现最终仍需要拿到完整输入并构造输出对象。更现实的定义应是：
- **流式读取进入预分配缓冲**
- 避免中间 JS 大对象反复复制
- 尽量不产生第二份、第三份整文件副本

这叫"**分块导入 + 有界内存**"，比"完全流式零拷贝转换"更现实，也更适合浏览器。

### 预期收益排序

如果按本计划实施，预期真实收益排序：

1. **浏览器稳定性提升最大** ✓
2. **峰值内存下降第二大** ✓
3. **转换耗时中等改善**
4. **冷启动改善最小**

也就是说，本方案的核心价值是：
- 从"容易炸"变成"可控"
- 从"高峰内存不可预测"变成"预算内运行"
- 再顺带拿到一部分性能提升

而不是宣传式的"原生高性能神优化"。

---

*文档生成时间：2025-03-25*  
*适用项目：spz_ecosystem_simplified/tools/spz_to_glb*

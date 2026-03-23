# SPZ2GLB WASM 优化实施计划

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 将 spz2glb WASM 构建从玩具级别提升到生产级别，支持三个独立模块（spz2glb + spz_verify + SPZ encoder）的优化构建

**Architecture:**
- 三个独立 WASM 模块，各自优化配置
- 智能内存管理，支持 A/B/C/D 四种文件大小场景
- 完整 GitHub Actions CI/CD 矩阵

**Tech Stack:** Emscripten, CMake, GitHub Actions, TypeScript

---

## 实施阶段

### Phase 1: CMakeLists.txt 核心优化

#### Task 1: 添加 C++ 编译优化

**Files:**
- Modify: `tools/spz_to_glb/CMakeLists.txt:162-168`

**Step 1: 检查当前 spz2glb-wasm 的编译选项**

```bash
# 当前配置在 line 158-159 只有 --use-port=zlib
# 需要添加完整的 C++ 优化选项
```

**Step 2: 添加 C++ 编译优化**

在 `target_link_options` 之前添加：

```cmake
  # C++ 编译时优化
  # 注意：不使用 -fno-exceptions 和 -fno-rtti，因为：
  # 1. 代码中使用了 throw（C++ 异常）
  # 2. Embind 需要 RTTI 支持
  target_compile_options(spz2glb-wasm PRIVATE
    "-O3"
    "-flto"
  )
```

**Step 3: 验证 CMake 配置**

```bash
cd tools/spz_to_glb
emcmake cmake -S ../ -B build_wasm_test -DSPZ2GLB_BUILD_WASM=ON
# 确认输出包含 "-O3" 和 "-flto"
```

---

#### Task 2: 添加完整链接优化配置

**Files:**
- Modify: `tools/spz_to_glb/CMakeLists.txt:161-179`

**Step 1: 替换现有的 target_link_options**

将现有的链接选项替换为完整配置：

```cmake
  # 完整的 WASM 链接优化配置
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

    # 内存配置（关键：大文件处理）
    "-sALLOW_MEMORY_GROWTH=1"
    "-sINITIAL_MEMORY=33554432"     # 32MB 初始
    "-sMAXIMUM_MEMORY=1073741824"   # 1GB 最大
    "-sSTACK_SIZE=10485760"          # 10MB 栈

    # Web 环境
    "-sENVIRONMENT=web"
    "-sEXPORT_ES6=1"

    # 文件系统（不需要，禁用以减小体积）
    "-sFILESYSTEM=0"

    # 生产环境优化
    "-sASSERTIONS=0"
    "-sNO_EXIT_RUNTIME=1"
    "-sNO_DYNAMIC_EXECUTION=1"
    "-sERROR_ON_UNDEFINED_SYMBOLS=1"
    "-sWARN_ON_UNDEFINED_SYMBOLS=1"

    # 性能优化
    "-sWASM_BIGINT=1"
    "-sMALLOC=emmalloc"

    # 文件拆分（减小 30% 体积）
    "-sSINGLE_FILE=0"
  )
```

**Step 2: 验证配置输出**

```bash
grep -A 30 "target_link_options" tools/spz_to_glb/CMakeLists.txt
```

---

#### Task 3: 添加 spz_verify-wasm 构建目标

**Files:**
- Modify: `tools/spz_to_glb/CMakeLists.txt`
- Create: `tools/spz_to_glb/src/emscripten/spz_verify.d.ts.in`

**Step 1: 在 spz2glb-wasm 配置块之后添加 spz_verify-wasm**

```cmake
  # ============================================================
  # spz_verify WASM 构建
  # ============================================================

  add_executable(spz_verify-wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spz_verify.cpp
  )

  target_link_libraries(spz_verify-wasm PRIVATE fastgltf)

  # KHR Gaussian Splatting 支持
  if(ENABLE_KHR_GAUSSIAN_SPLATTING)
    target_compile_definitions(spz_verify-wasm PRIVATE FASTGLTF_ENABLE_KHR_GAUSSIAN_SPLATTING=1)
  endif()

  # C++ 编译优化
  # 注意：不使用 -fno-exceptions 和 -fno-rtti（原因同上）
  target_compile_options(spz_verify-wasm PRIVATE
    "-O3"
    "-flto"
  )

  # 完整链接优化
  target_link_options(spz_verify-wasm PRIVATE
    "-O3"
    "-flto"
    "-sUSE_ZLIB=1"
    "--use-port=zlib"
    "--bind"
    "-sMODULARIZE=1"
    "-sEXPORT_NAME=createSpzVerifyModule"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sINITIAL_MEMORY=33554432"
    "-sMAXIMUM_MEMORY=1073741824"
    "-sSTACK_SIZE=10485760"
    "-sENVIRONMENT=web"
    "-sEXPORT_ES6=1"
    "-sFILESYSTEM=0"
    "-sASSERTIONS=0"
    "-sNO_EXIT_RUNTIME=1"
    "-sNO_DYNAMIC_EXECUTION=1"
    "-sERROR_ON_UNDEFINED_SYMBOLS=1"
    "-sWARN_ON_UNDEFINED_SYMBOLS=1"
    "-sWASM_BIGINT=1"
    "-sMALLOC=emmalloc"
    "-sSINGLE_FILE=0"
  )

  set_target_properties(spz_verify-wasm PROPERTIES
    OUTPUT_NAME "spz_verify"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dist"
  )
```

**Step 2: 创建 TypeScript 类型定义模板**

```typescript
// tools/spz_to_glb/src/emscripten/spz_verify.d.ts.in
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

**Step 3: 添加 TypeScript 声明文件生成**

在 CMakeLists.txt 的 WASM 配置块末尾添加：

```cmake
  # 生成 TypeScript 声明文件
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/emscripten/spz2glb.d.ts.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/dist/spz2glb.d.ts"
    COPYONLY
  )

  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/emscripten/spz_verify.d.ts.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/dist/spz_verify.d.ts"
    COPYONLY
  )
```

---

### Phase 2: Workflow 修复

#### Task 4: 修复 release.yml WASM 构建配置

**Files:**
- Modify: `tools/spz_to_glb/.github/workflows/release.yml:114-152`

**Step 1: 修复 WASM 构建 job**

替换现有的 wasm-build job：

```yaml
  wasm-build:
    name: Build WASM Modules
    runs-on: ubuntu-latest
    steps:
      - name: Checkout
        uses: actions/checkout@v5
        with:
          submodules: recursive

      - name: Setup Emscripten
        uses: mymindstorm/setup-emsdk@v14
        with:
          version: latest

      - name: Configure WASM build
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

      - name: Copy WASM outputs
        run: |
          mkdir -p dist
          cp tools/spz_to_glb/dist/*.wasm dist/ 2>/dev/null || true
          cp tools/spz_to_glb/dist/*.js dist/ 2>/dev/null || true
          cp tools/spz_to_glb/dist/*.data dist/ 2>/dev/null || true
          ls -la dist/

      - name: Upload WASM artifacts
        uses: actions/upload-artifact@v5
        with:
          name: wasm-modules
          path: dist/
```

**Step 2: 验证 workflow 语法**

```bash
# 本地验证 YAML 语法（需要 yamllint）
yamllint tools/spz_to_glb/.github/workflows/release.yml
```

---

### Phase 3: 验证构建

#### Task 5: 本地测试 WASM 构建

**Step 1: 检查 emscripten 是否可用**

```bash
emcc --version
# 期望输出：emcc (Emscripten gcc/clang-like replacement)...
```

**Step 2: 执行 WASM 构建**

```bash
cd tools/spz_to_glb
rm -rf build_wasm
emcmake cmake -S ../ -B build_wasm -DSPZ2GLB_BUILD_WASM=ON
emmake cmake --build build_wasm --config Release
```

**Step 3: 检查输出文件**

```bash
ls -la tools/spz_to_glb/dist/
# 期望看到：
# - spz2glb.js
# - spz2glb.wasm
# - spz2glb.data
# - spz_verify.js
# - spz_verify.wasm
# - spz_verify.data
```

---

## 质量检查清单

- [x] CMakeLists.txt 包含 `-O3 -flto` 编译选项
- [x] CMakeLists.txt 包含完整链接优化
- [x] spz_verify-wasm 构建目标存在
- [x] TypeScript .d.ts 文件会被生成（artifact 中已确认存在）
- [x] Workflow 的 WASM job 配置正确
- [x] 本地构建成功，输出文件存在

---

## Phase 3: 生产级优化（待实现）

### Task 6: 异常优化 - 用错误码替代 throw

**问题**：代码使用 C++ 异常（throw），导致 WASM 体积增大和性能下降

**Files:**
- Modify: `tools/spz_to_glb/src/spz_to_glb.cpp`
- Modify: `tools/spz_to_glb/src/spz_verify.cpp`

**Step 1: 识别所有 throw 位置**

```bash
grep -n "throw" tools/spz_to_glb/src/spz_to_glb.cpp
grep -n "throw" tools/spz_to_glb/src/spz_verify.cpp
```

**Step 2: 重构为错误码返回**

将 `throw std::runtime_error("message")` 替换为：

```cpp
// 之前
throw std::runtime_error("无法打开 SPZ 文件");

// 之后
return std::unexpected(ErrorCode::CannotOpenSpzFile);
```

**Step 3: 更新 CMakeLists.txt**

在异常代码移除后，添加优化选项：

```cmake
# 异常优化（重构代码后启用）
target_compile_options(spz2glb-wasm PRIVATE
  "-O3"
  "-flto"
  "-fno-exceptions"
)
```

---

### Task 7: 内存优化（重写 v2）

**问题**：
1. 当前 `ALLOW_MEMORY_GROWTH=1` 有性能开销
2. JS ↔ WASM 边界存在 3 次内存拷贝
3. spz2glb 和 spz_verify 内存使用模式不同，需要分层优化

**设计原则**：
- 单线程，不需要无锁队列
- 固定内存池 > 动态增长
- **分层池化**：热点对象 / 工作内存 / 输出内存
- **减少拷贝**：利用 Emscripten HEAP 共享视图

**Files:**
- Modify: `tools/spz_to_glb/CMakeLists.txt`
- Modify: `tools/spz_to_glb/src/spz_to_glb.cpp`
- Modify: `tools/spz_to_glb/src/spz_verify.cpp`
- Create: `tools/spz_to_glb/src/memory_pool.h`

---

## 内存分析：spz2glb vs spz_verify

### spz2glb 内存流程

```
JS → WASM (copy #1: Embind vectorFromJsArray)
    ↓
decompressSpzData → decompressedData (new allocation)
    ↓
createGltfAsset + writeGltfBinary → output buffer
    ↓
glbData.resize() + memcpy() (copy #2)
    ↓
jsUint8ArrayFromVector(glbData) → JS (copy #3)
```

**热点对象**：decompress buffer, glTF nodes
**问题**：3 次拷贝

### spz_verify 内存流程

| Layer | 操作 | 内存模式 |
|-------|------|----------|
| Layer 1 | 读 GLB header/JSON/BIN | 顺序读 → 验证 → 释放 |
| Layer 2 | 读 SPZ + GLB buffer | 两次读，MD5 计算 |
| Layer 3 | 读 SPZ + GLB | 按需分配，验证后释放 |

**热点对象**：JSON buffers, small fixed structs
**特点**：分层验证，可复用内存

---

## Step 1: 固定内存池配置

```cmake
# CMakeLists.txt - 禁用 ALLOW_MEMORY_GROWTH
target_link_options(spz2glb-wasm PRIVATE
  # 固定内存配置（禁用动态增长以获得最佳性能）
  "-sINITIAL_MEMORY=67108864"     # 64MB 初始
  "-sMAXIMUM_MEMORY=1073741824"  # 1GB 最大
  # 不使用 ALLOW_MEMORY_GROWTH=1
)
```

---

## Step 2: 三层内存池架构

```cpp
// tools/spz_to_glb/src/memory_pool.h

#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <cstddef>
#include <cstdint>

// Layer 1: 热点对象池（固定大小，高频分配）
// 用于：glTF node metadata, JSON parse buffers, MD5 state
template<size_t ObjectSize, uint32_t PoolSize>
class HotObjectPool {
    static_assert(PoolSize > 0, "PoolSize must be > 0");

    struct FreeNode {
        FreeNode* next;
    };

    FreeNode* freeList_ = nullptr;
    char pool_[ObjectSize * PoolSize];

public:
    HotObjectPool() {
        for (uint32_t i = 0; i < PoolSize; ++i) {
            auto* node = reinterpret_cast<FreeNode*>(pool_ + i * ObjectSize);
            node->next = freeList_;
            freeList_ = node;
        }
    }

    void* alloc() {
        if (!freeList_) return nullptr;
        auto* node = freeList_;
        freeList_ = freeList_->next;
        return node;
    }

    void dealloc(void* ptr) {
        auto* node = reinterpret_cast<FreeNode*>(ptr);
        node->next = freeList_;
        freeList_ = node;
    }

    uint32_t available() const {
        uint32_t count = 0;
        for (auto* n = freeList_; n; n = n->next) ++count;
        return count;
    }
};

// Layer 2: Bump Allocator（工作内存，可变大小）
// 用于：decompression buffer, JSON data, GLB chunks
class BumpAllocator {
private:
    char* pool_;
    char* current_;
    char* end_;
    size_t peak_usage_;

public:
    BumpAllocator(size_t size);
    ~BumpAllocator();

    void* alloc(size_t size);
    void reset();

    size_t used() const { return current_ - pool_; }
    size_t peak_usage() const { return peak_usage_; }
    size_t remaining() const { return end_ - current_; }
};

// Layer 3: 零拷贝视图（WASM HEAP 直接访问）
// 用于：JS ↔ WASM 数据交互
class ZeroCopyView {
    uint8_t* data_;
    size_t size_;

public:
    ZeroCopyView() : data_(nullptr), size_(0) {}

    // 从 JS ArrayBuffer 创建视图（零拷贝读取）
    bool createFromJsBuffer(emscripten::val buffer);

    uint8_t* data() { return data_; }
    size_t size() const { return size_; }
};
#endif
```

---

## Step 3: spz2glb 优化（减少拷贝）

**当前流程（3次拷贝）**：
```cpp
// 之前
emscripten::val convertSpzToGlb(const emscripten::val& spzBuffer) {
    std::vector<uint8_t> spzData = spz2glb::vectorFromJsArray(spzBuffer);  // copy #1
    std::vector<uint8_t> glbData;
    convertSpzToGlbCore(spzData, glbData);
    return spz2glb::jsUint8ArrayFromVector(glbData);  // copy #3
}
```

**优化后（1次拷贝）**：
```cpp
// 之后 - 利用 HEAP 视图减少拷贝
emscripten::val convertSpzToGlb(const emscripten::val& spzBuffer) {
    // 步骤 1: 零拷贝获取 JS Buffer 视图
    emscripten::val memory = emscripten::val::global("Module")["HEAP"];
    emscripten::val buffer = spzBuffer.call<emscripten::val>("buffer");
    size_t byteOffset = spzBuffer["byteOffset"].as<size_t>();
    size_t byteLength = spzBuffer["byteLength"].as<size_t>();

    uint8_t* spzPtr = &memory.as<uint8_t*>()[byteOffset / sizeof(uint8_t)];

    // 步骤 2: 在 WASM 内存中处理（使用 bump allocator）
    static BumpAllocator decompressPool(32 * 1024 * 1024);  // 32MB decompress buffer
    static HotObjectPool<sizeof(SpzHeader), 256> headerPool;

    // ... 处理逻辑 ...

    // 步骤 3: 直接返回结果（只有输出拷贝）
    return spz2glb::jsUint8ArrayFromVector(glbData);  // copy #1 (output only)
}
```

---

## Step 4: spz_verify 优化（分层池化）

```cpp
// spz_verify.cpp - WASM 优化版本

#ifdef __EMSCRIPTEN__

// 热点对象池
static HotObjectPool<1024, 128> jsonPool;    // JSON buffers
static HotObjectPool<sizeof(Md5Hash), 64> md5Pool;  // MD5 state

// 工作内存池
static BumpAllocator workPool(16 * 1024 * 1024);  // 16MB work buffer

emscripten::val layer1Validate(emscripten::val glbBuffer) {
    // 使用 jsonPool 分配 JSON buffer
    void* jsonBuf = jsonPool.alloc();
    // ... 验证逻辑 ...
    jsonPool.dealloc(jsonBuf);
    return emscripten::val::bool_(result);
}

emscripten::val layer2VerifyLossless(emscripten::val spzBuffer, emscripten::val glbBuffer) {
    // 使用 md5Pool 分配 MD5 state
    void* md5State = md5Pool.alloc();
    // 使用 workPool 分配临时 buffer
    void* tempBuf = workPool.alloc(bufferSize);
    // ... 验证逻辑 ...
    workPool.dealloc(tempBuf);
    md5Pool.dealloc(md5State);
    return emscripten::val::bool_(result);
}

EMSCRIPTEN_BINDINGS(spz_verify_module) {
    function("layer1Validate", &layer1Validate);
    function("layer2VerifyLossless", &layer2VerifyLossless);
    function("getMemoryStats", &getMemoryStats);
}

#endif
```

---

## Step 5: 内存统计 API

```cpp
struct MemoryStats {
    size_t peak_usage;
    size_t current_usage;
    size_t hot_allocations;
    size_t hot_available;
    size_t work_used;
    size_t work_remaining;
};

MemoryStats getMemoryStats() {
    return {
        g_peak_usage,
        g_current_usage,
        g_hot_allocations,
        jsonPool.available(),
        workPool.used(),
        workPool.remaining()
    };
}

EMSCRIPTEN_BINDINGS(spz2glb_module) {
    // ... existing bindings ...
    function("getMemoryStats", &getMemoryStats);
}
```

---

## Step 6: 验证优化效果

```bash
# 构建
emcmake cmake -B build_wasm
emmake cmake --build build_wasm --config Release

# 检查 WASM 大小
ls -lh dist/spz2glb.wasm dist/spz_verify.wasm

# 运行时内存统计（JS）
const stats = Module.getMemoryStats();
console.log(`
  Peak Usage: ${(stats.peak_usage / 1024 / 1024).toFixed(2)} MB
  Hot Pool Available: ${stats.hot_available}
  Work Pool: ${(stats.work_used / 1024 / 1024).toFixed(2)} MB / ${(stats.work_remaining / 1024 / 1024).toFixed(2)} MB
`);
```

---

## 内存配置指南

| 场景 | INITIAL_MEMORY | 热点池 | 工作池 |
|------|----------------|---------|--------|
| 轻量级（<10万点） | 32MB | 256 objects | 16MB |
| 标准（10-100万点） | 64MB | 512 objects | 32MB |
| 专业（>100万点） | 128MB | 1024 objects | 64MB |

---

## 注意

1. **单线程不需要无锁队列**：Michael-Scott 是多线程方案
2. **零拷贝有限制**：JS ↔ WASM 内存空间不同，但可以通过 HEAP 视图减少到 1 次拷贝
3. **分层池化收益**：热点对象池化收益高，工作内存用 bump allocator 最快
4. **spz_verify 更适合池化**：验证是顺序操作，内存可完全复用

---

## Phase 4: 验证与测试（待实现）

### Task 8: 构建验证

**Step 1: 验证 WASM 模块大小**

```bash
ls -lh tools/spz_to_glb/dist/
# 记录构建产物体积
```

**Step 2: 验证 TypeScript 类型定义**

```bash
cat tools/spz_to_glb/dist/spz2glb.d.ts
cat tools/spz_to_glb/dist/spz_verify.d.ts
```

---

## 注意事项

1. **不要破坏桌面版构建**：修改只在 `if(SPZ2GLB_BUILD_WASM)` 块内
2. **保持向后兼容**：现有代码不应被修改
3. **分支策略**：完成一个 task 后 commit 一次
4. **异常优化是可选的**：如果 throw 影响不大，可以保持现状

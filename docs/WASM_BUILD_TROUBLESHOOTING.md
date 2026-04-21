# WASM 构建问题排查总结

## 问题概述

在构建 SPZ 到 GLB 转换器的 WASM 版本时，遇到了两个主要错误：

1. **`Import #0 module="a" error: module is not an object or function`**
   - 原生 WebAssembly.instantiate() 加载失败
   - Emscripten 将导入模块名 `env` 压缩成了 `a`

2. **`missing function: _embind_register_function`**
   - 即使禁用了 embind，仍然出现 embind 注册函数错误
   - 原因是 `spz_to_glb.cpp` 中的 embind 代码被 C API 文件间接包含

---

## 尝试过的解决方案

### 方案 1: 使用 `-sSTANDALONE_WASM=1`
**结果**: ❌ 失败

```cmake
# 尝试生成独立 WASM
target_link_options(spz2glb-wasm PRIVATE
    "-sSTANDALONE_WASM=1"
)
```

**问题**: 仍然出现导入名压缩，且无法正确导出 C API 函数。

---

### 方案 2: 禁用导入名压缩
**结果**: ❌ 失败

```cmake
target_link_options(spz2glb-wasm PRIVATE
    "--minify=0"
    "-sLEGALIZE_JS_FFI=0"
)
```

**问题**: Emscripten 仍然压缩导入名，`env` 变成 `a`。

---

### 方案 3: 使用 dlmalloc 替代 emmalloc
**结果**: ❌ 失败

```cmake
target_link_options(spz2glb-wasm PRIVATE
    "-sMALLOC=dlmalloc"
)
```

**问题**: 更换内存分配器没有解决导入名压缩问题。

---

### 方案 4: 在 JS 中处理压缩后的导入名
**结果**: ❌ 失败

```javascript
// 尝试适配压缩后的导入名
const imports = {
    a: {  // 压缩后的 env
        __memory_base: 0,
        __table_base: 0,
        memory: new WebAssembly.Memory({ initial: 256 }),
        // ...
    }
};
```

**问题**: 压缩后的名称不稳定，且内部调用关系复杂，无法可靠地手动映射。

---

### 方案 5: 使用 Emscripten JS 胶水代码 ✅
**结果**: ✅ 成功（采用方案）

```cmake
target_link_options(spz2glb-wasm PRIVATE
    "-sMODULARIZE=1"
    "-sEXPORT_NAME=createSpz2glbModule"
)
```

**优点**:
- JS 胶水代码自动处理所有导入/导出
- 不需要手动管理内存分配器导入
- 支持 ES6 模块

**缺点**:
- 增加了 JS 文件依赖
- 不是纯 WASM 方案

---

### 方案 6: 完全禁用 embind
**结果**: ✅ 成功（修复 embind 错误）

**问题根源**: `spz2glb_wasm_c_api.cpp` 包含了 `spz_to_glb.cpp`，而后者包含 embind 代码。

**修复方法**:

1. 在 `spz_to_glb.cpp` 中添加条件编译：
```cpp
#ifdef __EMSCRIPTEN__
#ifndef SPZ2GLB_DISABLE_EMBIND
#include <emscripten/bind.h>
#include "emscripten_utils.h"
#endif
#endif

// ...

#ifndef SPZ2GLB_DISABLE_EMBIND
EMSCRIPTEN_BINDINGS(spz2glb_module) {
    emscripten::function("convertSpzToGlb", &convertSpzToGlb);
    emscripten::function("getMemoryStats", &spz2glb::getMemoryStats);
}
#endif
```

2. 在 `spz2glb_wasm_c_api.cpp` 中定义宏：
```cpp
#define SPZ2GLB_DISABLE_EMBIND 1
#include "spz2glb_wasm_c_api.h"
```

---

## 最终 CMakeLists.txt 配置

```cmake
# 链接优化（使用 JS 胶水代码，禁用 embind）
target_link_options(spz2glb-wasm PRIVATE
  "-O3"
  "-fno-lto"
  "-sUSE_ZLIB=1"
  "--use-port=zlib"

  # 内存配置
  "-sINITIAL_MEMORY=67108864"
  "-sMAXIMUM_MEMORY=1073741824"
  "-sALLOW_MEMORY_GROWTH=1"
  "-sSTACK_SIZE=10485760"

  # Web 环境
  "-sENVIRONMENT=web"
  "-sEXPORT_ES6=1"

  # 文件系统
  "-sFILESYSTEM=0"

  # 生成 JS 胶水代码
  "-sMODULARIZE=1"
  "-sEXPORT_NAME=createSpz2glbModule"

  # 禁用 embind（避免 _embind_register_function 错误）
  "-sEMBIND_STD_STRING_IS_UTF8=0"
  "-sDISABLE_EXCEPTION_CATCHING=1"

  # 生产优化
  "-sASSERTIONS=0"
  "-sNO_EXIT_RUNTIME=1"
  "-sNO_DYNAMIC_EXECUTION=1"
  "-sERROR_ON_UNDEFINED_SYMBOLS=0"
  "-sWASM_BIGINT=1"
  "-sMALLOC=emmalloc"

  # 单独文件
  "-sSINGLE_FILE=0"

  # C API 导出 + malloc/free
  "-sEXPORTED_FUNCTIONS=_spz2glb_alloc,_spz2glb_free,_spz2glb_convert,_spz2glb_validate_header,_spz2glb_get_version,_spz2glb_get_memory_stats,_spz2glb_reset_memory_stats,_malloc,_free"
  "-sEXPORTED_RUNTIME_METHODS=ccall,cwrap,getValue,setValue,UTF8ToString,stringToUTF8,lengthBytesUTF8"
)
```

---

## 替代方案: WASI SDK

如果仍希望使用纯 WASM 而不依赖 Emscripten JS 胶水代码，可以考虑使用 **WASI SDK**：

### WASI SDK 优势
1. **纯 WASM 输出**: 不生成 JS 胶水代码
2. **标准导入**: 使用 WASI 标准接口，导入名不会压缩
3. **独立运行**: 可以在任何 WASI 运行时中执行

### WASI SDK 使用步骤

```bash
# 1. 安装 WASI SDK
wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-24/wasi-sdk-24.0-x86_64-linux.tar.gz
tar xzf wasi-sdk-24.0-x86_64-linux.tar.gz
export WASI_SDK_PATH=/path/to/wasi-sdk-24.0-x86_64-linux

# 2. 配置 CMake 使用 WASI 工具链
cmake -DCMAKE_TOOLCHAIN_FILE=$WASI_SDK_PATH/share/cmake/wasi-sdk.cmake \
      -DWASI_SDK_PREFIX=$WASI_SDK_PATH \
      -DCMAKE_BUILD_TYPE=Release \
      ..

# 3. 构建
make spz2glb-wasm
```

### WASI SDK CMake 配置

创建 `cmake/wasi-toolchain.cmake`:
```cmake
set(CMAKE_SYSTEM_NAME WASI)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR wasm32)
set(CMAKE_C_COMPILER ${WASI_SDK_PREFIX}/bin/clang)
set(CMAKE_CXX_COMPILER ${WASI_SDK_PREFIX}/bin/clang++)
set(CMAKE_AR ${WASI_SDK_PREFIX}/bin/llvm-ar)
set(CMAKE_RANLIB ${WASI_SDK_PREFIX}/bin/llvm-ranlib)
set(CMAKE_C_FLAGS "--target=wasm32-wasi")
set(CMAKE_CXX_FLAGS "--target=wasm32-wasi")
```

### WASI SDK 注意事项

1. **没有浏览器 API**: WASI 是服务器端标准，没有 DOM、fetch 等浏览器 API
2. **需要 polyfill**: 在浏览器中使用需要 wasi polyfill
3. **ZLIB 需要重新编译**: WASI SDK 不包含 ZLIB，需要手动编译

---

## 项目路径

### 本地仓库路径
```
c:\Users\HP\Downloads\spz_ecosystem_simplified\tools\spz_to_glb\
```

### 关键文件
- `src/spz_to_glb.cpp` - 主转换逻辑（含 embind，条件编译）
- `src/spz2glb_wasm_c_api.cpp` - 纯 C API 实现（禁用 embind）
- `src/spz2glb_wasm_c_api.h` - C API 头文件
- `CMakeLists.txt` - 构建配置
- `docs/examples/spz2glb_bindings.js` - JS 绑定（使用 JS 胶水代码）

### 云端仓库
```
https://github.com/spz-ecosystem/spz2glb
```

### GitHub Pages 部署
```
https://spz-ecosystem.github.io/spz2glb/
```

---

## 经验教训

1. **Emscripten 导入名压缩**: 即使使用 `-fno-lto` 和 `--minify=0`，Emscripten 仍可能压缩导入名。最可靠的方案是使用 JS 胶水代码。

2. **embind 残留**: 禁用 embind 链接器标志不足以完全移除 embind，需要确保源代码中也没有 embind 代码被编译。

3. **条件编译**: 使用宏（如 `SPZ2GLB_DISABLE_EMBIND`）可以灵活控制代码编译，支持多种构建模式。

4. **WASI SDK**: 对于需要纯 WASM 的场景，WASI SDK 是更好的选择，但需要额外处理浏览器环境的适配。

---

## 提交历史与方案演进

以下是 WASM 构建相关的 Git 提交历史，展示了方案的演进过程：

```
f1bd9ac (HEAD -> main, origin/main) fix: 添加 fastgltf include 路径
755e14f fix: 修复 fastgltf 目录路径
7f0e981 fix: 禁用 LTO 优化，使用原生 WebAssembly API 加载 WASM
8e9d5bd fix: 移除 -sIMPORTED_MEMORY=1 解决 LTO 导入名压缩问题
a5ce574 fix: 修复 bindings 支持外部传入内存，实现智能内存管理+高性能
24c95ea feat: 使用高性能原生 WebAssembly API 替代 Emscripten 胶水代码
2bad0fb 修复 WASM 加载错误: Cannot use import.meta outside a module
b9e6ff6 fix: restore smart_memory.js download in workflow
50cf3e7 fix: update workflow for root-level build
3d5bf47 fix: use Emscripten JS glue for WASM loading
0e255aa Remove redundant tools/spz_to_glb directory
390a3db fix: correct WASM artifact paths
106fa4a feat: add --verify option for three-layer verification
0110550 fix: workflow paths, remove WASM from verify.html
87096a3 fix: add IMPORTED_MEMORY=1 for smart memory management
024fc97 trigger: rebuild GitHub Pages
9b21504 Merge pull request #5 from spz-ecosystem/wasm-optimization
83f6e3c fix: disable strict conversion warnings for fastgltf compatibility
f100694 fix: disable -Wconversion and -Wsign-conversion
bbb3ab6 fix: resolve compiler warnings
6eb3729 fix: resolve compilation errors
8f51927 fix: add underscore prefix to WASM function calls
9b7f581 refactor: remove Embind version, keep only pure C API WASM
```

### 提交历史分析

#### 阶段 1: 初始 Emscripten 方案 (3d5bf47 之前)
- 使用 Emscripten JS 胶水代码加载 WASM
- 遇到 `import.meta` 错误，使用动态 `import()` 修复

#### 阶段 2: 高性能原生 WebAssembly API 尝试 (24c95ea - 7f0e981)
- **目标**: 移除 JS 胶水代码，使用原生 `WebAssembly.instantiate()`
- **尝试的优化**:
  - 使用 `-sIMPORTED_MEMORY=1` 实现智能内存管理
  - 禁用 LTO (`-fno-lto`) 避免导入名压缩
  - 修复 bindings 支持外部传入内存
- **结果**: 遇到 `Import #0 module="a"` 错误，导入名仍被压缩

#### 阶段 3: 回归 JS 胶水代码 + 修复 embind (8e9d5bd - f1bd9ac)
- 重新使用 Emscripten JS 胶水代码
- 修复 embind 残留问题（添加 `SPZ2GLB_DISABLE_EMBIND` 宏）
- 修复 fastgltf 路径问题

### 关键转折点

| 提交 | 方案 | 结果 |
|------|------|------|
| 3d5bf47 | Emscripten JS 胶水 | ✅ 基础工作 |
| 24c95ea | 原生 WebAssembly API | ⚠️ 尝试高性能方案 |
| 7f0e981 | 禁用 LTO + 原生 API | ❌ 导入名仍被压缩 |
| 8e9d5bd | 回归 JS 胶水 | ✅ 稳定方案 |
| f1bd9ac | 修复 embind + 路径 | ✅ 最终修复 |

---

## 参考链接

- [Emscripten 文档](https://emscripten.org/docs/)
- [WASI SDK](https://github.com/WebAssembly/wasi-sdk)
- [WebAssembly 导入/导出](https://webassembly.github.io/spec/core/syntax/modules.html#imports)

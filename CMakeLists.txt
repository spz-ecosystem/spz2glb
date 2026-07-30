cmake_minimum_required(VERSION 3.16)
project(spz2glb VERSION 2.0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 选项
option(SPZ2GLB_BUILD_WASM "Build WASM version" OFF)

# 添加 fastgltf (文件直接在 third_party 目录下)
add_subdirectory(third_party)

# 添加 fastgltf include 路径
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/third_party/include)

# ============================================================
# 根据编译器设置警告选项
# ============================================================

if(MSVC)
  # MSVC 编译器选项
  set(STRICT_WARNINGS
    "/W4"
    "/WX"
    "/wd4100"  # 禁用未使用参数警告
  )
else()
  # GCC/Clang 编译器选项
  set(STRICT_WARNINGS
    "-Wall"
    "-Wextra"
    "-Wpedantic"
    "-Werror"
    "-Wno-unused-parameter"
  )
endif()

add_library(spz2glb_core
  ${CMAKE_CURRENT_SOURCE_DIR}/src/spz2glb_core.cpp
)

target_link_libraries(spz2glb_core PUBLIC fastgltf)

if(SPZ2GLB_BUILD_WASM)
  target_compile_options(spz2glb_core PRIVATE --use-port=zlib)
  # --use-port=zstd removed: Emscripten 6.0.3 recognizes the port but C++ sources
  # guard zstd with #ifndef __EMSCRIPTEN__, so WASM builds do not need it.
else()
  # zlib: find_package first, fallback to FetchContent with SHA256 (modeled on gatekeeper)
  # Uses INTERFACE library to unify system zlib and FetchContent paths.
  add_library(spz2glb_zlib INTERFACE)
  find_package(ZLIB QUIET)
  if(NOT ZLIB_FOUND)
    message(STATUS "zlib not found, using FetchContent to download v1.3.2")
    include(FetchContent)
    FetchContent_Declare(
      zlib_fetch
      URL "https://github.com/madler/zlib/releases/download/v1.3.2/zlib-1.3.2.tar.gz"
      URL_HASH SHA256=bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16
    )
    FetchContent_MakeAvailable(zlib_fetch)
    # zlib's CMake creates "zlibstatic" (static) or "zlib" (shared) target
    if(TARGET zlibstatic)
      target_link_libraries(spz2glb_zlib INTERFACE zlibstatic)
    elseif(TARGET zlib)
      target_link_libraries(spz2glb_zlib INTERFACE zlib)
    endif()
  else()
    target_link_libraries(spz2glb_zlib INTERFACE ZLIB::ZLIB)
  endif()
  target_link_libraries(spz2glb_core PUBLIC spz2glb_zlib)

  # zstd: find_package first, fallback to FetchContent with SHA256 + SOURCE_SUBDIR
  find_package(zstd QUIET)
  if(NOT zstd_FOUND)
    message(STATUS "zstd not found, using FetchContent to download v1.5.6")
    include(FetchContent)
    FetchContent_Declare(
      zstd_fetch
      URL "https://github.com/facebook/zstd/releases/download/v1.5.6/zstd-1.5.6.tar.gz"
      URL_HASH SHA256=8c29e06cf42aacc1eafc4077ae2ec6c6fcb96a626157e0593d5e82a34fd403c1
      SOURCE_SUBDIR build/cmake
    )
    set(ZSTD_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(zstd_fetch)
    if(NOT TARGET zstd::zstd)
      if(TARGET libzstd_static)
        add_library(zstd::zstd ALIAS libzstd_static)
      endif()
    endif()
    target_link_libraries(spz2glb_core PUBLIC zstd::zstd)
  else()
    if(TARGET zstd::libzstd_static)
      if(NOT TARGET zstd::zstd)
        add_library(zstd::zstd ALIAS zstd::libzstd_static)
      endif()
    elseif(TARGET zstd::libzstd_shared)
      if(NOT TARGET zstd::zstd)
        add_library(zstd::zstd ALIAS zstd::libzstd_shared)
      endif()
    endif()
    target_link_libraries(spz2glb_core PUBLIC zstd::zstd)
  endif()
endif()

target_compile_options(spz2glb_core PRIVATE ${STRICT_WARNINGS})

# ============================================================
# 原生构建 (非 WASM)
# ============================================================

if(NOT SPZ2GLB_BUILD_WASM)
  # 查找 ZLIB
  find_package(ZLIB REQUIRED)

  # CLI 优化标志：Release 级优化，用户即使忘记 -DCMAKE_BUILD_TYPE=Release 也能获得优化构建。
  if(MSVC)
    set(CLI_OPT_FLAGS "/O2")
  else()
    set(CLI_OPT_FLAGS "-O3")
  endif()

  target_compile_options(spz2glb_core PRIVATE ${CLI_OPT_FLAGS})

  # ============================================================
  # spz_verify 构建（命令行验证工具）
  # ============================================================

  add_executable(spz_verify
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spz_verify.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spz_verifier.cpp
  )

  target_link_libraries(spz_verify PRIVATE fastgltf spz2glb_zlib zstd::zstd)

  target_compile_options(spz_verify PRIVATE ${STRICT_WARNINGS} ${CLI_OPT_FLAGS})

  set_target_properties(spz_verify PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dist"
  )

  # ============================================================
  # spz2glb 主程序构建
  # ============================================================

  add_executable(spz2glb
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spz_to_glb.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spz_verifier.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/queue.cpp
  )

  target_link_libraries(spz2glb PRIVATE spz2glb_core fastgltf spz2glb_zlib)

  target_compile_options(spz2glb PRIVATE ${STRICT_WARNINGS} ${CLI_OPT_FLAGS})

  set_target_properties(spz2glb PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dist"
  )

  option(BUILD_TESTS "Build native tests" OFF)
  if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
  endif()
endif()

# ============================================================
# WASM 构建
# ============================================================

if(SPZ2GLB_BUILD_WASM)
  message(STATUS "Building WASM version")

  set(SPZ2GLB_WASM_PROFILE "compat" CACHE STRING "WASM memory profile: compat or perf-lite")
  set_property(CACHE SPZ2GLB_WASM_PROFILE PROPERTY STRINGS compat perf-lite)

  if(SPZ2GLB_WASM_PROFILE STREQUAL "perf-lite")
    set(SPZ2GLB_WASM_INITIAL_MEMORY 134217728)
    set(SPZ2GLB_WASM_ALLOW_MEMORY_GROWTH 0)
    set(SPZ2GLB_WASM_MAXIMUM_MEMORY_OPTION "")
  elseif(SPZ2GLB_WASM_PROFILE STREQUAL "compat")
    set(SPZ2GLB_WASM_INITIAL_MEMORY 67108864)
    set(SPZ2GLB_WASM_ALLOW_MEMORY_GROWTH 1)
    set(SPZ2GLB_WASM_MAXIMUM_MEMORY_OPTION "-sMAXIMUM_MEMORY=1073741824")
  else()
    message(FATAL_ERROR "Unsupported SPZ2GLB_WASM_PROFILE: ${SPZ2GLB_WASM_PROFILE}")
  endif()

  message(STATUS "WASM memory profile: ${SPZ2GLB_WASM_PROFILE}")

  # spz2glb-wasm: 高性能版本（纯 C API，无 LTO 避免导入名压缩问题）
  # ============================================================
  add_executable(spz2glb-wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spz2glb_wasm_c_api.cpp
  )

  target_link_libraries(spz2glb-wasm PRIVATE spz2glb_core fastgltf)

  # Emscripten ZLIB: compile and link with the zlib port so headers stay visible in CI.

  # C++ 编译优化（无 LTO 避免导入名压缩）
  target_compile_options(spz2glb-wasm PRIVATE
    "--use-port=zlib"
    "-O3"
    "-fno-exceptions"
    "-fno-lto"
    "-Wall"
    "-Wextra"
    "-Wpedantic"
    "-Werror"
    "-Wno-unused-parameter"
    "-Wno-unused-variable"
    "-Wno-old-style-cast"
    "-Wno-sign-conversion"
  )

  # 链接优化（使用 JS 胶水代码，禁用 embind）
  target_link_options(spz2glb-wasm PRIVATE
    "-O3"
    "-fno-lto"
    "-sUSE_ZLIB=1"
    "--use-port=zlib"
    # --use-port=zstd removed: Emscripten 5.0.1 does not recognize 'zstd' port name.

    # 内存配置（Phase 6: compat / perf-lite）
    "-sINITIAL_MEMORY=${SPZ2GLB_WASM_INITIAL_MEMORY}"
    "${SPZ2GLB_WASM_MAXIMUM_MEMORY_OPTION}"
    "-sALLOW_MEMORY_GROWTH=${SPZ2GLB_WASM_ALLOW_MEMORY_GROWTH}"
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
    "-sEXPORTED_FUNCTIONS=_spz2glb_alloc,_spz2glb_free,_spz2glb_reserve_input,_spz2glb_get_input_ptr,_spz2glb_convert_reserved_input,_spz2glb_release_output,_spz2glb_convert,_spz2glb_validate_header,_spz2glb_validate_glb_header,_spz2glb_validate_spz_header,_spz2glb_get_version,_spz2glb_get_memory_stats,_spz2glb_reset_memory_stats,_spz2glb_sizeof_size_t,_spz2glb_sizeof_memory_stats,_malloc,_free"
    "-sEXPORTED_RUNTIME_METHODS=ccall,cwrap,getValue,setValue,UTF8ToString,stringToUTF8,lengthBytesUTF8,HEAPU8"
  )

  set_target_properties(spz2glb-wasm PROPERTIES
    OUTPUT_NAME "spz2glb"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dist"
  )

  message(STATUS "Added spz2glb-wasm (high-performance, pure C API, no LTO)")

  # ============================================================
  # spz_verify WASM 构建
  # ============================================================

  add_executable(spz_verify-wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spz_verify.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spz_verifier.cpp
  )

  target_link_libraries(spz_verify-wasm PRIVATE fastgltf)

  # Emscripten ZLIB + ZSTD: compile and link with the port so verifier sources can include zlib.h + zstd.h.

  target_compile_options(spz_verify-wasm PRIVATE
    "--use-port=zlib"
    # --use-port=zstd removed: Emscripten 5.0.1 does not recognize 'zstd' port name.
    "-O3"
    "-fno-exceptions"
    "-Wall"
    "-Wextra"
    "-Wpedantic"
    "-Werror"
    "-Wno-unused-parameter"
    "-Wno-unused-variable"
    "-Wno-old-style-cast"
    "-Wno-sign-conversion"
  )

  target_link_options(spz_verify-wasm PRIVATE
    "-O3"
    "-sUSE_ZLIB=1"
    "--use-port=zlib"
    # --use-port=zstd removed: Emscripten 5.0.1 does not recognize 'zstd' port name.
    "-sINITIAL_MEMORY=${SPZ2GLB_WASM_INITIAL_MEMORY}"
    "${SPZ2GLB_WASM_MAXIMUM_MEMORY_OPTION}"
    "-sALLOW_MEMORY_GROWTH=${SPZ2GLB_WASM_ALLOW_MEMORY_GROWTH}"
    "-sSTACK_SIZE=10485760"
    "-sENVIRONMENT=web"
    "-sEXPORT_ES6=1"
    "-sFILESYSTEM=0"
    "-sASSERTIONS=0"
    "-sNO_EXIT_RUNTIME=1"
    "-sNO_DYNAMIC_EXECUTION=1"
    "-sERROR_ON_UNDEFINED_SYMBOLS=0"
    "-sWASM_BIGINT=1"
    "-sMALLOC=emmalloc"
    "-sSINGLE_FILE=0"
  )

  set_target_properties(spz_verify-wasm PROPERTIES
    OUTPUT_NAME "spz_verify"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dist"
  )

  message(STATUS "Added spz_verify-wasm")
endif()

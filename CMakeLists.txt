# Copyright (c) 2026 Pu Junhan
# SPDX-License-Identifier: MIT
# Project: SPZ-ecosystem
# Repository: https://github.com/spz-ecosystem/spz2glb
#
cmake_minimum_required(VERSION 3.15)
project(spz2glb VERSION 1.0.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
set(CMAKE_INSTALL_RPATH $ORIGIN/../lib)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)

# ============================================================
# WASM 构建配置（复用 Niantic SPZ 官方配置）
# ============================================================
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(SPZ2GLB_BUILD_WASM ON)
  if(NOT DEFINED SPZ2GLB_USE_EMSCRIPTEN_ZLIB)
    set(SPZ2GLB_USE_EMSCRIPTEN_ZLIB ON)
  endif()
  message(STATUS "Emscripten detected: WASM build enabled")
endif()

option(SPZ2GLB_BUILD_WASM "Build WASM version" OFF)

include(GNUInstallDirs)

option(ENABLE_KHR_GAUSSIAN_SPLATTING "Enable KHR_gaussian_splatting extension" ON)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unknown-pragmas)
endif()

# ============================================================
# ZLIB 配置（支持 Emscripten WASM）
# ============================================================
if(SPZ2GLB_USE_EMSCRIPTEN_ZLIB)
  # Emscripten 使用端口版本的 ZLIB
  # Emscripten 会自动配置 zlib 路径，但需要确保 include 目录正确
  message(STATUS "Using Emscripten port of ZLIB")
else()
  find_package(ZLIB QUIET)
  if(NOT ZLIB_FOUND)
    message(FATAL_ERROR "ZLIB not found. Install zlib-dev or use Emscripten for WASM build.")
  endif()
endif()

# 指向根目录的 third_party（fastgltf 源码）
# CMAKE_SOURCE_DIR = 仓库根目录 (spz_ecosystem_simplified)
set(FASTGLTF_DIR "${CMAKE_SOURCE_DIR}/third_party")

if(NOT EXISTS "${FASTGLTF_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "fastgltf not found at: ${FASTGLTF_DIR}")
endif()

add_subdirectory(${FASTGLTF_DIR} fastgltf_build)

set_target_properties(fastgltf PROPERTIES
    COMPILE_WARNING_AS_ERROR OFF
)

if(MSVC)
    target_compile_options(fastgltf PRIVATE /W0)
else()
    target_compile_options(fastgltf PRIVATE -w)
endif()

if(NOT SPZ2GLB_BUILD_WASM)
add_executable(spz2glb 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spz_to_glb.cpp
)

target_compile_options(spz2glb PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
)

target_link_libraries(spz2glb PRIVATE
    fastgltf
)

if(NOT SPZ2GLB_USE_EMSCRIPTEN_ZLIB)
  # 桌面版需要 ZLIB
  target_link_libraries(spz2glb PRIVATE ZLIB::ZLIB)
endif()

if(ENABLE_KHR_GAUSSIAN_SPLATTING)
    target_compile_definitions(spz2glb PRIVATE FASTGLTF_ENABLE_KHR_GAUSSIAN_SPLATTING=1)
    target_compile_definitions(fastgltf PRIVATE FASTGLTF_ENABLE_KHR_GAUSSIAN_SPLATTING=1)
endif()

add_executable(spz_verify
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spz_verify.cpp
)

target_compile_options(spz_verify PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
)

if(NOT SPZ2GLB_USE_EMSCRIPTEN_ZLIB)
  target_link_libraries(spz_verify PRIVATE ZLIB::ZLIB)
endif()

install(TARGETS spz2glb spz_verify
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
endif()

# 可选：启用测试
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    message(STATUS "Building tests...")
    add_subdirectory(tests)
endif()

message(STATUS "")
message(STATUS "========================================")
message(STATUS "spz2glb - SPZ to GLB Converter v${PROJECT_VERSION}")
message(STATUS "========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "KHR Gaussian Splatting: ${ENABLE_KHR_GAUSSIAN_SPLATTING}")
message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "Build WASM: ${SPZ2GLB_BUILD_WASM}")
message(STATUS "========================================")

# ============================================================
# WASM 版本配置（完全复用 Niantic SPZ 官方配置）
# ============================================================
if(SPZ2GLB_BUILD_WASM)
  message(STATUS "Configuring WASM build...")

  # 创建输出目录
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dist")

  # WASM 可执行文件
  add_executable(spz2glb-wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spz_to_glb.cpp
  )

  # 链接 fastgltf
  target_link_libraries(spz2glb-wasm PRIVATE fastgltf)

  # 启用 KHR Gaussian Splatting 扩展
  if(ENABLE_KHR_GAUSSIAN_SPLATTING)
    target_compile_definitions(spz2glb-wasm PRIVATE FASTGLTF_ENABLE_KHR_GAUSSIAN_SPLATTING=1)
  endif()

  # Emscripten ZLIB 配置（编译和链接都需要）
  target_compile_options(spz2glb-wasm PRIVATE --use-port=zlib)
  target_link_options(spz2glb-wasm PRIVATE --use-port=zlib)

  # C++ 编译时优化
  # 注意：-fno-rtti 仍不能使用，因为 Embind 需要 RTTI
  # 但 -fno-exceptions 可以使用了，因为代码已移除 throw
  target_compile_options(spz2glb-wasm PRIVATE
    "-O3"
    "-flto"
    "-fno-exceptions"
  )

  # WASM 链接优化配置
  target_link_options(spz2glb-wasm PRIVATE
    # 优化级别
    "-O3"
    "-flto"

    # Embind
    "--bind"

    # 模块化
    "-sMODULARIZE=1"
    "-sEXPORT_NAME=createSpz2GlbModule"

    # 内存配置（固定内存池，性能优先）
    "-sINITIAL_MEMORY=67108864"     # 64MB 初始
    "-sMAXIMUM_MEMORY=1073741824"   # 1GB 最大
    "-sSTACK_SIZE=10485760"         # 10MB 栈
    # 禁用 ALLOW_MEMORY_GROWTH（固定内存最快）

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

  # 输出配置
  set_target_properties(spz2glb-wasm PROPERTIES
    OUTPUT_NAME "spz2glb"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dist"
  )

  # ============================================================
  # spz2glb-wasm-standalone: 高性能版本（无 Embind）
  # ============================================================
  add_executable(spz2glb-wasm-standalone
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spz2glb_wasm_c_api.cpp
  )

  target_link_libraries(spz2glb-wasm-standalone PRIVATE fastgltf)

  if(ENABLE_KHR_GAUSSIAN_SPLATTING)
    target_compile_definitions(spz2glb-wasm-standalone PRIVATE FASTGLTF_ENABLE_KHR_GAUSSIAN_SPLATTING=1)
  endif()

  # Emscripten ZLIB
  target_compile_options(spz2glb-wasm-standalone PRIVATE --use-port=zlib)

  # C++ 编译优化
  target_compile_options(spz2glb-wasm-standalone PRIVATE
    "-O3"
    "-flto"
    "-fno-exceptions"
  )

  # 完整链接优化（无 Embind）
  target_link_options(spz2glb-wasm-standalone PRIVATE
    "-O3"
    "-flto"
    "-sUSE_ZLIB=1"
    "--use-port=zlib"

    # 内存配置
    "-sINITIAL_MEMORY=67108864"
    "-sMAXIMUM_MEMORY=1073741824"
    "-sSTACK_SIZE=10485760"

    # Web 环境
    "-sENVIRONMENT=web"
    "-sEXPORT_ES6=1"

    # 文件系统
    "-sFILESYSTEM=0"

    # 生产优化
    "-sASSERTIONS=0"
    "-sNO_EXIT_RUNTIME=1"
    "-sNO_DYNAMIC_EXECUTION=1"
    "-sERROR_ON_UNDEFINED_SYMBOLS=0"
    "-sWASM_BIGINT=1"
    "-sMALLOC=emmalloc"

    # 单独文件
    "-sSINGLE_FILE=0"

    # C API 导出
    "-sEXPORTED_FUNCTIONS=_spz2glb_alloc,_spz2glb_free,_spz2glb_convert,_spz2glb_validate_header,_spz2glb_get_version,_spz2glb_get_memory_stats,_spz2glb_reset_memory_stats"
  )

  set_target_properties(spz2glb-wasm-standalone PROPERTIES
    OUTPUT_NAME "spz2glb-standalone"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dist"
  )

  message(STATUS "Added spz2glb-wasm-standalone (high-performance, no Embind)")

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

  # Emscripten ZLIB 配置（编译和链接都需要）
  target_compile_options(spz_verify-wasm PRIVATE --use-port=zlib)

  # C++ 编译优化
  # 注意：-fno-rtti 仍不能使用，因为 Embind 需要 RTTI
  target_compile_options(spz_verify-wasm PRIVATE
    "-O3"
    "-flto"
    "-fno-exceptions"
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
    # 内存配置（固定内存池，性能优先）
    "-sINITIAL_MEMORY=67108864"
    "-sMAXIMUM_MEMORY=1073741824"
    "-sSTACK_SIZE=10485760"
    # 禁用 ALLOW_MEMORY_GROWTH（固定内存最快）
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

  message(STATUS "Added spz_verify-wasm configured")

  # ============================================================
  # 生成 TypeScript 声明文件
  # ============================================================
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

  message(STATUS "TypeScript definitions generated")

  message(STATUS "WASM output directory: ${CMAKE_CURRENT_SOURCE_DIR}/dist")
endif()

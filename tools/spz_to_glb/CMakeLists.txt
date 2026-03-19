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
  # Emscripten 使用端口版本的 ZLIB，无需 find_package
  message(STATUS "Using Emscripten port of ZLIB")
else()
  find_package(ZLIB QUIET)
  if(NOT ZLIB_FOUND)
    message(FATAL_ERROR "ZLIB not found. Install zlib-dev or use Emscripten for WASM build.")
  endif()
endif()

# 指向 tools/third_party/fastgltf（fastgltf 源码）
# CMAKE_SOURCE_DIR = 仓库根目录 (spz_ecosystem_simplified)
set(FASTGLTF_DIR "${CMAKE_SOURCE_DIR}/tools/third_party/fastgltf")

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

  # Emscripten ZLIB 配置
  # 注意：-sUSE_ZLIB=1 在下面的 WASM 链接选项中已设置

  # WASM 链接选项（完全复用 Niantic 官方配置）
  target_link_options(spz2glb-wasm PRIVATE
    "-O3"                              # 优化级别
    "-sUSE_ZLIB=1"                     # 使用 ZLIB
    "--bind"                           # Emscripten bindings
    "-sMODULARIZE=1"                   # 模块化输出
    "-sEXPORT_NAME=createSpz2GlbModule" # 导出函数名
    "-sALLOW_MEMORY_GROWTH=1"          # 允许内存增长
    "-sENVIRONMENT=web"                # Web 环境
    "-sEXPORT_ES6=1"                   # ES6 模块
    "-sSINGLE_FILE=1"                  # 单文件输出（JS+WASM）
    "-sEXPORTED_RUNTIME_METHODS=[\"ccall\",\"cwrap\"]" # 导出运行时方法
  )

  # 输出配置
  set_target_properties(spz2glb-wasm PROPERTIES
    OUTPUT_NAME "spz2glb"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dist"
  )

  message(STATUS "WASM output directory: ${CMAKE_CURRENT_SOURCE_DIR}/dist")
endif()

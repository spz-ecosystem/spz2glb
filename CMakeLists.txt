cmake_minimum_required(VERSION 3.16)
project(spz2glb VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 选项
option(SPZ2GLB_BUILD_WASM "Build WASM version" OFF)
option(SPZ2GLB_USE_EMSCRIPTEN_ZLIB "Use Emscripten ZLIB port" OFF)
option(ENABLE_KHR_GAUSSIAN_SPLATTING "Enable KHR_gaussian_splatting support" ON)

# 添加 fastgltf (文件直接在 third_party 目录下)
add_subdirectory(third_party)

# 添加 fastgltf include 路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/include)

# ============================================================
# spz_verify 构建（命令行验证工具）
# ============================================================

add_executable(spz_verify
  ${CMAKE_CURRENT_SOURCE_DIR}/src/spz_verify.cpp
)

target_link_libraries(spz_verify PRIVATE fastgltf)

if(ENABLE_KHR_GAUSSIAN_SPLATTING)
  target_compile_definitions(spz_verify PRIVATE FASTGLTF_ENABLE_KHR_GAUSSIAN_SPLATTING=1)
endif()

# 严格警告
target_compile_options(spz_verify PRIVATE
  "-Wall"
  "-Wextra"
  "-Wpedantic"
  "-Werror"
  "-Wno-unused-parameter"
)

set_target_properties(spz_verify PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dist"
)

# ============================================================
# spz2glb 主程序构建
# ============================================================

add_executable(spz2glb
  ${CMAKE_CURRENT_SOURCE_DIR}/src/spz_to_glb.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/spz_verifier.cpp
)

target_link_libraries(spz2glb PRIVATE fastgltf z)

if(ENABLE_KHR_GAUSSIAN_SPLATTING)
  target_compile_definitions(spz2glb PRIVATE FASTGLTF_ENABLE_KHR_GAUSSIAN_SPLATTING=1)
endif()

# 严格警告
target_compile_options(spz2glb PRIVATE
  "-Wall"
  "-Wextra"
  "-Wpedantic"
  "-Werror"
  "-Wno-unused-parameter"
)

set_target_properties(spz2glb PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dist"
)

# ============================================================
# WASM 构建
# ============================================================

if(SPZ2GLB_BUILD_WASM)
  message(STATUS "Building WASM version")

  # spz2glb-wasm: 高性能版本（纯 C API，无 LTO 避免导入名压缩问题）
  # ============================================================
  add_executable(spz2glb-wasm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spz2glb_wasm_c_api.cpp
  )

  target_link_libraries(spz2glb-wasm PRIVATE fastgltf)

  if(ENABLE_KHR_GAUSSIAN_SPLATTING)
    target_compile_definitions(spz2glb-wasm PRIVATE FASTGLTF_ENABLE_KHR_GAUSSIAN_SPLATTING=1)
  endif()

  # Emscripten ZLIB
  target_compile_options(spz2glb-wasm PRIVATE --use-port=zlib)

  # C++ 编译优化（无 LTO 避免导入名压缩）
  target_compile_options(spz2glb-wasm PRIVATE
    "-O3"
    "-fno-exceptions"
    "-fno-lto"
    "-Wall"
    "-Wextra"
    "-Wpedantic"
    "-Werror"
    "-Wno-unused-parameter"
    "-Wno-old-style-cast"
    "-Wno-sign-conversion"
  )

  # 链接优化（无 LTO，禁用导入名压缩）
  target_link_options(spz2glb-wasm PRIVATE
    "-O3"
    "-fno-lto"
    "-sUSE_ZLIB=1"
    "--use-port=zlib"

    # 内存配置 - 支持外部导入内存（智能内存管理）
    "-sIMPORTED_MEMORY=1"
    "-sINITIAL_MEMORY=67108864"
    "-sMAXIMUM_MEMORY=1073741824"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sSTACK_SIZE=10485760"

    # Web 环境
    "-sENVIRONMENT=web"
    "-sEXPORT_ES6=1"

    # 文件系统
    "-sFILESYSTEM=0"

    # 生产优化（禁用导入名压缩）
    "-sASSERTIONS=0"
    "-sNO_EXIT_RUNTIME=1"
    "-sNO_DYNAMIC_EXECUTION=1"
    "-sERROR_ON_UNDEFINED_SYMBOLS=0"
    "-sWASM_BIGINT=1"
    "-sMALLOC=emmalloc"
    "-sMINIFY_HTML=0"
    "-sMINIFY_JS=0"

    # 单独文件
    "-sSINGLE_FILE=0"

    # C API 导出
    "-sEXPORTED_FUNCTIONS=_spz2glb_alloc,_spz2glb_free,_spz2glb_convert,_spz2glb_validate_header,_spz2glb_get_version,_spz2glb_get_memory_stats,_spz2glb_reset_memory_stats"
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
  )

  target_link_libraries(spz_verify-wasm PRIVATE fastgltf)

  if(ENABLE_KHR_GAUSSIAN_SPLATTING)
    target_compile_definitions(spz_verify-wasm PRIVATE FASTGLTF_ENABLE_KHR_GAUSSIAN_SPLATTING=1)
  endif()

  target_compile_options(spz_verify-wasm PRIVATE --use-port=zlib)

  target_compile_options(spz_verify-wasm PRIVATE
    "-O3"
    "-fno-exceptions"
    "-Wall"
    "-Wextra"
    "-Wpedantic"
    "-Werror"
    "-Wno-unused-parameter"
    "-Wno-old-style-cast"
    "-Wno-sign-conversion"
  )

  target_link_options(spz_verify-wasm PRIVATE
    "-O3"
    "-sUSE_ZLIB=1"
    "--use-port=zlib"
    "-sINITIAL_MEMORY=67108864"
    "-sMAXIMUM_MEMORY=1073741824"
    "-sALLOW_MEMORY_GROWTH=1"
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

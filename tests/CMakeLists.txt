cmake_minimum_required(VERSION 3.15)
project(spz2glb_tests)

# 启用测试
enable_testing()

# 查找 spz2glb 和 spz_verify 可执行文件
find_program(SPZ2GLB spz2glb PATHS "${CMAKE_BINARY_DIR}/.." "${CMAKE_BINARY_DIR}")
find_program(SPZ_VERIFY spz_verify PATHS "${CMAKE_BINARY_DIR}/.." "${CMAKE_BINARY_DIR}")

if(NOT SPZ2GLB)
    message(FATAL_ERROR "spz2glb executable not found")
endif()

if(NOT SPZ_VERIFY)
    message(FATAL_ERROR "spz_verify executable not found")
endif()

message(STATUS "Found spz2glb: ${SPZ2GLB}")
message(STATUS "Found spz_verify: ${SPZ_VERIFY}")

# 测试数据目录
set(TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data")

# 临时输出目录
set(TEST_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/output")
file(MAKE_DIRECTORY ${TEST_OUTPUT_DIR})

# 辅助函数：添加转换测试
function(add_spz_conversion_test test_name input_spz)
    set(output_glb "${TEST_OUTPUT_DIR}/${test_name}.glb")
    
    add_test(
        NAME "${test_name}_convert"
        COMMAND ${SPZ2GLB} "${input_spz}" "${output_glb}"
        WORKING_DIRECTORY ${TEST_OUTPUT_DIR}
    )
    
    set_tests_properties("${test_name}_convert" PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[SUCCESS\\] GLB exported"
    )
endfunction()

# 辅助函数：添加验证测试
function(add_spz_verify_test test_name input_spz output_glb)
    add_test(
        NAME "${test_name}_verify"
        COMMAND ${SPZ_VERIFY} all "${input_spz}" "${output_glb}"
        WORKING_DIRECTORY ${TEST_OUTPUT_DIR}
    )
    
    set_tests_properties("${test_name}_verify" PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[SUCCESS\\] All 3 layers validation passed"
        DEPENDS "${test_name}_convert"
    )
endfunction()

# 辅助函数：添加完整测试（转换 + 验证）
function(add_spz_full_test test_name input_spz)
    set(output_glb "${TEST_OUTPUT_DIR}/${test_name}.glb")
    
    # 转换测试
    add_spz_conversion_test(${test_name} ${input_spz})
    
    # 验证测试
    add_spz_verify_test(${test_name} ${input_spz} ${output_glb})
endfunction()

# 示例测试（如果有测试数据）
if(EXISTS "${TEST_DATA_DIR}/triangle.spz")
    add_spz_full_test("triangle" "${TEST_DATA_DIR}/triangle.spz")
endif()

if(EXISTS "${TEST_DATA_DIR}/cube.spz")
    add_spz_full_test("cube" "${TEST_DATA_DIR}/cube.spz")
endif()

# Layer 单独测试
if(EXISTS "${TEST_DATA_DIR}/test.spz")
    set(test_glb "${TEST_OUTPUT_DIR}/test.glb")
    
    # Layer 1: GLB 结构验证
    add_test(
        NAME "layer1_structure"
        COMMAND ${SPZ_VERIFY} layer1 "${test_glb}"
        WORKING_DIRECTORY ${TEST_OUTPUT_DIR}
    )
    set_tests_properties("layer1_structure" PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[PASS\\] Layer 1 validation passed"
        DEPENDS "test_convert"
    )
    
    # Layer 2: 二进制无损验证
    add_test(
        NAME "layer2_lossless"
        COMMAND ${SPZ_VERIFY} layer2 "${TEST_DATA_DIR}/test.spz" "${test_glb}"
        WORKING_DIRECTORY ${TEST_OUTPUT_DIR}
    )
    set_tests_properties("layer2_lossless" PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[PASS\\] Layer 2 validation passed"
        DEPENDS "test_convert"
    )
    
    # Layer 3: 解码一致性验证
    add_test(
        NAME "layer3_consistency"
        COMMAND ${SPZ_VERIFY} layer3 "${TEST_DATA_DIR}/test.spz" "${test_glb}"
        WORKING_DIRECTORY ${TEST_OUTPUT_DIR}
    )
    set_tests_properties("layer3_consistency" PROPERTIES
        PASS_REGULAR_EXPRESSION "\\[PASS\\] Layer 3 validation passed"
        DEPENDS "test_convert"
    )
endif()

# 打印测试信息
message(STATUS "")
message(STATUS "=== Test Configuration ===")
message(STATUS "Test data directory: ${TEST_DATA_DIR}")
message(STATUS "Test output directory: ${TEST_OUTPUT_DIR}")
message(STATUS "")

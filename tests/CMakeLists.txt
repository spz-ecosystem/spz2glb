cmake_minimum_required(VERSION 3.15)
project(spz2glb_tests)

# 启用测试
enable_testing()

if(TARGET spz2glb AND TARGET spz_verify)
    set(SPZ2GLB $<TARGET_FILE:spz2glb>)
    set(SPZ_VERIFY $<TARGET_FILE:spz_verify>)
    message(STATUS "Integration tests enabled for native targets")
else()
    message(STATUS "Native targets unavailable - integration conversion tests skipped")
endif()

# 测试数据目录
set(TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data")

# 临时输出目录
set(TEST_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/output")
file(MAKE_DIRECTORY ${TEST_OUTPUT_DIR})

if(TARGET spz2glb AND TARGET spz_verify)
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

    function(add_spz_full_test test_name input_spz)
        set(output_glb "${TEST_OUTPUT_DIR}/${test_name}.glb")
        add_spz_conversion_test(${test_name} ${input_spz})
        add_spz_verify_test(${test_name} ${input_spz} ${output_glb})
    endfunction()

    if(EXISTS "${TEST_DATA_DIR}/triangle.spz")
        add_spz_full_test("triangle" "${TEST_DATA_DIR}/triangle.spz")
    endif()

    if(EXISTS "${TEST_DATA_DIR}/cube.spz")
        add_spz_full_test("cube" "${TEST_DATA_DIR}/cube.spz")
    endif()

    if(EXISTS "${TEST_DATA_DIR}/test.spz")
        set(test_glb "${TEST_OUTPUT_DIR}/test.glb")

        add_test(
            NAME "layer1_structure"
            COMMAND ${SPZ_VERIFY} layer1 "${test_glb}"
            WORKING_DIRECTORY ${TEST_OUTPUT_DIR}
        )
        set_tests_properties("layer1_structure" PROPERTIES
            PASS_REGULAR_EXPRESSION "\\[PASS\\] Layer 1 validation passed"
            DEPENDS "test_convert"
        )

        add_test(
            NAME "layer2_lossless"
            COMMAND ${SPZ_VERIFY} layer2 "${TEST_DATA_DIR}/test.spz" "${test_glb}"
            WORKING_DIRECTORY ${TEST_OUTPUT_DIR}
        )
        set_tests_properties("layer2_lossless" PROPERTIES
            PASS_REGULAR_EXPRESSION "\\[PASS\\] Layer 2 validation passed"
            DEPENDS "test_convert"
        )

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
endif()

add_executable(wasm_c_api_test
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm_c_api_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/spz2glb_wasm_c_api.cpp
)

target_link_libraries(wasm_c_api_test PRIVATE fastgltf ZLIB::ZLIB)

if(ENABLE_KHR_GAUSSIAN_SPLATTING)
    target_compile_definitions(wasm_c_api_test PRIVATE FASTGLTF_ENABLE_KHR_GAUSSIAN_SPLATTING=1)
endif()

add_test(
    NAME "wasm_c_api_test"
    COMMAND wasm_c_api_test
)

# 打印测试信息
message(STATUS "")
message(STATUS "=== Test Configuration ===")
message(STATUS "Test data directory: ${TEST_DATA_DIR}")
message(STATUS "Test output directory: ${TEST_OUTPUT_DIR}")
message(STATUS "")

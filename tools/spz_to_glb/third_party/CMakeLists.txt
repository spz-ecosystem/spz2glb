# Check if tests/examples directories exist and are non-empty, skip if empty
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    # tests dir exists with CMakeLists - would be processed
else()
    message(STATUS "tests directory is empty or missing - skipping")
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt)
    # examples dir exists with CMakeLists - would be processed
else()
    message(STATUS "examples directory is empty or missing - skipping")
endif()

cmake_minimum_required(VERSION 3.13)

project(fastgltf VERSION 0.9.0 LANGUAGES C CXX)

message(STATUS "=== fastgltf CMakeLists.txt ===")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")

option(FASTGLTF_USE_CUSTOM_SMALLVECTOR "Uses a custom SmallVector type optimised for small arrays" OFF)
option(FASTGLTF_ENABLE_TESTS "Enables test targets for fastgltf" OFF)
option(FASTGLTF_ENABLE_EXAMPLES "Enables example targets for fastgltf" OFF)
option(FASTGLTF_ENABLE_DOCS "Enables documentation targets" OFF)
option(FASTGLTF_ENABLE_GLTF_RS "Enables gltf-rs benchmarks" OFF)
option(FASTGLTF_ENABLE_ASSIMP "Enables assimp benchmarks" OFF)
option(FASTGLTF_ENABLE_DEPRECATED_EXT "Enables deprecated extensions" OFF)
option(FASTGLTF_DISABLE_CUSTOM_MEMORY_POOL "Disables custom memory pool" OFF)
option(FASTGLTF_USE_64BIT_FLOAT "Use 64-bit floats" OFF)
option(FASTGLTF_COMPILE_AS_CPP20 "Compile as C++20" OFF)
option(FASTGLTF_ENABLE_CPP_MODULES "Enable C++ modules" OFF)
option(FASTGLTF_USE_STD_MODULE "Use std module" OFF)
option(FASTGLTF_ENABLE_KHR_IMPLICIT_SHAPES "KHR_implicit_shapes" OFF)
option(FASTGLTF_ENABLE_KHR_PHYSICS_RIGID_BODIES "KHR_physics_rigid_bodies" OFF)
option(FASTGLTF_ENABLE_KHR_GAUSSIAN_SPLATTING "KHR_gaussian_splatting" OFF)

if(FASTGLTF_COMPILE_AS_CPP20)
    set(FASTGLTF_COMPILE_TARGET cxx_std_20)
else()
    set(FASTGLTF_COMPILE_TARGET cxx_std_17)
endif()

message(STATUS "Including cmake/add_source_directory.cmake...")
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/add_source_directory.cmake)

message(STATUS "Including cmake/compilers.cmake...")
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/compilers.cmake)

set(SIMDJSON_DL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/simdjson")
message(STATUS "SIMDJSON_DL_DIR: ${SIMDJSON_DL_DIR}")

message(STATUS "Including cmake/dependencies.cmake...")
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependencies.cmake)

set(FASTGLTF_HEADERS
        "include/fastgltf/base64.hpp"
        "include/fastgltf/core.hpp"
        "include/fastgltf/dxmath_element_traits.hpp"
        "include/fastgltf/glm_element_traits.hpp"
        "include/fastgltf/tools.hpp"
        "include/fastgltf/types.hpp"
        "include/fastgltf/util.hpp"
        "include/fastgltf/math.hpp"
)

set(FASTGLTF_SOURCES
        "src/fastgltf.cpp"
        "src/base64.cpp"
        "src/io.cpp"
)

message(STATUS "Creating fastgltf static library...")
add_library(fastgltf STATIC)
target_sources(fastgltf PRIVATE ${FASTGLTF_SOURCES})
target_include_directories(fastgltf PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>" "$<INSTALL_INTERFACE:include>")
target_compile_features(fastgltf PUBLIC ${FASTGLTF_COMPILE_TARGET})

message(STATUS "Adding source directories...")
fastgltf_add_source_directory(TARGET fastgltf FOLDER src)
fastgltf_add_source_directory(TARGET fastgltf FOLDER deps/simdjson)
fastgltf_add_source_directory(TARGET fastgltf FOLDER include)

target_compile_definitions(fastgltf PUBLIC SIMDJSON_VERSION=\"4.3.1\")
target_compile_definitions(fastgltf PRIVATE SIMDJSON_USING_LIBRARY)

set_target_properties(fastgltf PROPERTIES
        CXX_EXTENSIONS OFF
        CXX_STANDARD 17
        PUBLIC_HEADER "${FASTGLTF_HEADERS}"
        OUTPUT_NAME "fastgltf"
        VERSION "${PROJECT_VERSION}"
        SOVERSION ${PROJECT_VERSION_MAJOR}
)

message(STATUS "Applying compile options...")
fastgltf_add_compile_options(fastgltf)

message(STATUS "=== fastgltf configured successfully ===")

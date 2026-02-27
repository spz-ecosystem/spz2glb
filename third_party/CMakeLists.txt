cmake_minimum_required(VERSION 3.13)

if ("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" VERSION_GREATER_EQUAL "3.24")
    cmake_policy(SET CMP0135 NEW)
endif ()

project(fastgltf VERSION 0.9.0 LANGUAGES C CXX)

option(FASTGLTF_USE_CUSTOM_SMALLVECTOR "Uses a custom SmallVector type optimised for small arrays" OFF)
option(FASTGLTF_ENABLE_TESTS "Enables test targets for fastgltf" OFF)
option(FASTGLTF_ENABLE_EXAMPLES "Enables example targets for fastgltf" OFF)
option(FASTGLTF_ENABLE_DOCS "Enables the configuration of targets that build/generate documentation" OFF)
option(FASTGLTF_ENABLE_GLTF_RS "Enables the benchmark usage of gltf-rs" OFF)
option(FASTGLTF_ENABLE_ASSIMP "Enables the benchmark usage of assimp" OFF)
option(FASTGLTF_ENABLE_DEPRECATED_EXT "Enables support for deprecated extensions" OFF)
option(FASTGLTF_DISABLE_CUSTOM_MEMORY_POOL "Disables the memory allocation algorithm based on polymorphic resources" OFF)
option(FASTGLTF_USE_64BIT_FLOAT "Default to 64-bit double precision floats for everything" OFF)
option(FASTGLTF_COMPILE_AS_CPP20 "Have the library compile as C++20" OFF)
option(FASTGLTF_ENABLE_CPP_MODULES "Enables the fastgltf::module target, which uses C++20 modules" OFF)
option(FASTGLTF_USE_STD_MODULE "Use the std module when compiling using C++ modules" OFF)
option(FASTGLTF_ENABLE_KHR_IMPLICIT_SHAPES "Enable support for the experimental KHR_implicit_shapes extension" OFF)
option(FASTGLTF_ENABLE_KHR_PHYSICS_RIGID_BODIES "Enable support for the experimental KHR_physics_rigid_bodies extension" OFF)

if (FASTGLTF_COMPILE_AS_CPP20)
    set(FASTGLTF_COMPILE_TARGET cxx_std_20)
else ()
    set(FASTGLTF_COMPILE_TARGET cxx_std_17)
endif ()

if (FASTGLTF_ENABLE_KHR_PHYSICS_RIGID_BODIES)
    set(FASTGLTF_ENABLE_KHR_IMPLICIT_SHAPES ON)
endif ()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/add_source_directory.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/compilers.cmake)

set(SIMDJSON_DL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/simdjson")
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

add_library(fastgltf STATIC)
target_sources(fastgltf PRIVATE ${FASTGLTF_SOURCES})
target_sources(fastgltf PUBLIC FILE_SET HEADERS SET BASE_HEADERS ${FASTGLTF_HEADERS})
target_include_directories(fastgltf PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>" "$<INSTALL_INTERFACE:include>")
target_compile_features(fastgltf PUBLIC ${FASTGLTF_COMPILE_TARGET})

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

install(TARGETS fastgltf
        EXPORT fastgltfTargets
        FILE_SET HEADERS BASE_HEADERS
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(EXPORT fastgltfTargets
        FILE fastgltfConfig.cmake
        NAMESPACE fastgltf::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/fastgltf"
)

configure_file("cmake/fastgltfConfigVersion.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/fastgltfConfigVersion.cmake"
        @ONLY
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fastgltfConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/fastgltf"
)

fastgltf_add_compile_options(fastgltf)

include(CMakePackageConfigHelpers)

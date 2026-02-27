cmake_minimum_required(VERSION 3.15)
project(spz2glb VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
set(CMAKE_INSTALL_RPATH $ORIGIN/../lib)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)

include(GNUInstallDirs)

option(ENABLE_KHR_GAUSSIAN_SPLATTING "Enable KHR_gaussian_splatting extension" ON)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unknown-pragmas)
endif()

find_package(ZLIB REQUIRED)

set(FASTGLTF_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

if(NOT EXISTS "${FASTGLTF_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "fastgltf not found at: ${FASTGLTF_DIR}")
endif()

add_subdirectory(${FASTGLTF_DIR} fastgltf_build)

add_executable(spz2glb 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spz_to_glb.cpp
)

target_link_libraries(spz2glb PRIVATE
    ZLIB::ZLIB
    fastgltf
)

if(ENABLE_KHR_GAUSSIAN_SPLATTING)
    target_compile_definitions(spz2glb PRIVATE FASTGLTF_ENABLE_KHR_GAUSSIAN_SPLATTING=1)
    target_compile_definitions(fastgltf PRIVATE FASTGLTF_ENABLE_KHR_GAUSSIAN_SPLATTING=1)
endif()

add_executable(spz_verify
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spz_verify.cpp
)

target_link_libraries(spz_verify PRIVATE
    ZLIB::ZLIB
)

install(TARGETS spz2glb spz_verify
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

message(STATUS "")
message(STATUS "========================================")
message(STATUS "spz2glb - SPZ to GLB Converter v${PROJECT_VERSION}")
message(STATUS "========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "KHR Gaussian Splatting: ${ENABLE_KHR_GAUSSIAN_SPLATTING}")
message(STATUS "========================================")

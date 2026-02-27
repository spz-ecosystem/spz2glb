# Embedded simdjson - no download needed
# simdjson sources are included via add_source_directory in CMakeLists.txt

message(STATUS "Using embedded simdjson v4.3.1")

set(SIMDJSON_SOURCES
    ${SIMDJSON_DL_DIR}/simdjson.cpp
)

set(SIMDJSON_HEADERS
    ${SIMDJSON_DL_DIR}/simdjson.h
)

# Just verify files exist - no download needed
if(NOT EXISTS ${SIMDJSON_DL_DIR}/simdjson.cpp)
    message(FATAL_ERROR "simdjson source not found at: ${SIMDJSON_DL_DIR}/simdjson.cpp")
endif()

if(NOT EXISTS ${SIMDJSON_DL_DIR}/simdjson.h)
    message(FATAL_ERROR "simdjson header not found at: ${SIMDJSON_DL_DIR}/simdjson.h")
endif()

message(STATUS "simdjson sources found - using embedded version")

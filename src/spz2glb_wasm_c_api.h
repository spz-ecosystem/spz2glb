// Copyright (c) 2026 Pu Junhan
// SPDX-License-Identifier: MIT
// Project: SPZ-ecosystem
// Repository: https://github.com/spz-ecosystem/spz2glb
//
// High-performance WebAssembly C API
// No Embind - pure C exports for maximum performance
//
// CODE QUALITY:
// - Strict compile flags: -Wall -Wextra -Werror -pedantic
// - All pointers checked for NULL before use
// - No implicit conversions
// - No raw new/delete - use alloc/free pairs only

#ifndef SPZ2GLB_WASM_C_API_H
#define SPZ2GLB_WASM_C_API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Ensure struct has known size
#if defined(__cplusplus)
static_assert(sizeof(uint32_t) == 4, "uint32_t must be 4 bytes");
static_assert(sizeof(uint64_t) == 8, "uint64_t must be 8 bytes");
static_assert(sizeof(size_t) >= 4, "size_t must be at least 32 bits");
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(uint32_t) == 4, "uint32_t must be 4 bytes");
_Static_assert(sizeof(uint64_t) == 8, "uint64_t must be 8 bytes");
_Static_assert(sizeof(size_t) >= 4, "size_t must be at least 32 bits");
#endif

#ifdef __cplusplus
extern "C" {
#endif

// LEAK_DETECTION: Track all allocations
// Set to 1 to enable debug allocation tracking
#ifndef SPZ2GLB_DEBUG_ALLOC
#define SPZ2GLB_DEBUG_ALLOC 0
#endif

// NULL_CHECK: Enforce NULL checks
#ifndef SPZ2GLB_NULL_CHECKS
#define SPZ2GLB_NULL_CHECKS 1
#endif

/**
 * Memory management
 * IMPORTANT: Always use spz2glb_alloc/spz2glb_free pairs
 * Never use raw new/delete or malloc/free
 */
uint8_t* spz2glb_alloc(size_t size);
void spz2glb_free(uint8_t* ptr);

/**
 * Core conversion: SPZ -> GLB
 * @param spzData Input SPZ data (must be non-NULL)
 * @param spzSize Size of SPZ data in bytes (must be > 0)
 * @param outSize Output parameter: size of returned GLB data
 * @return Pointer to GLB data, or NULL on error
 *         Caller MUST call spz2glb_free() on success
 */
uint8_t* spz2glb_convert(const uint8_t* spzData, size_t spzSize, size_t* outSize);

/**
 * Validate GLB header
 * @param data Data to validate (must be non-NULL, size >= 12)
 * @param size Size of data (must be >= 12)
 * @return true if valid GLB header, false otherwise
 */
bool spz2glb_validate_header(const uint8_t* data, size_t size);

/**
 * Get library version
 */
void spz2glb_get_version(int* major, int* minor, int* patch);

/**
 * Memory statistics (for debugging)
 */
typedef struct {
    size_t peak_usage_bytes;
    size_t current_usage_bytes;
    size_t total_allocations;
    size_t total_frees;
    size_t failed_allocations;
} Spz2GlbMemoryStats;

void spz2glb_get_memory_stats(Spz2GlbMemoryStats* stats);
void spz2glb_reset_memory_stats(void);

/**
 * Assert macro that always compiles
 */
#if defined(__cplusplus) && __cplusplus >= 201703L
#include <cassert>
#define SPZ2GLB_ASSERT(expr, msg) assert((expr) && (msg))
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#include <assert.h>
#define SPZ2GLB_ASSERT(expr, msg) _Static_assert((expr), msg)
#else
#define SPZ2GLB_ASSERT(expr, msg) ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif // SPZ2GLB_WASM_C_API_H

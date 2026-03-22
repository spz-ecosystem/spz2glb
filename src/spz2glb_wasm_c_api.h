// Copyright (c) 2026 Pu Junhan
// SPDX-License-Identifier: MIT
// Project: SPZ-ecosystem
// Repository: https://github.com/spz-ecosystem/spz2glb
//
// High-performance WebAssembly C API
// No Embind - pure C exports for maximum performance

#ifndef SPZ2GLB_WASM_C_API_H
#define SPZ2GLB_WASM_C_API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Memory management
uint8_t* spz2glb_alloc(size_t size);
void spz2glb_free(uint8_t* ptr);

// Core conversion: SPZ -> GLB
// Returns: pointer to GLB data, sets outSize
// Caller must call spz2glb_free on returned pointer
uint8_t* spz2glb_convert(const uint8_t* spzData, size_t spzSize, size_t* outSize);

// Validation
bool spz2glb_validate_header(const uint8_t* data, size_t size);

// Utility
void spz2glb_get_version(int* major, int* minor, int* patch);

// Memory stats
typedef struct {
    size_t peak_usage;
    size_t current_usage;
    size_t total_allocations;
    size_t total_frees;
} Spz2GlbMemoryStats;

void spz2glb_get_memory_stats(Spz2GlbMemoryStats* stats);
void spz2glb_reset_memory_stats(void);

#ifdef __cplusplus
}
#endif

#endif // SPZ2GLB_WASM_C_API_H

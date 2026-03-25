// Copyright (c) 2026 Pu Junhan
// SPDX-License-Identifier: MIT
// Project: SPZ-ecosystem
// Repository: https://github.com/spz-ecosystem/spz2glb

#ifndef SPZ2GLB_WASM_C_API_H
#define SPZ2GLB_WASM_C_API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

#ifndef SPZ2GLB_DEBUG_ALLOC
#define SPZ2GLB_DEBUG_ALLOC 0
#endif

typedef struct {
    size_t peak_usage_bytes;
    size_t current_usage_bytes;
    size_t total_allocations;
    size_t total_frees;
    size_t failed_allocations;
} Spz2GlbMemoryStats;

uint8_t* spz2glb_alloc(size_t size);
void spz2glb_free(uint8_t* ptr);

size_t spz2glb_reserve_input(size_t size);
uint8_t* spz2glb_get_input_ptr(void);
int spz2glb_convert_reserved_input(size_t size, uint8_t** outPtr, size_t* outSize);
void spz2glb_release_output(uint8_t* ptr);

uint8_t* spz2glb_convert(const uint8_t* spzData, size_t spzSize, size_t* outSize);
bool spz2glb_validate_header(const uint8_t* data, size_t size);
void spz2glb_get_version(int* major, int* minor, int* patch);
void spz2glb_get_memory_stats(Spz2GlbMemoryStats* stats);
void spz2glb_reset_memory_stats(void);

#ifdef __cplusplus
}
#endif

#endif  // SPZ2GLB_WASM_C_API_H


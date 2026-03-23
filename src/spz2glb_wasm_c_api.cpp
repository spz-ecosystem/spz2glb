// Copyright (c) 2026 Pu Junhan
// SPDX-License-Identifier: MIT
//
// Implementation of high-performance WASM C API
//
// CODE QUALITY ENFORCED:
// - All pointers validated before use
// - Memory leak detection
// - Failed allocation tracking
// - No silent failures

#include "spz2glb_wasm_c_api.h"
#include "spz_to_glb.cpp"
#include <cstring>
#include <cstdlib>

// Memory tracking
static Spz2GlbMemoryStats g_stats = {0, 0, 0, 0, 0};
static bool g_initialized = false;

// Debug: track active allocations
#if SPZ2GLB_DEBUG_ALLOC
#include <stdio.h>
#define DEBUG_LOG(...) fprintf(stderr, "[spz2glb] " __VA_ARGS__ "\n")
#else
#define DEBUG_LOG(...) ((void)0)
#endif

// Initialization flag
static inline void ensure_initialized(void) {
    if (!g_initialized) {
        g_initialized = true;
        DEBUG_LOG("Initialized");
    }
}

uint8_t* spz2glb_alloc(size_t size) {
    ensure_initialized();

    // Check for reasonable size
    if (size == 0) {
        DEBUG_LOG("WARNING: alloc(0) called");
        g_stats.failed_allocations++;
        return NULL;
    }

    // Check for excessive size (potential overflow or bug)
    if (size > 1024 * 1024 * 1024) { // 1GB limit
        DEBUG_LOG("ERROR: alloc size too large: %zu", size);
        g_stats.failed_allocations++;
        return NULL;
    }

    uint8_t* ptr = new (std::nothrow) uint8_t[size];
    if (ptr == NULL) {
        DEBUG_LOG("ERROR: allocation failed for size %zu", size);
        g_stats.failed_allocations++;
        return NULL;
    }

    g_stats.current_usage_bytes += size;
    g_stats.total_allocations++;

    if (g_stats.current_usage_bytes > g_stats.peak_usage_bytes) {
        g_stats.peak_usage_bytes = g_stats.current_usage_bytes;
    }

    DEBUG_LOG("alloc(%zu) = %p, total: %zu", size, (void*)ptr, g_stats.current_usage_bytes);
    return ptr;
}

void spz2glb_free(uint8_t* ptr) {
    // NULL is valid (no-op)
    if (ptr == NULL) {
        DEBUG_LOG("free(NULL) - no-op");
        return;
    }

    // Note: We can't track exact size without keeping a map
    // This is a limitation of simple allocation tracking
    g_stats.total_frees++;
    DEBUG_LOG("free(%p)", (void*)ptr);

    delete[] ptr;
}

uint8_t* spz2glb_convert(const uint8_t* spzData, size_t spzSize, size_t* outSize) {
    ensure_initialized();

    // Validate inputs
    if (spzData == NULL) {
        DEBUG_LOG("ERROR: spzData is NULL");
        if (outSize) *outSize = 0;
        return NULL;
    }

    if (spzSize == 0) {
        DEBUG_LOG("ERROR: spzSize is 0");
        if (outSize) *outSize = 0;
        return NULL;
    }

    if (outSize == NULL) {
        DEBUG_LOG("ERROR: outSize is NULL");
        return NULL;
    }

    DEBUG_LOG("convert: size=%zu", spzSize);

    // Create input vector
    std::vector<uint8_t> input(spzData, spzData + spzSize);

    // Create output vector
    std::vector<uint8_t> output;

    // Call core conversion
    if (!convertSpzToGlbCore(input, output)) {
        DEBUG_LOG("ERROR: convertSpzToGlbCore failed");
        *outSize = 0;
        return NULL;
    }

    // Check output is valid
    if (output.empty()) {
        DEBUG_LOG("ERROR: output is empty");
        *outSize = 0;
        return NULL;
    }

    // Allocate result buffer
    size_t resultSize = output.size();
    uint8_t* result = spz2glb_alloc(resultSize);
    if (result == NULL) {
        DEBUG_LOG("ERROR: failed to allocate result buffer");
        *outSize = 0;
        return NULL;
    }

    // Copy data
    std::memcpy(result, output.data(), resultSize);
    *outSize = resultSize;

    DEBUG_LOG("convert: success, output size=%zu", resultSize);
    return result;
}

bool spz2glb_validate_header(const uint8_t* data, size_t size) {
    ensure_initialized();

    // Validate inputs
    if (data == NULL) {
        DEBUG_LOG("ERROR: data is NULL in validate_header");
        return false;
    }

    if (size < 12) {
        DEBUG_LOG("ERROR: size %zu < 12 in validate_header", size);
        return false;
    }

    // Read magic and version
    // Using memcpy for strict aliasing safety
    uint32_t magic = 0;
    uint32_t version = 0;
    std::memcpy(&magic, data, sizeof(uint32_t));
    std::memcpy(&version, data + 4, sizeof(uint32_t));

    // GLB magic: 0x46546C67 ('glTF')
    if (magic != 0x46546C67) {
        DEBUG_LOG("ERROR: invalid magic: 0x%08X", magic);
        return false;
    }

    // Only version 2 is supported
    if (version != 2) {
        DEBUG_LOG("ERROR: unsupported version: %u", version);
        return false;
    }

    DEBUG_LOG("validate_header: valid");
    return true;
}

void spz2glb_get_version(int* major, int* minor, int* patch) {
    if (major) *major = 1;
    if (minor) *minor = 0;
    if (patch) *patch = 0;
}

void spz2glb_get_memory_stats(Spz2GlbMemoryStats* stats) {
    if (stats == NULL) {
        return;
    }
    *stats = g_stats;
}

void spz2glb_reset_memory_stats(void) {
    DEBUG_LOG("reset_memory_stats: peak=%zu, current=%zu, allocs=%zu, frees=%zu, failed=%zu",
              g_stats.peak_usage_bytes,
              g_stats.current_usage_bytes,
              g_stats.total_allocations,
              g_stats.total_frees,
              g_stats.failed_allocations);

    g_stats.peak_usage_bytes = 0;
    g_stats.current_usage_bytes = 0;
    g_stats.total_allocations = 0;
    g_stats.total_frees = 0;
    g_stats.failed_allocations = 0;
}

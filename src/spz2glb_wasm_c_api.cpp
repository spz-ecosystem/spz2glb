// Copyright (c) 2026 Pu Junhan
// SPDX-License-Identifier: MIT
//
// Implementation of high-performance WASM C API

#include "spz2glb_wasm_c_api.h"
#include "spz_to_glb.cpp"

static Spz2GlbMemoryStats g_stats = {0, 0, 0, 0};

uint8_t* spz2glb_alloc(size_t size) {
    uint8_t* ptr = new uint8_t[size];
    g_stats.current_usage += size;
    g_stats.total_allocations++;
    if (g_stats.current_usage > g_stats.peak_usage) {
        g_stats.peak_usage = g_stats.current_usage;
    }
    return ptr;
}

void spz2glb_free(uint8_t* ptr) {
    if (ptr) {
        delete[] ptr;
        g_stats.total_frees++;
    }
}

uint8_t* spz2glb_convert(const uint8_t* spzData, size_t spzSize, size_t* outSize) {
    std::vector<uint8_t> input(spzData, spzData + spzSize);
    std::vector<uint8_t> output;

    if (!convertSpzToGlbCore(input, output)) {
        *outSize = 0;
        return nullptr;
    }

    uint8_t* result = spz2glb_alloc(output.size());
    std::memcpy(result, output.data(), output.size());
    *outSize = output.size();
    return result;
}

bool spz2glb_validate_header(const uint8_t* data, size_t size) {
    if (size < 12) return false;

    uint32_t magic = *reinterpret_cast<const uint32_t*>(data);
    uint32_t version = *reinterpret_cast<const uint32_t*>(data + 4);

    return magic == 0x46546C67 && version == 2;
}

void spz2glb_get_version(int* major, int* minor, int* patch) {
    *major = 1;
    *minor = 0;
    *patch = 0;
}

void spz2glb_get_memory_stats(Spz2GlbMemoryStats* stats) {
    *stats = g_stats;
}

void spz2glb_reset_memory_stats(void) {
    g_stats = {0, 0, 0, 0};
}

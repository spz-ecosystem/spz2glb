// Copyright (c) 2026 Pu Junhan
// SPDX-License-Identifier: MIT

#define SPZ2GLB_DISABLE_EMBIND 1
#define SPZ2GLB_NO_CLI_MAIN 1

#include "spz2glb_wasm_c_api.h"
#include "spz_to_glb.cpp"

#include <cstdio>
#include <cstring>
#include <new>
#include <vector>

namespace {

constexpr size_t kMaxAllocationBytes = 1024ULL * 1024ULL * 1024ULL;
constexpr uint32_t kOwnedOutputPoolSize = 8;

struct OwnedOutput {
    std::vector<std::byte> bytes;
    size_t tracked_bytes = 0;
    bool from_hot_pool = false;
    OwnedOutput* next = nullptr;
};

using OwnedOutputPool = spz2glb::HotObjectPool<sizeof(OwnedOutput), kOwnedOutputPoolSize>;

uint8_t* g_reserved_input = nullptr;
size_t g_reserved_input_capacity = 0;
OwnedOutput* g_owned_outputs = nullptr;
OwnedOutputPool g_owned_output_pool;

#if SPZ2GLB_DEBUG_ALLOC
#define DEBUG_LOG(...) do { std::fprintf(stderr, "[spz2glb] " __VA_ARGS__); std::fprintf(stderr, "\n"); } while (0)
#else
#define DEBUG_LOG(...) do { } while (0)
#endif

void track_alloc(size_t size) {
    spz2glb::trackExternalAlloc(size);
}

void track_free(size_t size) {
    spz2glb::trackExternalFree(size);
}

uint8_t* alloc_tracked(size_t size) {
    if (size == 0 || size > kMaxAllocationBytes) {
        spz2glb::trackFailedAllocation();
        return nullptr;
    }

    const size_t rawSize = size + sizeof(size_t);
    auto* raw = new (std::nothrow) uint8_t[rawSize];
    if (raw == nullptr) {
        spz2glb::trackFailedAllocation();
        return nullptr;
    }

    std::memcpy(raw, &rawSize, sizeof(size_t));
    auto* userPtr = raw + sizeof(size_t);
    track_alloc(rawSize);

    DEBUG_LOG("alloc(%zu) -> %p", size, static_cast<void*>(userPtr));
    return userPtr;
}

size_t get_allocation_size(const uint8_t* ptr) {
    if (ptr == nullptr) {
        return 0;
    }

    size_t size = 0;
    std::memcpy(&size, ptr - sizeof(size_t), sizeof(size_t));
    return size;
}

void free_tracked(uint8_t* ptr) {
    if (ptr == nullptr) {
        return;
    }

    const size_t size = get_allocation_size(ptr);
    track_free(size);

    auto* raw = ptr - sizeof(size_t);
    DEBUG_LOG("free(%p)", static_cast<void*>(ptr));
    delete[] raw;
}

void clear_reserved_input() {
    if (g_reserved_input != nullptr) {
        free_tracked(g_reserved_input);
        g_reserved_input = nullptr;
        g_reserved_input_capacity = 0;
    }
}

OwnedOutput* create_owned_output() {
    if (void* slot = g_owned_output_pool.alloc()) {
        auto* owner = new (slot) OwnedOutput();
        owner->from_hot_pool = true;
        return owner;
    }

    auto* owner = new (std::nothrow) OwnedOutput();
    if (owner == nullptr) {
        spz2glb::trackFailedAllocation();
        return nullptr;
    }

    return owner;
}

void destroy_owned_output(OwnedOutput* owner) {
    if (owner == nullptr) {
        return;
    }

    if (owner->from_hot_pool) {
        owner->~OwnedOutput();
        g_owned_output_pool.dealloc(owner);
        return;
    }

    delete owner;
}

uint8_t* register_output(std::vector<std::byte>&& output, size_t* outSize) {
    if (outSize == nullptr || output.empty() || output.size() > kMaxAllocationBytes) {
        spz2glb::trackFailedAllocation();
        return nullptr;
    }

    OwnedOutput* owner = create_owned_output();
    if (owner == nullptr) {
        return nullptr;
    }

    owner->bytes = std::move(output);
    auto* resultPtr = reinterpret_cast<uint8_t*>(owner->bytes.data());
    if (resultPtr == nullptr) {
        spz2glb::trackFailedAllocation();
        destroy_owned_output(owner);
        return nullptr;
    }

    *outSize = owner->bytes.size();
    owner->tracked_bytes = owner->bytes.capacity();
    if (!owner->from_hot_pool) {
        owner->tracked_bytes += sizeof(OwnedOutput);
    }
    track_alloc(owner->tracked_bytes);

    owner->next = g_owned_outputs;
    g_owned_outputs = owner;
    return resultPtr;
}

bool release_owned_output(uint8_t* ptr) {
    OwnedOutput** current = &g_owned_outputs;
    while (*current != nullptr) {
        auto* owner = *current;
        if (reinterpret_cast<uint8_t*>(owner->bytes.data()) == ptr) {
            *current = owner->next;
            track_free(owner->tracked_bytes);
            destroy_owned_output(owner);
            return true;
        }
        current = &owner->next;
    }

    return false;
}

uint8_t* convert_input(const uint8_t* spzData, size_t spzSize, size_t* outSize) {
    if (outSize == nullptr) {
        return nullptr;
    }

    *outSize = 0;
    if (spzData == nullptr || spzSize == 0) {
        return nullptr;
    }

    std::vector<std::byte> output;
    if (!convertSpzToGlbCore(spzData, spzSize, output) || output.empty()) {
        return nullptr;
    }

    return register_output(std::move(output), outSize);
}

}  // namespace

uint8_t* spz2glb_alloc(size_t size) {
    return alloc_tracked(size);
}

void spz2glb_free(uint8_t* ptr) {
    free_tracked(ptr);
}

size_t spz2glb_reserve_input(size_t size) {
    clear_reserved_input();
    if (size == 0) {
        return 0;
    }

    g_reserved_input = alloc_tracked(size);
    if (g_reserved_input == nullptr) {
        return 0;
    }

    g_reserved_input_capacity = size;
    return g_reserved_input_capacity;
}

uint8_t* spz2glb_get_input_ptr(void) {
    return g_reserved_input;
}

int spz2glb_convert_reserved_input(size_t size, uint8_t** outPtr, size_t* outSize) {
    if (outPtr == nullptr || outSize == nullptr) {
        return 0;
    }

    *outPtr = nullptr;
    *outSize = 0;

    if (g_reserved_input == nullptr || size == 0 || size > g_reserved_input_capacity) {
        return 0;
    }

    *outPtr = convert_input(g_reserved_input, size, outSize);
    return *outPtr != nullptr ? 1 : 0;
}

void spz2glb_release_output(uint8_t* ptr) {
    release_owned_output(ptr);
}

uint8_t* spz2glb_convert(const uint8_t* spzData, size_t spzSize, size_t* outSize) {
    return convert_input(spzData, spzSize, outSize);
}

bool spz2glb_validate_glb_header(const uint8_t* data, size_t size) {
    if (data == nullptr || size < 12) {
        return false;
    }

    uint32_t magic = 0;
    uint32_t version = 0;
    std::memcpy(&magic, data, sizeof(uint32_t));
    std::memcpy(&version, data + 4, sizeof(uint32_t));

    return magic == 0x46546C67 && version == 2;
}

bool spz2glb_validate_spz_header(const uint8_t* data, size_t size) {
    SpzHeader header{};
    return peekSpzHeader(data, size, header);
}

bool spz2glb_validate_header(const uint8_t* data, size_t size) {
    return spz2glb_validate_glb_header(data, size);
}

void spz2glb_get_version(int* major, int* minor, int* patch) {
    if (major != nullptr) {
        *major = 1;
    }
    if (minor != nullptr) {
        *minor = 0;
    }
    if (patch != nullptr) {
        *patch = 0;
    }
}

void spz2glb_get_memory_stats(Spz2GlbMemoryStats* stats) {
    if (stats == nullptr) {
        return;
    }

    const spz2glb::MemoryStats coreStats = spz2glb::getMemoryStats();
    stats->peak_usage_bytes = coreStats.peak_usage;
    stats->current_usage_bytes = coreStats.current_usage;
    stats->total_allocations = coreStats.total_allocations;
    stats->total_frees = coreStats.total_frees;
    stats->failed_allocations = coreStats.failed_allocations;
    stats->hot_available = coreStats.hot_available;
    stats->work_used = coreStats.work_used;
    stats->work_capacity = coreStats.work_capacity;
    stats->work_peak = coreStats.work_peak;
}

void spz2glb_reset_memory_stats(void) {
    spz2glb::resetMemoryStats();
}

size_t spz2glb_sizeof_size_t(void) {
    return sizeof(size_t);
}

size_t spz2glb_sizeof_memory_stats(void) {
    return sizeof(Spz2GlbMemoryStats);
}


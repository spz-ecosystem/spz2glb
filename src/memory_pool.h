#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <new>

namespace spz2glb {

struct MemoryStats {
    size_t peak_usage = 0;
    size_t current_usage = 0;
    size_t total_allocations = 0;
    size_t total_frees = 0;
    size_t failed_allocations = 0;
    size_t hot_available = 0;
    size_t work_used = 0;
    size_t work_capacity = 0;
    size_t work_peak = 0;
};

class MemoryTracker {
public:
    void trackExternalAlloc(size_t size) {
        if (size == 0) {
            return;
        }
        stats_.current_usage += size;
        stats_.peak_usage = std::max(stats_.peak_usage, stats_.current_usage);
        ++stats_.total_allocations;
    }

    void trackExternalFree(size_t size) {
        if (size == 0) {
            return;
        }
        stats_.current_usage = stats_.current_usage > size ? stats_.current_usage - size : 0;
        ++stats_.total_frees;
    }

    void trackFailedAllocation() {
        ++stats_.failed_allocations;
    }

    void trackWorkReserve(size_t size) {
        stats_.work_capacity = size;
        trackExternalAlloc(size);
    }

    void trackWorkRelease(size_t reservedSize, size_t usedSize) {
        stats_.work_used = stats_.work_used > usedSize ? stats_.work_used - usedSize : 0;
        stats_.work_capacity = stats_.work_capacity > reservedSize ? stats_.work_capacity - reservedSize : 0;
        trackExternalFree(reservedSize);
    }

    void trackWorkUsage(size_t usedSize) {
        stats_.work_used = usedSize;
        stats_.work_peak = std::max(stats_.work_peak, usedSize);
        ++stats_.total_allocations;
    }

    void resetWorkUsage() {
        stats_.work_used = 0;
    }

    void setHotAvailable(size_t available) {
        stats_.hot_available = available;
    }

    void resetCountersPreserveLiveAllocations() {
        stats_.peak_usage = stats_.current_usage;
        stats_.total_allocations = 0;
        stats_.total_frees = 0;
        stats_.failed_allocations = 0;
        stats_.work_peak = std::max(stats_.work_peak, stats_.work_used);
    }

    MemoryStats snapshot() const {
        return stats_;
    }

private:
    MemoryStats stats_{};
};

inline MemoryTracker& memoryTracker() {
    static MemoryTracker tracker;
    return tracker;
}

inline void trackExternalAlloc(size_t size) {
    memoryTracker().trackExternalAlloc(size);
}

inline void trackExternalFree(size_t size) {
    memoryTracker().trackExternalFree(size);
}

inline void trackFailedAllocation() {
    memoryTracker().trackFailedAllocation();
}

inline void resetMemoryStats() {
    memoryTracker().resetCountersPreserveLiveAllocations();
}

class BumpAllocator {
public:
    BumpAllocator() = default;

    explicit BumpAllocator(size_t size) {
        init(size);
    }

    BumpAllocator(const BumpAllocator&) = delete;
    BumpAllocator& operator=(const BumpAllocator&) = delete;

    ~BumpAllocator() {
        release();
    }

    bool init(size_t size) {
        release();
        if (size == 0) {
            return true;
        }

        pool_ = new (std::nothrow) std::byte[size];
        if (pool_ == nullptr) {
            trackFailedAllocation();
            return false;
        }

        capacity_ = size;
        current_offset_ = 0;
        peak_usage_ = 0;
        allocations_ = 0;
        memoryTracker().trackWorkReserve(size);
        return true;
    }

    void release() {
        if (pool_ == nullptr) {
            return;
        }

        memoryTracker().trackWorkRelease(capacity_, current_offset_);
        delete[] pool_;
        pool_ = nullptr;
        capacity_ = 0;
        current_offset_ = 0;
        peak_usage_ = 0;
        allocations_ = 0;
    }

    void* alloc(size_t size, size_t alignment = alignof(std::max_align_t)) {
        if (pool_ == nullptr || size == 0) {
            return nullptr;
        }

        const size_t aligned_offset = alignUp(current_offset_, alignment);
        if (aligned_offset > capacity_ || capacity_ - aligned_offset < size) {
            trackFailedAllocation();
            return nullptr;
        }

        void* ptr = pool_ + aligned_offset;
        current_offset_ = aligned_offset + size;
        peak_usage_ = std::max(peak_usage_, current_offset_);
        ++allocations_;
        memoryTracker().trackWorkUsage(current_offset_);
        return ptr;
    }

    void reset() {
        current_offset_ = 0;
        memoryTracker().resetWorkUsage();
    }

    size_t used() const { return current_offset_; }
    size_t peak_usage() const { return peak_usage_; }
    size_t remaining() const { return capacity_ > current_offset_ ? capacity_ - current_offset_ : 0; }
    size_t allocations() const { return allocations_; }
    size_t capacity() const { return capacity_; }

private:
    static size_t alignUp(size_t value, size_t alignment) {
        const size_t mask = alignment - 1;
        return (value + mask) & ~mask;
    }

    std::byte* pool_ = nullptr;
    size_t capacity_ = 0;
    size_t current_offset_ = 0;
    size_t peak_usage_ = 0;
    size_t allocations_ = 0;
};

template<size_t ObjectSize, uint32_t PoolSize>
class HotObjectPool {
    static_assert(PoolSize > 0, "PoolSize must be > 0");
    static_assert(ObjectSize >= sizeof(void*), "ObjectSize must be >= sizeof(void*)");

    struct FreeNode {
        FreeNode* next;
    };

public:
    HotObjectPool() : freeList_(nullptr), allocations_(0) {
        for (uint32_t i = 0; i < PoolSize; ++i) {
            auto* node = reinterpret_cast<FreeNode*>(pool_ + i * ObjectSize);
            node->next = freeList_;
            freeList_ = node;
        }
        memoryTracker().setHotAvailable(PoolSize);
    }

    void* alloc() {
        if (freeList_ == nullptr) {
            return nullptr;
        }
        auto* node = freeList_;
        freeList_ = freeList_->next;
        ++allocations_;
        memoryTracker().setHotAvailable(available());
        return node;
    }

    void dealloc(void* ptr) {
        auto* node = reinterpret_cast<FreeNode*>(ptr);
        node->next = freeList_;
        freeList_ = node;
        memoryTracker().setHotAvailable(available());
    }

    uint32_t available() const {
        uint32_t count = 0;
        for (auto* node = freeList_; node != nullptr; node = node->next) {
            ++count;
        }
        return count;
    }

    uint32_t allocations() const { return allocations_; }

private:
    FreeNode* freeList_;
    alignas(std::max_align_t) std::byte pool_[ObjectSize * PoolSize];
    uint32_t allocations_;
};

inline MemoryStats getMemoryStats() {
    return memoryTracker().snapshot();
}

}  // namespace spz2glb

#endif

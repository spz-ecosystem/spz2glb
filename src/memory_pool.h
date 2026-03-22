#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <cstddef>
#include <cstdint>
#include <cstring>

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#endif

namespace spz2glb {

struct MemoryStats {
    size_t peak_usage;
    size_t current_usage;
    size_t hot_allocations;
    size_t hot_available;
    size_t work_used;
    size_t work_remaining;
};

class BumpAllocator {
private:
    char* pool_;
    char* current_;
    char* end_;
    size_t peak_usage_;
    size_t allocations_;

public:
    BumpAllocator() : pool_(nullptr), current_(nullptr), end_(nullptr), peak_usage_(0), allocations_(0) {}

    bool init(size_t size) {
        pool_ = new char[size];
        if (!pool_) return false;
        current_ = pool_;
        end_ = pool_ + size;
        peak_usage_ = 0;
        allocations_ = 0;
        return true;
    }

    ~BumpAllocator() {
        delete[] pool_;
    }

    void* alloc(size_t size) {
        if (current_ + size > end_) {
            return nullptr;
        }
        void* ptr = current_;
        current_ += size;
        size_t used = current_ - pool_;
        if (used > peak_usage_) {
            peak_usage_ = used;
        }
        allocations_++;
        return ptr;
    }

    void reset() {
        current_ = pool_;
    }

    size_t used() const { return current_ - pool_; }
    size_t peak_usage() const { return peak_usage_; }
    size_t remaining() const { return end_ - current_; }
    size_t allocations() const { return allocations_; }
};

template<size_t ObjectSize, uint32_t PoolSize>
class HotObjectPool {
    static_assert(PoolSize > 0, "PoolSize must be > 0");
    static_assert(ObjectSize >= sizeof(void*), "ObjectSize must be >= sizeof(void*)");

    struct FreeNode {
        FreeNode* next;
    };

    FreeNode* freeList_;
    char pool_[ObjectSize * PoolSize];
    uint32_t allocations_;

public:
    HotObjectPool() : freeList_(nullptr), allocations_(0) {
        for (uint32_t i = 0; i < PoolSize; ++i) {
            auto* node = reinterpret_cast<FreeNode*>(pool_ + i * ObjectSize);
            node->next = freeList_;
            freeList_ = node;
        }
    }

    void* alloc() {
        if (!freeList_) return nullptr;
        auto* node = freeList_;
        freeList_ = freeList_->next;
        allocations_++;
        return node;
    }

    void dealloc(void* ptr) {
        auto* node = reinterpret_cast<FreeNode*>(ptr);
        node->next = freeList_;
        freeList_ = node;
    }

    uint32_t available() const {
        uint32_t count = 0;
        for (auto* n = freeList_; n; n = n->next) ++count;
        return count;
    }

    uint32_t allocations() const { return allocations_; }
};

#ifdef __EMSCRIPTEN__

class ZeroCopyBuffer {
private:
    uint8_t* data_;
    size_t size_;
    bool ownsData_;

public:
    ZeroCopyBuffer() : data_(nullptr), size_(0), ownsData_(false) {}

    ~ZeroCopyBuffer() {
        if (ownsData_ && data_) {
            delete[] data_;
        }
    }

    bool createFromJsBuffer(emscripten::val buffer) {
        if (ownsData_ && data_) {
            delete[] data_;
        }
        ownsData_ = false;

        data_ = static_cast<uint8_t*>(buffer.as_raw_mem());
        size_ = buffer["byteLength"].as<size_t>();
        return data_ != nullptr;
    }

    uint8_t* data() { return data_; }
    size_t size() const { return size_; }
};

#endif

inline MemoryStats getMemoryStats() {
    return {0, 0, 0, 0, 0, 0};
}

}

#endif

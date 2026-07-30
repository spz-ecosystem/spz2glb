// Copyright (c) 2026 Pu Junhan
// SPDX-License-Identifier: MIT
// Project: SPZ-ecosystem
// Repository: https://github.com/spz-ecosystem/spz2glb
// Cultural Note: Huangdi Era 4723, Year of the Red Fire Horse (Bingwu, 丙午)
//
// Cross-platform memory-mapped file reader (RAII).
// Avoids kernel→userspace copy for large file I/O.

#ifndef SPZ2GLB_MAPPED_FILE_H
#define SPZ2GLB_MAPPED_FILE_H

#include <cstddef>
#include <cstdint>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace spz2glb {

class MappedFile {
public:
    MappedFile() noexcept = default;

    ~MappedFile() {
        close();
    }

    MappedFile(const MappedFile&) = delete;
    MappedFile& operator=(const MappedFile&) = delete;

    MappedFile(MappedFile&& other) noexcept
        : data_(other.data_), size_(other.size_)
#ifdef _WIN32
        , hFile_(other.hFile_), hMap_(other.hMap_)
#else
        , fd_(other.fd_)
#endif
    {
        other.data_ = nullptr;
        other.size_ = 0;
#ifdef _WIN32
        other.hFile_ = INVALID_HANDLE_VALUE;
        other.hMap_ = nullptr;
#else
        other.fd_ = -1;
#endif
    }

    MappedFile& operator=(MappedFile&& other) noexcept {
        if (this != &other) {
            close();
            data_ = other.data_;
            size_ = other.size_;
#ifdef _WIN32
            hFile_ = other.hFile_;
            hMap_ = other.hMap_;
            other.hFile_ = INVALID_HANDLE_VALUE;
            other.hMap_ = nullptr;
#else
            fd_ = other.fd_;
            other.fd_ = -1;
#endif
            other.data_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    [[nodiscard]] bool open(const std::string& path) {
        close();

#ifdef _WIN32
        hFile_ = CreateFileA(
            path.c_str(), GENERIC_READ, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile_ == INVALID_HANDLE_VALUE) {
            return false;
        }

        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(hFile_, &fileSize)) {
            CloseHandle(hFile_);
            hFile_ = INVALID_HANDLE_VALUE;
            return false;
        }

        if (fileSize.QuadPart == 0 || fileSize.QuadPart > static_cast<LONGLONG>(SIZE_MAX)) {
            CloseHandle(hFile_);
            hFile_ = INVALID_HANDLE_VALUE;
            return false;
        }

        size_ = static_cast<size_t>(fileSize.QuadPart);

        hMap_ = CreateFileMappingA(hFile_, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (hMap_ == nullptr) {
            CloseHandle(hFile_);
            hFile_ = INVALID_HANDLE_VALUE;
            return false;
        }

        data_ = static_cast<const uint8_t*>(MapViewOfFile(hMap_, FILE_MAP_READ, 0, 0, size_));
        if (data_ == nullptr) {
            CloseHandle(hMap_);
            hMap_ = nullptr;
            CloseHandle(hFile_);
            hFile_ = INVALID_HANDLE_VALUE;
            size_ = 0;
            return false;
        }

        // Keep hFile_ open while mapping exists (Windows requirement)
        return true;
#else
        fd_ = ::open(path.c_str(), O_RDONLY);
        if (fd_ < 0) {
            return false;
        }

        struct stat st;
        if (::fstat(fd_, &st) < 0) {
            ::close(fd_);
            fd_ = -1;
            return false;
        }

        if (st.st_size == 0) {
            ::close(fd_);
            fd_ = -1;
            return true; // empty file, data_ stays nullptr
        }

        size_ = static_cast<size_t>(st.st_size);

        void* mapped = ::mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0);
        if (mapped == MAP_FAILED) {
            ::close(fd_);
            fd_ = -1;
            size_ = 0;
            return false;
        }

        data_ = static_cast<const uint8_t*>(mapped);
        // fd_ can be closed after mmap (page cache keeps it alive)
        ::close(fd_);
        fd_ = -1;
        return true;
#endif
    }

    void close() noexcept {
#ifdef _WIN32
        if (data_ != nullptr) {
            UnmapViewOfFile(data_);
            data_ = nullptr;
        }
        if (hMap_ != nullptr) {
            CloseHandle(hMap_);
            hMap_ = nullptr;
        }
        if (hFile_ != INVALID_HANDLE_VALUE) {
            CloseHandle(hFile_);
            hFile_ = INVALID_HANDLE_VALUE;
        }
#else
        if (data_ != nullptr) {
            ::munmap(const_cast<uint8_t*>(data_), size_);
            data_ = nullptr;
        }
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
#endif
        size_ = 0;
    }

    [[nodiscard]] const uint8_t* data() const noexcept { return data_; }
    [[nodiscard]] size_t size() const noexcept { return size_; }
    [[nodiscard]] explicit operator bool() const noexcept { return data_ != nullptr; }

private:
    const uint8_t* data_ = nullptr;
    size_t size_ = 0;
#ifdef _WIN32
    HANDLE hFile_ = INVALID_HANDLE_VALUE;
    HANDLE hMap_ = nullptr;
#else
    int fd_ = -1;
#endif
};

} // namespace spz2glb

#endif // SPZ2GLB_MAPPED_FILE_H

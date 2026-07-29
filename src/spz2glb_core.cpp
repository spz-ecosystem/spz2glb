// Copyright (c) 2026 Pu Junhan
// SPDX-License-Identifier: MIT
// Project: SPZ-ecosystem
// Repository: https://github.com/spz-ecosystem/spz2glb
// Cultural Note: Huangdi Era 4723, Year of the Red Fire Horse (Bingwu, 丙午)

#include "spz2glb_core.h"

#include <cstring>
#include <iostream>
#include <memory>

#include <zlib.h>

#ifndef __EMSCRIPTEN__
#include <zstd.h>
#endif

#include "memory_pool.h"

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>

namespace {

struct SpzHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t numPoints;
    uint8_t shDegree;
    uint8_t fractionalBits;
    uint8_t flags;
    uint8_t reserved;
};

// SPZ v4 扩展头部: v3 的 16B + 额外 16B = 32B
struct SpzV4Header {
    // v3 公共部分 (16B)
    uint32_t magic;
    uint32_t version;
    uint32_t numPoints;
    uint8_t shDegree;
    uint8_t fractionalBits;
    uint8_t flags;
    uint8_t reserved;
    // v4 扩展部分 (16B)
    uint32_t pointCount;
    uint8_t shBandCount;
    uint8_t chunkConfig;
    uint16_t attributeOffsets;
    uint32_t tocByteOffset;
    uint32_t reserved2; // padding: 28B → 32B
};

static_assert(sizeof(SpzHeader) == 16, "SpzHeader must be 16 bytes");
static_assert(sizeof(SpzV4Header) == 32, "SpzV4Header must be 32 bytes");

constexpr uint32_t kSpzMagic = 0x5053474e;
constexpr uint32_t kZstdMagic = 0xFD2FB528;
constexpr size_t kConversionWorkArenaBytes = 64 * 1024;

fastgltf::span<const std::byte> asByteSpan(const uint8_t* data, size_t size) {
    return fastgltf::span<const std::byte>(reinterpret_cast<const std::byte*>(data), size);
}

bool parseSpzHeader(const uint8_t* data, size_t size, SpzHeader& header) {
    if (data == nullptr || size < sizeof(SpzHeader)) {
        return false;
    }

    std::memcpy(&header, data, sizeof(SpzHeader));
    if (header.magic != kSpzMagic) {
        std::cerr << "[ERROR] Invalid SPZ magic number: 0x" << std::hex << header.magic << std::dec << std::endl;
        return false;
    }

    return true;
}

bool isGzipData(const uint8_t* data, size_t size) {
    return data != nullptr && size >= 2 && data[0] == 0x1f && data[1] == 0x8b;
}

bool isZstdData(const uint8_t* data, size_t size) {
    if (data == nullptr || size < 4) return false;
    uint32_t magic = 0;
    std::memcpy(&magic, data, sizeof(magic));
    return magic == kZstdMagic;
}

// ZSTD v4: 前 4B 是 ZSTD magic, bytes[4..35] 是 32B SPZ v4 header 明文
bool peekSpzHeaderFromZstd(const uint8_t* data, size_t size, SpzHeader& header) {
    if (data == nullptr || size < sizeof(SpzV4Header) + 4) {
        std::cerr << "[ERROR] v4 ZSTD SPZ too small for header" << std::endl;
        return false;
    }

    SpzV4Header v4Header{};
    // v4 header 从 offset 4 开始 (ZSTD magic 之后)
    std::memcpy(&v4Header, data + 4, sizeof(SpzV4Header));

    if (v4Header.magic != kSpzMagic) {
        std::cerr << "[ERROR] Invalid SPZ magic in v4 ZSTD header" << std::endl;
        return false;
    }

    // 拷贝公共字段到 SpzHeader
    header.magic = v4Header.magic;
    header.version = v4Header.version;
    header.numPoints = v4Header.numPoints;
    header.shDegree = v4Header.shDegree;
    header.fractionalBits = v4Header.fractionalBits;
    header.flags = v4Header.flags;
    header.reserved = v4Header.reserved;

    std::cout << "[INFO] v4 ZSTD SPZ: pointCount=" << v4Header.pointCount
              << ", shBandCount=" << static_cast<int>(v4Header.shBandCount)
              << ", chunkConfig=0x" << std::hex << static_cast<int>(v4Header.chunkConfig) << std::dec
              << ", attributeOffsets=0x" << std::hex << v4Header.attributeOffsets << std::dec
              << ", tocByteOffset=" << v4Header.tocByteOffset << std::endl;

    return true;
}

bool peekSpzHeaderFromGzip(const uint8_t* compressedData, size_t compressedSize, SpzHeader& header,
        spz2glb::BumpAllocator* workArena = nullptr) {
    spz2glb::BumpAllocator localArena;
    if (workArena == nullptr) {
        if (!localArena.init(sizeof(SpzHeader))) {
            std::cerr << "[ERROR] Failed to initialize SPZ header work arena" << std::endl;
            return false;
        }
        workArena = &localArena;
    } else {
        workArena->reset();
    }

    auto* headerBytes = static_cast<uint8_t*>(workArena->alloc(sizeof(SpzHeader), alignof(SpzHeader)));
    if (headerBytes == nullptr) {
        std::cerr << "[ERROR] Failed to allocate SPZ header scratch buffer" << std::endl;
        return false;
    }

    z_stream strm = {};
    strm.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(compressedData));
    strm.avail_in = static_cast<uInt>(compressedSize);
    strm.next_out = reinterpret_cast<Bytef*>(headerBytes);
    strm.avail_out = static_cast<uInt>(sizeof(SpzHeader));

    if (inflateInit2(&strm, 16 + MAX_WBITS) != Z_OK) {
        std::cerr << "[ERROR] Failed to initialize zlib decompression" << std::endl;
        return false;
    }

    int ret = Z_OK;
    while (ret == Z_OK && strm.total_out < sizeof(SpzHeader)) {
        ret = inflate(&strm, Z_NO_FLUSH);
    }

    const bool hasHeader = strm.total_out >= sizeof(SpzHeader);
    inflateEnd(&strm);
    if (!hasHeader || (ret != Z_OK && ret != Z_STREAM_END)) {
        std::cerr << "[ERROR] Failed to peek SPZ header from gzip stream" << std::endl;
        return false;
    }

    return parseSpzHeader(headerBytes, sizeof(SpzHeader), header);
}

bool peekSpzHeader(const uint8_t* data, size_t size, SpzHeader& header,
        spz2glb::BumpAllocator* workArena = nullptr) {
    if (data == nullptr || size == 0) {
        std::cerr << "[ERROR] SPZ input is empty" << std::endl;
        return false;
    }

    if (isGzipData(data, size)) {
        return peekSpzHeaderFromGzip(data, size, header, workArena);
    }

    if (isZstdData(data, size)) {
        return peekSpzHeaderFromZstd(data, size, header);
    }

    return parseSpzHeader(data, size, header);
}

fastgltf::Asset createGltfAsset(fastgltf::span<const std::byte> spzData,
                                 const SpzHeader& header,
                                 const std::string& compressionType = "gzip",
                                 uint32_t coordinateSystem = 0) {
    fastgltf::Asset asset;

    asset.extensionsUsed.emplace_back("KHR_gaussian_splatting");
    asset.extensionsUsed.emplace_back("KHR_gaussian_splatting_compression_spz_2");
    asset.extensionsRequired.emplace_back("KHR_gaussian_splatting");
    asset.extensionsRequired.emplace_back("KHR_gaussian_splatting_compression_spz_2");

    asset.assetInfo.emplace();
    asset.assetInfo->gltfVersion = "2.0";
    asset.assetInfo->copyright = "";
    asset.assetInfo->generator = "spz_to_glb_fastgltf";

    const size_t spzSize = spzData.size();

    fastgltf::Buffer buffer;
    fastgltf::sources::ByteView viewData;
    viewData.bytes = spzData;
    viewData.mimeType = fastgltf::MimeType::None;

    buffer.data = viewData;
    buffer.byteLength = spzSize;
    asset.buffers.emplace_back(std::move(buffer));

    fastgltf::BufferView spzBufferView;
    spzBufferView.bufferIndex = 0;
    spzBufferView.byteOffset = 0;
    spzBufferView.byteLength = spzSize;
    asset.bufferViews.emplace_back(std::move(spzBufferView));

    fastgltf::Primitive primitive;
    primitive.type = fastgltf::PrimitiveType::Points;

    auto gaussianSplat = std::make_unique<fastgltf::GaussianSplatExtension>();
    auto spzCompression = std::make_unique<fastgltf::GaussianSplatSpzCompression>();
    spzCompression->bufferView = 0;
    spzCompression->spzVersion = header.version;
    spzCompression->compression = compressionType;
    spzCompression->coordinateSystem = coordinateSystem;
    gaussianSplat->spzCompression = std::move(spzCompression);
    primitive.gaussianSplat = std::move(gaussianSplat);

    fastgltf::Mesh mesh;
    mesh.primitives.emplace_back(std::move(primitive));
    asset.meshes.emplace_back(std::move(mesh));

    fastgltf::Node node;
    node.meshIndex = 0;
    asset.nodes.emplace_back(std::move(node));

    fastgltf::Scene scene;
    scene.nodeIndices.emplace_back(0);
    asset.scenes.emplace_back(std::move(scene));
    asset.defaultScene = 0;

    return asset;
}

}  // namespace

// ILV 记录类型常量
constexpr uint32_t kIlvTypeCoordSys = 0xADBE0003;

// 从 v4 ZSTD 数据的 header zone 中扫描 ILV 记录, 提取 coordinateSystem
// header zone 范围: bytes[4 + sizeof(SpzV4Header) .. 4 + sizeof(SpzV4Header) + extendedBytes)
// 其中 extendedBytes = v4Header.tocByteOffset (从 v4 header 末尾开始的偏移)
uint32_t readHeaderZoneCoordSys(const uint8_t* data, size_t size, const SpzV4Header& v4Header) {
    const size_t headerZoneStart = 4 + sizeof(SpzV4Header); // after ZSTD magic + v4 header
    if (v4Header.tocByteOffset == 0 || headerZoneStart >= size) {
        return 0; // 无扩展区域
    }

    const size_t headerZoneEnd = headerZoneStart + v4Header.tocByteOffset;
    if (headerZoneEnd > size) {
        std::cerr << "[WARN] v4 header zone extends beyond file size" << std::endl;
        return 0;
    }

    size_t pos = headerZoneStart;
    while (pos + 8 <= headerZoneEnd) { // 每个 ILV 记录至少 8B (type + length)
        uint32_t type = 0;
        uint32_t length = 0;
        std::memcpy(&type, data + pos, sizeof(type));
        std::memcpy(&length, data + pos + 4, sizeof(length));

        if (type == kIlvTypeCoordSys && length >= 4) {
            if (pos + 8 + 4 <= headerZoneEnd) {
                uint32_t coordSys = 0;
                std::memcpy(&coordSys, data + pos + 8, sizeof(coordSys));
                std::cout << "[INFO] Coordinate system extension found: " << coordSys << std::endl;
                return coordSys;
            }
        }

        pos += 8 + length; // 跳到下一个 ILV 记录
    }

    return 0;
}

bool validateSpzHeaderCore(const uint8_t* data, size_t size) {
    SpzHeader header{};
    return peekSpzHeader(data, size, header);
}

bool convertSpzToGlbCore(const uint8_t* spzData, size_t spzSize, std::vector<std::byte>& glbData) {
    spz2glb::BumpAllocator workArena;
    if (!workArena.init(kConversionWorkArenaBytes)) {
        std::cerr << "[ERROR] Failed to initialize conversion work arena" << std::endl;
        return false;
    }

    SpzHeader header;
    if (!peekSpzHeader(spzData, spzSize, header, &workArena)) {
        std::cerr << "[ERROR] Failed to parse SPZ header" << std::endl;
        return false;
    }

    // 检测压缩类型
    std::string compressionType = "none";
    if (isGzipData(spzData, spzSize)) {
        compressionType = "gzip";
    } else if (isZstdData(spzData, spzSize)) {
        compressionType = "zstd";
    }

    // v4 ZSTD: 解析 ILV header zone 提取 coordinateSystem
    uint32_t coordinateSystem = 0;
    if (compressionType == "zstd" && spzSize >= 4 + sizeof(SpzV4Header)) {
        SpzV4Header v4Hdr{};
        std::memcpy(&v4Hdr, spzData + 4, sizeof(SpzV4Header));
        if (v4Hdr.magic == kSpzMagic && v4Hdr.version >= 4) {
            coordinateSystem = readHeaderZoneCoordSys(spzData, spzSize, v4Hdr);
        }
    }

    std::cout << "[INFO] SPZ version: " << static_cast<int>(header.version)
              << ", compression: " << compressionType << std::endl;
    std::cout << "[INFO] Num points: " << header.numPoints << std::endl;
    std::cout << "[INFO] SH degree: " << static_cast<int>(header.shDegree) << std::endl;
    if (coordinateSystem > 0) {
        std::cout << "[INFO] Coordinate system extension found: " << coordinateSystem << std::endl;
    }
    std::cout << "[INFO] Creating glTF Asset with KHR extensions" << std::endl;

    auto asset = createGltfAsset(asByteSpan(spzData, spzSize), header, compressionType, coordinateSystem);

    std::cout << "[INFO] Exporting GLB..." << std::endl;
    fastgltf::Exporter exporter;
    auto result = exporter.writeGltfBinary(asset);
    if (result.error() != fastgltf::Error::None) {
        std::cerr << "[ERROR] GLB export failed: " << std::string(fastgltf::getErrorMessage(result.error())) << std::endl;
        return false;
    }

    glbData = std::move(result.get().output);
    return true;
}

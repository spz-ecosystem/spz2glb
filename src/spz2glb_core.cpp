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

constexpr uint32_t kSpzMagic = 0x5053474e;
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

    if (!isGzipData(data, size)) {
        return parseSpzHeader(data, size, header);
    }

    return peekSpzHeaderFromGzip(data, size, header, workArena);
}

fastgltf::Asset createGltfAsset(fastgltf::span<const std::byte> spzData, const SpzHeader& header) {
    (void)header;

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

    std::cout << "[INFO] SPZ version: " << static_cast<int>(header.version) << std::endl;
    std::cout << "[INFO] Num points: " << header.numPoints << std::endl;
    std::cout << "[INFO] SH degree: " << static_cast<int>(header.shDegree) << std::endl;
    std::cout << "[INFO] Creating glTF Asset with KHR extensions" << std::endl;

    auto asset = createGltfAsset(asByteSpan(spzData, spzSize), header);

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

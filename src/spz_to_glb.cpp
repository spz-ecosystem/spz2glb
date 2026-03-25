// Copyright (c) 2026 Pu Junhan
// SPDX-License-Identifier: MIT
// Project: SPZ-ecosystem
// Repository: https://github.com/spz-ecosystem/spz2glb
//
/**
 * SPZ 到 GLB 转换器
 * 
 * 将 SPZ 文件转换为 glTF 2.0 GLB 格式
 * 使用 KHR_gaussian_splatting_compression_spz_2 扩展
 *
 * 压缩流模式（根据 SPZ_2 规范）：
 * - SPZ 压缩数据直接存储在 bufferView 中
 * - 不定义 accessors 或 attributes
 * - 渲染需要 SPZ 兼容的解码器
 * 
 * 这是 SPZ_2 规范推荐的模式：
 * - 无损（无重新编码，直接复制 SPZ 流）
 * - 最小文件大小（SPZ 压缩率约 10 倍）
 * - 最快加载速度
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <memory>
#include <optional>
#include <span>
#include <zlib.h>


#include "memory_pool.h"

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>

#ifdef __EMSCRIPTEN__
#ifndef SPZ2GLB_DISABLE_EMBIND
#include <emscripten/bind.h>
#include "emscripten_utils.h"
#endif
#endif

enum class SpzErrorCode {
    Success = 0,
    CannotOpenSpzFile = 1,
    FailedToReadSpzFile = 2,
    FailedToInitZlib = 3,
    FailedToDecompress = 4,
    ConversionFailed = 5,
    CannotOpenOutputFile = 6
};

struct SpzResult {
    bool success;
    std::string errorMessage;
    std::vector<uint8_t> data;

    static SpzResult ok(std::vector<uint8_t> data) {
        return {true, "", std::move(data)};
    }
    static SpzResult error(SpzErrorCode code, const std::string& msg) {
        (void)code;
        return {false, msg, {}};
    }
};

/**
 * SPZ 文件格式头结构（16 字节）
 */
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


/**
 * 加载 SPZ 文件（二进制读取）
 * 
 * @param spzPath SPZ 文件路径
 * @return 包含完整 SPZ 二进制数据的 vector
 * 
 * 关键点：
 * - 以二进制模式读取，保持原始字节不变
 * - 使用 ios::ate 先定位到文件末尾获取大小
 * - 返回的是 gzip 压缩的原始数据，不解压
 * 
 * 为什么保持压缩状态？
 * - SPZ 压缩率约 10 倍，解压后会变大
 * - GLB 存储压缩数据，加载时由 SPZ 解码器解压
 * - 符合 SPZ_2 规范的压缩流模式
 */
SpzResult loadSpzFile(const std::string& spzPath) {
    // 以二进制模式打开文件，ios::ate 将读取位置定位到文件末尾
    std::ifstream file(spzPath, std::ios::binary | std::ios::ate);
    if (!file) {
        return SpzResult::error(SpzErrorCode::CannotOpenSpzFile,
            "Cannot open SPZ file: " + spzPath);
    }

    // 获取文件大小（tellg 返回当前位置，即文件末尾）
    auto size = file.tellg();
    // 重置读取位置到文件开头
    file.seekg(0, std::ios::beg);

    // 分配缓冲区并调整大小
    std::vector<uint8_t> rawBuffer;
    rawBuffer.resize(static_cast<size_t>(size));

    // 一次性读取整个文件到缓冲区
    if (!file.read(reinterpret_cast<char*>(rawBuffer.data()), static_cast<std::streamsize>(size))) {
        return SpzResult::error(SpzErrorCode::FailedToReadSpzFile,
            "Failed to read SPZ file");
    }

    // 返回原始 SPZ 数据（保持 gzip 压缩状态）
    // 重要：不要解压！GLB 必须存储原始压缩数据
    // SPZ 解码器在加载时会自动解压
    return SpzResult::ok(std::move(rawBuffer));
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


#ifdef __EMSCRIPTEN__

#ifndef SPZ2GLB_DISABLE_EMBIND
/**
 * WASM 导出函数：SPZ 转 GLB
 *
 * @param spzBuffer JavaScript Uint8Array (SPZ 文件数据)
 * @return JavaScript Uint8Array (GLB 文件数据)，失败返回 null
 *
 * JavaScript 使用示例：
 * const Module = await createSpz2GlbModule();
 * const spzData = new Uint8Array([...]);
 * const glbData = Module.convertSpzToGlb(spzData);
 */
emscripten::val convertSpzToGlb(const emscripten::val& spzBuffer) {
    std::vector<uint8_t> spzData = spz2glb::vectorFromJsArray(spzBuffer);

    std::vector<std::byte> glbData;
    if (!convertSpzToGlbCore(spzData.data(), spzData.size(), glbData)) {
        return emscripten::val::null();
    }

    return spz2glb::jsUint8ArrayFromBytes(glbData);
}

EMSCRIPTEN_BINDINGS(spz2glb_module) {
    emscripten::function("convertSpzToGlb", &convertSpzToGlb);
    emscripten::function("getMemoryStats", &spz2glb::getMemoryStats);
}
#endif

#else  // __EMSCRIPTEN__

#include "spz_verifier.h"

#ifndef SPZ2GLB_NO_CLI_MAIN
void printUsage(const char* progName) {
    std::cout << "SPZ to GLB Converter\n";
    std::cout << "Usage: " << progName << " <input.spz> <output.glb> [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --verify    Run three-layer verification after conversion\n";
    std::cout << "  --help      Show this help message\n";
}

int main(int argc, char** argv) {
    bool doVerify = false;
    std::string inputPath;
    std::string outputPath;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--verify") {
            doVerify = true;
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg[0] != '-') {
            if (inputPath.empty()) {
                inputPath = arg;
            } else if (outputPath.empty()) {
                outputPath = arg;
            }
        } else {
            std::cerr << "[ERROR] Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    if (inputPath.empty() || outputPath.empty()) {
        std::cerr << "[ERROR] Missing input or output file\n";
        printUsage(argv[0]);
        return 1;
    }

    std::cout << "[INFO] Loading SPZ: " << inputPath << std::endl;
    auto spzResult = loadSpzFile(inputPath);
    if (!spzResult.success) {
        std::cerr << "[ERROR] " << spzResult.errorMessage << std::endl;
        return 1;
    }

    std::cout << "[INFO] Converting to GLB..." << std::endl;
    std::vector<std::byte> glbData;
    if (!convertSpzToGlbCore(spzResult.data.data(), spzResult.data.size(), glbData)) {
        std::cerr << "[ERROR] Conversion failed" << std::endl;
        return 1;
    }

    std::cout << "[INFO] Writing GLB: " << outputPath << std::endl;
    std::ofstream file(outputPath, std::ios::binary);
    if (!file) {
        std::cerr << "[ERROR] Cannot open output file: " << outputPath << std::endl;
        return 1;
    }

    file.write(reinterpret_cast<const char*>(glbData.data()), static_cast<std::streamsize>(glbData.size()));

    std::cout << "[SUCCESS] GLB exported: " << outputPath << std::endl;
    std::cout << "[INFO] GLB size: " << (glbData.size() / 1024.0 / 1024.0) << " MB" << std::endl;

    if (doVerify) {

        std::cout << "\n============================================================\n";
        std::cout << "Running Three-Layer Verification...\n";
        std::cout << "============================================================\n\n";
        
        std::vector<uint8_t> glbBytes(glbData.size());
        std::memcpy(glbBytes.data(), glbData.data(), glbData.size());

        spz::Verifier verifier;
        auto result = verifier.verify(spzResult.data, glbBytes);

        
        std::cout << result.layer1_detail << "\n";
        std::cout << result.layer2_detail << "\n";
        std::cout << result.layer3_detail << "\n";
        
        std::cout << "============================================================\n";
        std::cout << "Summary:\n";
        std::cout << "  Layer 1 (GLB Structure): " << (result.layer1_passed ? "PASSED" : "FAILED") << "\n";
        std::cout << "  Layer 2 (Binary Lossless): " << (result.layer2_passed ? "PASSED" : "FAILED") << "\n";
        std::cout << "  Layer 3 (Decoding): " << (result.layer3_passed ? "PASSED" : "FAILED") << "\n";
        std::cout << "============================================================\n";
        
        if (result.all_passed()) {
            std::cout << "\n[SUCCESS] All verifications PASSED!\n";
        } else {
            std::cout << "\n[WARNING] Some verifications FAILED!\n";
            return 2;
        }
    }

    return 0;
}
#endif

#endif  // __EMSCRIPTEN__

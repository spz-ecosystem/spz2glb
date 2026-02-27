/**
 * SPZ to GLB Converter
 * 
 * Converts SPZ files to glTF 2.0 GLB format
 * with KHR_gaussian_splatting_compression_spz_2 extension.
 *
 * COMPRESSION STREAM MODE (per SPZ_2 specification):
 * - SPZ compressed data is stored directly in bufferView
 * - No accessors or attributes are defined
 * - Rendering requires a SPZ-compatible decoder
 * 
 * This is the recommended mode per SPZ_2 specification:
 * - Lossless (no re-encoding, direct copy of SPZ stream)
 * - Smallest file size (SPZ compression ~10x)
 * - Fastest loading
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <memory>
#include <zlib.h>

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>

struct SpzHeader {
    uint32_t magic;        // 0x5053474e "NGSP"
    uint32_t version;      // 2 or 3
    uint32_t numPoints;    // Number of gaussian points
    uint8_t shDegree;      // 0-3
    uint8_t fractionalBits;
    uint8_t flags;
    uint8_t reserved;
};

bool parseSpzHeader(const std::vector<uint8_t>& data, SpzHeader& header) {
    if (data.size() < sizeof(SpzHeader)) {
        return false;
    }

    memcpy(&header, data.data(), sizeof(SpzHeader));

    if (header.magic != 0x5053474e) {
        std::cerr << "[ERROR] Invalid SPZ magic number: 0x" << std::hex << header.magic << std::dec << std::endl;
        return false;
    }

    return true;
}

int getAccessorCount(int shDegree) {
    int baseAccessors = 4; // POSITION, COLOR_0, SCALE, ROTATION
    int shAccessors = 0;

    if (shDegree >= 1) shAccessors += 3;
    if (shDegree >= 2) shAccessors += 5;
    if (shDegree >= 3) shAccessors += 7;

    return baseAccessors + shAccessors;
}

std::vector<uint8_t> loadSpzFile(const std::string& spzPath) {
    std::ifstream file(spzPath, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Cannot open SPZ file: " + spzPath);
    }

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> rawBuffer;
    rawBuffer.resize(static_cast<size_t>(size));

    if (!file.read(reinterpret_cast<char*>(rawBuffer.data()), size)) {
        throw std::runtime_error("Failed to read SPZ file");
    }

    // Return raw SPZ data as-is (gzip compressed)
    // DO NOT decompress - the GLB must store the original compressed data
    // The SPZ decoder will decompress when loading
    return rawBuffer;
}

std::vector<uint8_t> decompressSpzData(const std::vector<uint8_t>& compressedData) {
    // For header parsing only: decompress gzip to get SPZ internal format
    // This is only needed to read metadata, not for storage
    if (compressedData.size() < 2 || compressedData[0] != 0x1f || compressedData[1] != 0x8b) {
        // Not gzip compressed, return as-is
        return compressedData;
    }

    std::vector<uint8_t> decompressed;
    decompressed.resize(compressedData.size() * 10);

    z_stream strm = {};
    strm.next_in = const_cast<uint8_t*>(compressedData.data());
    strm.avail_in = compressedData.size();
    strm.next_out = decompressed.data();
    strm.avail_out = decompressed.size();

    if (inflateInit2(&strm, 16 + MAX_WBITS) != Z_OK) {
        throw std::runtime_error("Failed to initialize zlib decompression");
    }

    int ret;
    do {
        if (strm.avail_out == 0) {
            size_t oldSize = decompressed.size();
            decompressed.resize(oldSize * 2);
            strm.next_out = decompressed.data() + oldSize;
            strm.avail_out = oldSize;
        }
        ret = inflate(&strm, Z_NO_FLUSH);
    } while (ret == Z_OK);

    if (ret != Z_STREAM_END) {
        inflateEnd(&strm);
        throw std::runtime_error("Failed to decompress SPZ file");
    }

    decompressed.resize(strm.total_out);
    inflateEnd(&strm);

    return decompressed;
}

fastgltf::Asset createGltfAsset(std::vector<uint8_t>& spzData, const SpzHeader& header) {
    (void)header; // Header info available for future use
    
    fastgltf::Asset asset;

    asset.extensionsUsed.emplace_back("KHR_gaussian_splatting");
    asset.extensionsUsed.emplace_back("KHR_gaussian_splatting_compression_spz_2");

    asset.extensionsRequired.emplace_back("KHR_gaussian_splatting");
    asset.extensionsRequired.emplace_back("KHR_gaussian_splatting_compression_spz_2");

    asset.assetInfo.emplace();
    asset.assetInfo->gltfVersion = "2.0";
    asset.assetInfo->copyright = "";
    asset.assetInfo->generator = "spz_to_glb_fastgltf";

    size_t spzSize = spzData.size();

    // Convert uint8_t vector to std::byte vector
    std::vector<std::byte> byteData;
    byteData.reserve(spzSize);
    for (const auto& b : spzData) {
        byteData.push_back(static_cast<std::byte>(b));
    }

    fastgltf::Buffer buffer;
    buffer.data.emplace<fastgltf::sources::Vector>();
    std::get<fastgltf::sources::Vector>(buffer.data).bytes = std::move(byteData);
    buffer.byteLength = spzSize;
    asset.buffers.emplace_back(std::move(buffer));

    fastgltf::BufferView spzBufferView;
    spzBufferView.bufferIndex = 0;
    spzBufferView.byteOffset = 0;
    spzBufferView.byteLength = spzSize;
    asset.bufferViews.emplace_back(std::move(spzBufferView));

    // Create primitive with extensions (compression stream mode)
    fastgltf::Primitive primitive;
    primitive.type = fastgltf::PrimitiveType::Points;

    // Add SPZ compression extension reference to bufferView 0
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

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input.spz> <output.glb>" << std::endl;
        return 1;
    }

    std::string inputPath = argv[1];
    std::string outputPath = argv[2];

    try {
        std::cout << "[INFO] Loading SPZ: " << inputPath << std::endl;

        auto spzData = loadSpzFile(inputPath);

        // Decompress only for header parsing
        auto decompressedData = decompressSpzData(spzData);

        SpzHeader header;
        if (!parseSpzHeader(decompressedData, header)) {
            throw std::runtime_error("Failed to parse SPZ header");
        }

        std::cout << "[INFO] SPZ version: " << (int)header.version << std::endl;
        std::cout << "[INFO] Num points: " << header.numPoints << std::endl;
        std::cout << "[INFO] SH degree: " << (int)header.shDegree << std::endl;
        std::cout << "[INFO] SPZ size (raw compressed): " << spzData.size() / 1024 / 1024 << " MB" << std::endl;

        std::cout << "[INFO] Creating glTF Asset with KHR extensions" << std::endl;
        auto asset = createGltfAsset(spzData, header);

        std::cout << "[INFO] Exporting GLB..." << std::endl;
        fastgltf::Exporter exporter;

        auto result = exporter.writeGltfBinary(asset);
        if (result.error() != fastgltf::Error::None) {
            throw std::runtime_error("GLB export failed: " + std::string(fastgltf::getErrorMessage(result.error())));
        }

        std::ofstream file(outputPath, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot open output file: " + outputPath);
        }

        const auto& output = result.get().output;
        file.write(reinterpret_cast<const char*>(output.data()), output.size());

        std::cout << "[SUCCESS] GLB exported: " << outputPath << std::endl;
        std::cout << "[INFO] GLB size: " << output.size() / 1024 / 1024 << " MB" << std::endl;

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return 1;
    }
}

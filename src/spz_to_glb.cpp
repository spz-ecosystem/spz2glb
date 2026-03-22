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
#include <zlib.h>

#include "memory_pool.h"

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#include "emscripten_utils.h"
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
 * 
 * SPZ 文件使用大端序存储，包含高斯泼溅的元数据
 * Magic: "NGSP" (0x5053474e) - 标识 SPZ 文件
 */
struct SpzHeader {
    uint32_t magic;        // 魔术数字：0x5053474e ("NGSP")，用于标识 SPZ 文件
    uint32_t version;      // SPZ 规范版本：2 或 3
    uint32_t numPoints;    // 高斯泼溅点数量，决定渲染质量
    uint8_t shDegree;      // 球谐函数阶数：0-3（0=无 SH，3=最高质量）
    uint8_t fractionalBits; // 定点数精度位数
    uint8_t flags;         // 标志位
    uint8_t reserved;      // 保留字节（对齐用）
};

/**
 * 解析 SPZ 文件头
 * 
 * @param data SPZ 文件的二进制数据（已解压）
 * @param header 输出参数，存储解析后的头信息
 * @return true 如果头解析成功，false 如果格式错误
 * 
 * 验证步骤：
 * 1. 检查数据大小是否足够（至少 16 字节）
 * 2. 复制二进制数据到结构体
 * 3. 验证魔术数字是否为 "NGSP"
 */
bool parseSpzHeader(const std::vector<uint8_t>& data, SpzHeader& header) {
    // 数据必须至少包含完整的头结构（16 字节）
    if (data.size() < sizeof(SpzHeader)) {
        return false;
    }

    // 从二进制数据复制头结构
    memcpy(&header, data.data(), sizeof(SpzHeader));

    // 验证魔术数字：必须是 0x5053474e ("NGSP")
    if (header.magic != 0x5053474e) {
        std::cerr << "[ERROR] Invalid SPZ magic number: 0x" << std::hex << header.magic << std::dec << std::endl;
        return false;
    }

    return true;
}

/**
 * 计算 glTF accessor 数量（基于球谐函数阶数）
 * 
 * @param shDegree 球谐函数阶数（0-3）
 * @return 需要的 accessor 总数
 * 
 * glTF 高斯泼溅需要以下属性：
 * - 基础属性（4 个）：POSITION, COLOR_0, SCALE, ROTATION
 * - 球谐系数（可选）：根据 SH 阶数增加
 *   - SH 阶数 1: +3 个系数
 *   - SH 阶数 2: +5 个系数
 *   - SH 阶数 3: +7 个系数
 * 
 * 注意：在压缩流模式（compression stream mode）下，
 * 这些 accessor 都不需要，因为数据存储在 SPZ 压缩流中
 */
int getAccessorCount(int shDegree) {
    // 基础属性：位置、颜色、缩放、旋转
    int baseAccessors = 4; // POSITION, COLOR_0, SCALE, ROTATION
    int shAccessors = 0;

    // 根据 SH 阶数添加球谐系数
    // 球谐函数用于编码光照方向信息，阶数越高，光照质量越好
    if (shDegree >= 1) shAccessors += 3;  // 一阶 SH：3 个系数
    if (shDegree >= 2) shAccessors += 5;  // 二阶 SH：5 个系数
    if (shDegree >= 3) shAccessors += 7;  // 三阶 SH：7 个系数

    return baseAccessors + shAccessors;
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
    if (!file.read(reinterpret_cast<char*>(rawBuffer.data()), size)) {
        return SpzResult::error(SpzErrorCode::FailedToReadSpzFile,
            "Failed to read SPZ file");
    }

    // 返回原始 SPZ 数据（保持 gzip 压缩状态）
    // 重要：不要解压！GLB 必须存储原始压缩数据
    // SPZ 解码器在加载时会自动解压
    return SpzResult::ok(std::move(rawBuffer));
}

/**
 * 解压 SPZ gzip 数据（仅用于头解析）
 * 
 * @param compressedData gzip 压缩的 SPZ 数据
 * @return 解压后的 SPZ 内部格式数据
 * 
 * 用途：
 * - 仅用于解析 SPZ 头部元数据
 * - 不用于存储（存储时保持压缩状态）
 * 
 * gzip 格式识别：
 * - 前两个字节：0x1f 0x8b
 * - 如果不是 gzip，直接返回原始数据
 * 
 * 解压流程：
 * 1. 检测 gzip 魔数（0x1f8b）
 * 2. 初始化 zlib 解压流（16 + MAX_WBITS 表示 gzip 格式）
 * 3. 动态扩展输出缓冲区（按需 2 倍增长）
 * 4. 循环解压直到 Z_STREAM_END
 * 5. 调整最终大小并返回
 */
SpzResult decompressSpzData(std::vector<uint8_t> compressedData) {
    // 检查 gzip 魔数：前两个字节必须是 0x1f 0x8b
    if (compressedData.size() < 2 || compressedData[0] != 0x1f || compressedData[1] != 0x8b) {
        // 不是 gzip 压缩，直接返回原始数据
        return SpzResult::ok(const_cast<std::vector<uint8_t>&>(compressedData));
    }

    // 预分配解压缓冲区（假设压缩率约 10 倍）
    std::vector<uint8_t> decompressed;
    decompressed.resize(compressedData.size() * 10);

    // 初始化 zlib 流结构
    z_stream strm = {};
    strm.next_in = const_cast<uint8_t*>(compressedData.data());        // 输入数据指针
    strm.avail_in = static_cast<uInt>(compressedData.size());          // 输入数据大小
    strm.next_out = decompressed.data();                                // 输出缓冲区指针
    strm.avail_out = static_cast<uInt>(decompressed.size());            // 输出缓冲区大小

    // 初始化解压：16 + MAX_WBITS 表示使用 gzip 格式（而不是 zlib）
    if (inflateInit2(&strm, 16 + MAX_WBITS) != Z_OK) {
        return SpzResult::error(SpzErrorCode::FailedToInitZlib,
            "Failed to initialize zlib decompression");
    }

    // 循环解压直到完成
    int ret;
    do {
        // 如果输出缓冲区满了，扩展 2 倍
        if (strm.avail_out == 0) {
            size_t oldSize = decompressed.size();
            decompressed.resize(oldSize * 2);
            // 设置新的输出位置到扩展后的位置
            strm.next_out = decompressed.data() + oldSize;
            strm.avail_out = static_cast<uInt>(oldSize);
        }
        // 解压一块数据
        ret = inflate(&strm, Z_NO_FLUSH);
    } while (ret == Z_OK);  // Z_OK 表示还有更多数据

    // 检查解压结果：必须是 Z_STREAM_END
    if (ret != Z_STREAM_END) {
        inflateEnd(&strm);
        return SpzResult::error(SpzErrorCode::FailedToDecompress,
            "Failed to decompress SPZ file");
    }

    // 调整到实际解压的大小
    decompressed.resize(strm.total_out);
    // 释放 zlib 资源
    inflateEnd(&strm);

    return SpzResult::ok(std::move(decompressed));
}

/**
 * 创建 glTF 资产（包含 SPZ 压缩扩展）
 * 
 * @param spzData SPZ 压缩数据（保持 gzip 压缩状态）
 * @param header SPZ 头部信息（预留未来使用）
 * @return 完整的 glTF 资产对象
 * 
 * glTF 结构：
 * - Asset: 根对象，包含元数据
 * - Buffers: 二进制数据（SPZ 压缩流）
 * - BufferViews: Buffer 的视图（指向整个 SPZ 数据）
 * - Mesh/Primitive: 点云几何体（使用 SPZ 扩展）
 * - Node: 场景节点
 * - Scene: 默认场景
 * 
 * 关键扩展：
 * - KHR_gaussian_splatting: 高斯泼溅支持
 * - KHR_gaussian_splatting_compression_spz_2: SPZ 压缩格式
 * 
 * 压缩流模式特点：
 * - 没有 accessors（数据在压缩流中）
 * - 没有 attributes（数据在压缩流中）
 * - 渲染器需要 SPZ 解码器
 */
fastgltf::Asset createGltfAsset(std::vector<uint8_t> spzData, const SpzHeader& header) {
    (void)header; // 头部信息预留未来使用（如验证点数等）
    
    fastgltf::Asset asset;

    // 注册使用的扩展（必须声明才能使用）
    asset.extensionsUsed.emplace_back("KHR_gaussian_splatting");
    asset.extensionsUsed.emplace_back("KHR_gaussian_splatting_compression_spz_2");

    // 必需的扩展（没有这些扩展无法正确渲染）
    asset.extensionsRequired.emplace_back("KHR_gaussian_splatting");
    asset.extensionsRequired.emplace_back("KHR_gaussian_splatting_compression_spz_2");

    // glTF 资产元信息
    asset.assetInfo.emplace();
    asset.assetInfo->gltfVersion = "2.0";           // glTF 2.0 规范
    asset.assetInfo->copyright = "";                // 版权信息（空）
    asset.assetInfo->generator = "spz_to_glb_fastgltf";  // 生成器名称

    // 获取 SPZ 数据大小
    size_t spzSize = spzData.size();

    // 创建 glTF Buffer（存储 SPZ 压缩数据）
    // 使用 ByteView 避免拷贝：直接引用原始数据
    fastgltf::Buffer buffer;
    fastgltf::sources::ByteView byteView;
    byteView.bytes = fastgltf::span<const std::byte>(
        reinterpret_cast<const std::byte*>(spzData.data()),
        spzSize
    );
    byteView.mimeType = fastgltf::MimeType::None;
    buffer.data = std::move(byteView);
    buffer.byteLength = spzSize;
    asset.buffers.emplace_back(std::move(buffer));

    // 创建 BufferView（指向整个 Buffer）
    fastgltf::BufferView spzBufferView;
    spzBufferView.bufferIndex = 0;         // 引用第 0 个 Buffer
    spzBufferView.byteOffset = 0;          // 从开头开始
    spzBufferView.byteLength = spzSize;    // 长度为整个 SPZ 数据
    asset.bufferViews.emplace_back(std::move(spzBufferView));

    // 创建 Primitive（使用 SPZ 压缩扩展）
    fastgltf::Primitive primitive;
    primitive.type = fastgltf::PrimitiveType::Points;  // 点云类型

    // 配置 SPZ 压缩扩展
    // 这是压缩流模式的核心：通过扩展引用 bufferView 中的压缩数据
    auto gaussianSplat = std::make_unique<fastgltf::GaussianSplatExtension>();
    auto spzCompression = std::make_unique<fastgltf::GaussianSplatSpzCompression>();
    spzCompression->bufferView = 0;  // 引用第 0 个 bufferView
    gaussianSplat->spzCompression = std::move(spzCompression);
    primitive.gaussianSplat = std::move(gaussianSplat);

    // 创建 Mesh 并添加 Primitive
    fastgltf::Mesh mesh;
    mesh.primitives.emplace_back(std::move(primitive));
    asset.meshes.emplace_back(std::move(mesh));

    // 创建 Node（场景中的对象）
    fastgltf::Node node;
    node.meshIndex = 0;  // 引用第 0 个 Mesh
    asset.nodes.emplace_back(std::move(node));

    // 创建 Scene（场景根节点）
    fastgltf::Scene scene;
    scene.nodeIndices.emplace_back(0);  // 包含第 0 个 Node
    asset.scenes.emplace_back(std::move(scene));
    asset.defaultScene = 0;  // 默认显示第 0 个场景

    return asset;
}

/**
 * 程序入口：SPZ 到 GLB 转换器
 *
 * 使用方法：spz_to_glb <input.spz> <output.glb>
 *
 * 转换流程：
 * 1. 读取 SPZ 文件（保持 gzip 压缩状态）
 * 2. 解压副本用于解析头部（获取元数据）
 * 3. 创建 glTF 资产（包含 SPZ 压缩扩展）
 * 4. 导出 GLB 二进制文件
 *
 * 输出信息：
 * - SPZ 版本号
 * - 高斯点数量
 * - 球谐函数阶数
 * - SPZ 文件大小（压缩后）
 * - GLB 文件大小
 */

/**
 * 核心转换函数（桌面版和 WASM 共用）
 *
 * @param spzData SPZ 压缩数据
 * @param glbData 输出 GLB 数据
 * @return true 如果转换成功
 *
 * 转换流程：
 * 1. 解压副本用于解析头部
 * 2. 解析 SPZ 头部
 * 3. 创建 glTF 资产
 * 4. 导出 GLB
 */
bool convertSpzToGlbCore(std::vector<uint8_t> spzData, std::vector<uint8_t>& glbData) {
    // 步骤 1: 解压（移动语义，避免拷贝）
    auto decompressResult = decompressSpzData(std::move(spzData));
    if (!decompressResult.success) {
        std::cerr << "[ERROR] " << decompressResult.errorMessage << std::endl;
        return false;
    }
    std::vector<uint8_t> decompressedData = std::move(decompressResult.data);

    // 步骤 2: 解析 SPZ 头部
    SpzHeader header;
    if (!parseSpzHeader(decompressedData, header)) {
        std::cerr << "[ERROR] Failed to parse SPZ header" << std::endl;
        return false;
    }

    // 步骤 3: 打印 SPZ 元数据
    std::cout << "[INFO] SPZ version: " << (int)header.version << std::endl;
    std::cout << "[INFO] Num points: " << header.numPoints << std::endl;
    std::cout << "[INFO] SH degree: " << (int)header.shDegree << std::endl;
    std::cout << "[INFO] SPZ size (raw compressed): " << (spzData.size() / 1024 / 1024) << " MB" << std::endl;

    // 步骤 4: 创建 glTF 资产
    std::cout << "[INFO] Creating glTF Asset with KHR extensions" << std::endl;
    auto asset = createGltfAsset(std::move(decompressedData), header);

    // 步骤 5: 导出 GLB
    std::cout << "[INFO] Exporting GLB..." << std::endl;
    fastgltf::Exporter exporter;

    auto result = exporter.writeGltfBinary(asset);
    if (result.error() != fastgltf::Error::None) {
        std::cerr << "[ERROR] GLB export failed: " << std::string(fastgltf::getErrorMessage(result.error())) << std::endl;
        return false;
    }

    // 步骤 6: 获取 GLB 数据（移动语义，避免拷贝）
    std::vector<std::byte> output = std::move(result.get().output);
    glbData.resize(output.size());
    std::memcpy(glbData.data(), output.data(), output.size());

    return true;
}

#ifdef __EMSCRIPTEN__

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
    // JavaScript Uint8Array 转 C++ vector（Embind 标准做法）
    std::vector<uint8_t> spzData = spz2glb::vectorFromJsArray(spzBuffer);

    // 调用核心转换函数
    std::vector<uint8_t> glbData;
    if (!convertSpzToGlbCore(spzData, glbData)) {
        return emscripten::val::null();
    }

    // 返回 JavaScript Uint8Array
    return spz2glb::jsUint8ArrayFromVector(glbData);
}

EMSCRIPTEN_BINDINGS(spz2glb_module) {
    emscripten::function("convertSpzToGlb", &convertSpzToGlb);
    emscripten::function("getMemoryStats", &spz2glb::getMemoryStats);
}

#else  // __EMSCRIPTEN__

int main(int argc, char** argv) {
    // 参数检查：需要输入文件和输出文件两个参数
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input.spz> <output.glb>" << std::endl;
        return 1;
    }

    // 获取命令行参数
    std::string inputPath = argv[1];   // 输入 SPZ 文件路径
    std::string outputPath = argv[2];  // 输出 GLB 文件路径

    // 步骤 1: 加载 SPZ 文件
    std::cout << "[INFO] Loading SPZ: " << inputPath << std::endl;
    auto spzResult = loadSpzFile(inputPath);
    if (!spzResult.success) {
        std::cerr << "[ERROR] " << spzResult.errorMessage << std::endl;
        return 1;
    }

    // 步骤 2: 调用核心转换函数
    std::vector<uint8_t> glbData;
    if (!convertSpzToGlbCore(spzResult.data, glbData)) {
        std::cerr << "[ERROR] Conversion failed" << std::endl;
        return 1;
    }

    // 步骤 3: 写入文件
    std::cout << "[INFO] Writing GLB: " << outputPath << std::endl;
    std::ofstream file(outputPath, std::ios::binary);
    if (!file) {
        std::cerr << "[ERROR] Cannot open output file: " << outputPath << std::endl;
        return 1;
    }

    file.write(reinterpret_cast<const char*>(glbData.data()), glbData.size());

    // 步骤 4: 打印成功信息
    std::cout << "[SUCCESS] GLB exported: " << outputPath << std::endl;
    std::cout << "[INFO] GLB size: " << (glbData.size() / 1024 / 1024) << " MB" << std::endl;

    return 0;  // 成功退出
}

#endif  // __EMSCRIPTEN__

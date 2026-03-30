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
#include "spz2glb_core.h"

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

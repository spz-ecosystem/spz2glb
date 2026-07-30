// Copyright (c) 2026 Pu Junhan
// SPDX-License-Identifier: MIT
// Project: SPZ-ecosystem
// Repository: https://github.com/spz-ecosystem/spz2glb
// Cultural Note: Huangdi Era 4723, Year of the Red Fire Horse (Bingwu, 丙午)
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
#include <filesystem>
#include "spz2glb_core.h"
#include "mapped_file.h"

#ifdef __EMSCRIPTEN__
#ifndef SPZ2GLB_DISABLE_EMBIND
#include <emscripten/bind.h>
#include "emscripten_utils.h"
#endif
#endif

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
bool convertSingleFile(const std::string& inputPath, const std::string& outputPath, bool doVerify) {
    spz2glb::MappedFile mappedFile;
    if (!mappedFile.open(inputPath)) {
        std::cerr << "[ERROR] Cannot open SPZ file: " << inputPath << std::endl;
        return false;
    }

    const uint8_t* spzData = mappedFile.data();
    size_t spzSize = mappedFile.size();

    if (spzData == nullptr || spzSize == 0) {
        std::cerr << "[ERROR] Empty SPZ file: " << inputPath << std::endl;
        return false;
    }

    std::cout << "[INFO] Converting to GLB..." << std::endl;
    std::vector<std::byte> glbData;
    if (!convertSpzToGlbCore(spzData, spzSize, glbData)) {
        std::cerr << "[ERROR] Conversion failed" << std::endl;
        return false;
    }

    std::cout << "[INFO] Writing GLB: " << outputPath << std::endl;
    std::ofstream file(outputPath, std::ios::binary);
    if (!file) {
        std::cerr << "[ERROR] Cannot open output file: " << outputPath << std::endl;
        return false;
    }

    file.write(reinterpret_cast<const char*>(glbData.data()),
               static_cast<std::streamsize>(glbData.size()));

    std::cout << "[SUCCESS] GLB exported: " << outputPath << std::endl;
    std::cout << "[INFO] GLB size: " << (glbData.size() / 1024.0 / 1024.0) << " MB" << std::endl;

    if (doVerify) {
        std::cout << "\n============================================================\n";
        std::cout << "Running Five-Layer Verification...\n";
        std::cout << "============================================================\n\n";

        spz::Verifier verifier;
        auto result = verifier.verify(
            spzData, spzSize,
            reinterpret_cast<const uint8_t*>(glbData.data()), glbData.size());

        std::cout << result.layer1_detail << "\n";
        std::cout << result.layer2_detail << "\n";
        std::cout << result.layer3_detail << "\n";
        std::cout << result.layer4_detail << "\n";
        std::cout << result.layer5_detail << "\n";

        std::cout << "============================================================\n";
        std::cout << "Summary:\n";
        std::cout << "  Layer 1 (GLB Structure): " << (result.layer1_passed ? "PASSED" : "FAILED") << "\n";
        std::cout << "  Layer 2 (Binary Lossless): " << (result.layer2_passed ? "PASSED" : "FAILED") << "\n";
        std::cout << "  Layer 3 (Decoding): " << (result.layer3_passed ? "PASSED" : "FAILED") << "\n";
        std::cout << "  Layer 4 (Metadata): " << (result.layer4_passed ? "PASSED" : "FAILED") << "\n";
        std::cout << "  Layer 5 (ILV Extension): " << (result.layer5_passed ? "PASSED" : "FAILED") << "\n";
        std::cout << "============================================================\n";

        if (!result.all_passed()) {
            std::cerr << "[WARNING] Verification failed for: " << inputPath << std::endl;
            return false;
        }
        std::cout << "[SUCCESS] All verifications PASSED!\n";
    }

    return true;
}

void printUsage(const char* progName) {
    std::cout << "SPZ to GLB Converter v2.0.3\n";
    std::cout << "Usage:\n";
    std::cout << "  " << progName << " <input.spz> <output.glb> [--verify]\n";
    std::cout << "  " << progName << " --batch .spz [--verify]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --verify     Run 5-layer verification after conversion\n";
    std::cout << "  --batch EXT  Batch convert all files with given extension (e.g. .spz)\n";
    std::cout << "  --help, -h   Show this help message\n";
}

int main(int argc, char** argv) {
    bool doVerify = false;
    bool batchMode = false;
    std::string batchExt;
    std::string inputPath;
    std::string outputPath;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--verify") {
            doVerify = true;
        } else if (arg == "--batch") {
            if (i + 1 >= argc) {
                std::cerr << "[ERROR] --batch requires an extension argument (e.g. .spz)" << std::endl;
                return 1;
            }
            batchMode = true;
            batchExt = argv[++i];
            if (batchExt.front() != '.') {
                batchExt = "." + batchExt;
            }
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

    if (batchMode) {
        // 批量模式：遍历当前目录，匹配扩展名
        namespace fs = std::filesystem;
        std::vector<fs::path> targets;
        for (const auto& entry : fs::directory_iterator(fs::current_path())) {
            if (entry.is_regular_file() && entry.path().extension() == batchExt) {
                targets.push_back(entry.path());
            }
        }

        if (targets.empty()) {
            std::cout << "[INFO] No files matching *" << batchExt << " found in current directory" << std::endl;
            return 0;
        }

        std::cout << "[INFO] Batch mode: " << targets.size() << " files to convert" << std::endl;
        int successCount = 0;
        int failCount = 0;
        for (const auto& target : targets) {
            auto outPath = target;
            outPath.replace_extension(".glb");
            std::cout << "\n--- [" << (successCount + failCount + 1) << "/" << targets.size()
                      << "] " << target.filename().string() << " ---\n";
            if (convertSingleFile(target.string(), outPath.string(), doVerify)) {
                ++successCount;
            } else {
                std::cerr << "[FAIL] " << target.filename().string() << std::endl;
                ++failCount;
            }
        }

        std::cout << "\n============================================================\n";
        std::cout << "Batch complete: " << successCount << " succeeded, "
                  << failCount << " failed out of " << targets.size() << std::endl;
        return failCount > 0 ? 1 : 0;
    }

    // 单文件模式
    if (inputPath.empty() || outputPath.empty()) {
        std::cerr << "[ERROR] Missing input or output file" << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    return convertSingleFile(inputPath, outputPath, doVerify) ? 0 : 1;
}
#endif

#endif  // __EMSCRIPTEN__

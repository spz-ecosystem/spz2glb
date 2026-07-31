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
#include "queue.h"
#include "mapped_file.h"

#ifndef SPZ2GLB_NO_CLI_MAIN

/// 生成单次转换的 JSON 报告到 stdout
bool generateReport(const std::string& spzPath, const std::string& glbPath) {
    spz2glb::MappedFile spzFile;
    if (!spzFile.open(spzPath)) {
        std::cerr << "[ERROR] Cannot open SPZ file: " << spzPath << std::endl;
        return false;
    }

    spz2glb::MappedFile glbFile;
    if (!glbFile.open(glbPath)) {
        std::cerr << "[ERROR] Cannot open GLB file: " << glbPath << std::endl;
        return false;
    }

    spz2glb::ConversionResult result;
    result.inputFile = std::filesystem::path(spzPath).filename().string();
    result.outputFile = std::filesystem::path(glbPath).filename().string();
    result.spzSizeBytes = spzFile.size();
    result.glbSizeBytes = glbFile.size();

    // 解析 GLB 扩展信息
    std::vector<uint8_t> glbVec(glbFile.data(), glbFile.data() + glbFile.size());

    // 解析 GLB 基本信息
    constexpr uint32_t kGlbMagic = 0x46546C67;
    if (glbVec.size() >= 12) {
        uint32_t magic = 0;
        std::memcpy(&magic, glbVec.data(), 4);
        if (magic == kGlbMagic) {
            std::memcpy(&result.glbTotalSize, glbVec.data() + 8, 4);
        }
    }

    std::cout << result.toJson() << std::flush;
    return true;
}

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
    std::cout << "SPZ to GLB Converter v2.0.4\n";
    std::cout << "Usage:\n";
    std::cout << "  " << progName << " <input.spz> <output.glb> [--verify]\n";
    std::cout << "  " << progName << " --batch EXT [--verify]\n";
    std::cout << "  " << progName << " --queue-add <file.spz>...\n";
    std::cout << "  " << progName << " --queue\n";
    std::cout << "  " << progName << " --queue-status\n";
    std::cout << "  " << progName << " --queue-clear\n";
    std::cout << "  " << progName << " --report <input.spz> <output.glb>\n\n";
    std::cout << "Options:\n";
    std::cout << "  --verify       Run 5-layer verification after conversion\n";
    std::cout << "  --batch EXT    Batch convert all files with given extension (e.g. .spz)\n";
    std::cout << "  --queue-add    Add file(s) to the .spzqueue directory\n";
    std::cout << "  --queue        Start queue processing daemon\n";
    std::cout << "  --queue-status Show queue status\n";
    std::cout << "  --queue-clear  Clear all queue directories\n";
    std::cout << "  --report       Generate JSON report for a single conversion\n";
    std::cout << "  --help, -h     Show this help message\n";
}

int main(int argc, char** argv) {
    bool doVerify = false;
    bool batchMode = false;
    bool queueMode = false;
    bool queueAddMode = false;
    bool queueStatusMode = false;
    bool queueClearMode = false;
    bool reportMode = false;
    std::string batchExt;
    std::string inputPath;
    std::string outputPath;
    std::vector<std::string> queueFiles;

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
        } else if (arg == "--queue") {
            queueMode = true;
        } else if (arg == "--queue-add") {
            queueAddMode = true;
            // 收集后续所有非选项参数作为文件列表
            for (int j = i + 1; j < argc; j++) {
                if (argv[j][0] == '-') break;
                queueFiles.push_back(argv[j]);
            }
            i = argc; // 跳过已处理的文件参数
        } else if (arg == "--queue-status") {
            queueStatusMode = true;
        } else if (arg == "--queue-clear") {
            queueClearMode = true;
        } else if (arg == "--report") {
            reportMode = true;
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

    // 队列模式
    if (queueAddMode) {
        spz2glb::Queue q;
        return q.add(queueFiles) ? 0 : 1;
    }

    if (queueMode) {
        spz2glb::Queue q;
        return q.run(2, doVerify) ? 0 : 1;
    }

    if (queueStatusMode) {
        spz2glb::Queue q;
        return q.printStatus() ? 0 : 1;
    }

    if (queueClearMode) {
        spz2glb::Queue q;
        return q.clear() ? 0 : 1;
    }

    // 报告模式
    if (reportMode) {
        if (inputPath.empty() || outputPath.empty()) {
            std::cerr << "[ERROR] --report requires <input.spz> and <output.glb>" << std::endl;
            return 1;
        }
        return generateReport(inputPath, outputPath) ? 0 : 1;
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

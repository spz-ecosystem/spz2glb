// Copyright (c) 2026 Pu Junhan
// SPDX-License-Identifier: MIT
// Project: SPZ-ecosystem
// Repository: https://github.com/spz-ecosystem/spz2glb
// Cultural Note: Huangdi Era 4723, Year of the Red Fire Horse (Bingwu, 丙午)

#include "spz_verifier.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

void printUsage(const char* progName) {
    std::cout << "SPZ to GLB Verification Tool\n";
    std::cout << "Usage: " << progName << " <command> [options]\n\n";
    std::cout << "  Commands:\n";
    std::cout << "    layer1 <glb>           - Validate GLB structure & KHR extension fields (Layer 1)\n";
    std::cout << "    layer2 <spz> <glb>     - Payload extraction & byte equality (Layer 2)\n";
    std::cout << "    layer3 <spz> <glb>     - Decoding consistency & v4 checks (Layer 3)\n";
    std::cout << "    layer4 <spz> <glb>     - GLB metadata vs SPZ header consistency (Layer 4)\n";
    std::cout << "    layer5 <spz>           - ILV extension completeness (Layer 5)\n";
    std::cout << "    all <spz> <glb>        - Run all five layers\n";
    std::cout << "    verify <spz> <glb>     - Alias for 'all'\n\n";
    std::cout << "  Options:\n";
    std::cout << "    --report <file.json>   - Validate conversion report (optional)\n";
}

void printSummary(const spz::VerifyResult& result) {
    std::cout << "============================================================\n";
    std::cout << "Summary:\n";
    std::cout << "  Layer 1 (GLB Structure & KHR Extension): " << (result.layer1_passed ? "PASSED" : "FAILED") << "\n";
    std::cout << "  Layer 2 (Binary Lossless): " << (result.layer2_passed ? "PASSED" : "FAILED") << "\n";
    std::cout << "  Layer 3 (Decoding): " << (result.layer3_passed ? "PASSED" : "FAILED") << "\n";
    std::cout << "  Layer 4 (Metadata): " << (result.layer4_passed ? "PASSED" : "FAILED") << "\n";
    std::cout << "  Layer 5 (ILV Extensions): " << (result.layer5_passed ? "PASSED" : "FAILED") << "\n";
    std::cout << "  Report Validation: " << (result.report_passed ? "PASSED" : "FAILED") << "\n";
    std::cout << "============================================================\n";
}

bool runLayer1(spz::Verifier& verifier, const std::string& glbPath) {
    std::string detail;
    const bool ok = verifier.verify_layer1_file(glbPath, detail);
    std::cout << detail;
    std::cout << (ok ? "[PASS] Layer 1 validation passed\n" : "[FAIL] Layer 1 validation failed\n");
    return ok;
}

// 从 argv 中提取 --report <path> 的值，返回空串表示无 --report
std::string extractReportPath(int argc, char** argv, int skipBefore) {
    for (int i = skipBefore + 1; i < argc - 1; ++i) {
        if (argv[i] == std::string("--report")) {
            return argv[i + 1];
        }
    }
    return {};
}

// 返回 argc 中位置参数（非 --option）的数量
int countPositionalArgs(int argc, char** argv) {
    int count = 0;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg.find("--") == 0) {
            ++i; // skip value
            continue;
        }
        ++count;
    }
    return count;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    const std::string command = argv[1];
    spz::Verifier verifier;

    if (command == "layer1" && countPositionalArgs(argc, argv) >= 2) {
        return runLayer1(verifier, argv[2]) ? 0 : 1;
    }

    if ((command == "layer2" || command == "layer3" || command == "layer4") && countPositionalArgs(argc, argv) >= 3) {
        int pos = 0;
        for (int i = 2; i < argc; ++i) {
            if (std::string(argv[i]).find("--") == 0) { ++i; continue; }
            if (pos == 0) { /* spz, skip */ pos++; continue; }
            const auto result = verifier.verify_files(argv[2], argv[i]);
            auto getDetail = [&]() -> std::string {
                if (command == "layer2") return result.layer2_detail;
                if (command == "layer3") return result.layer3_detail;
                return result.layer4_detail;
            };
            auto getPassed = [&]() -> bool {
                if (command == "layer2") return result.layer2_passed;
                if (command == "layer3") return result.layer3_passed;
                return result.layer4_passed;
            };
            std::cout << getDetail();
            std::cout << (getPassed() ? "[PASS] " : "[FAIL] ") << "Layer " << command.back() << " validation " << (getPassed() ? "passed\n" : "failed\n");
            return getPassed() ? 0 : 1;
        }
    }

    if (command == "layer5" && countPositionalArgs(argc, argv) >= 2) {
        spz::VerifyResult result;
        std::vector<uint8_t> spzData;
        auto readBytes = [](const std::string& path, std::vector<uint8_t>& out) {
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file) return false;
            auto size = file.tellg();
            file.seekg(0, std::ios::beg);
            out.resize(static_cast<size_t>(size));
            return file.read(reinterpret_cast<char*>(out.data()), size).good();
        };
        if (!readBytes(argv[2], spzData)) {
            std::cerr << "[ERROR] Cannot open SPZ file: " << argv[2] << "\n";
            return 1;
        }
        result.layer5_passed = verifier.verify_layer5(spzData, result.layer5_detail);
        std::cout << result.layer5_detail;
        std::cout << (result.layer5_passed ? "[PASS] Layer 5 validation passed\n" : "[FAIL] Layer 5 validation failed\n");
        return result.layer5_passed ? 0 : 1;
    }

    if ((command == "all" || command == "verify") && countPositionalArgs(argc, argv) >= 3) {
        // 提取 --report <path>（可选）
        const std::string reportPath = extractReportPath(argc, argv, 1);

        // 找到第2个位置参数（glb 路径）
        int posCount = 0;
        std::string spzPath, glbPath;
        for (int i = 2; i < argc; ++i) {
            if (std::string(argv[i]).find("--") == 0) { ++i; continue; }
            if (posCount == 0) { spzPath = argv[i]; posCount++; continue; }
            if (posCount == 1) { glbPath = argv[i]; break; }
        }

        auto result = verifier.verify_files(spzPath, glbPath);

        // 可选报告验证
        result.report_passed = verifier.verify_report_file(reportPath, result.report_detail);

        std::cout << result.layer1_detail << "\n";
        std::cout << result.layer2_detail << "\n";
        std::cout << result.layer3_detail << "\n";
        std::cout << result.layer4_detail << "\n";
        std::cout << result.layer5_detail << "\n";
        std::cout << result.report_detail << "\n";
        printSummary(result);
        if (result.all_passed()) {
            std::cout << "[SUCCESS] All 5 layers validation passed\n";
            return 0;
        }
        std::cout << "[FAILED] Some layer validations failed\n";
        return 1;
    }

    printUsage(argv[0]);
    return 1;
}

// Copyright (c) 2026 Pu Junhan
// SPDX-License-Identifier: MIT
// Project: SPZ-ecosystem
// Repository: https://github.com/spz-ecosystem/spz2glb

#include "spz_verifier.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

void printUsage(const char* progName) {
    std::cout << "SPZ to GLB Verification Tool\n";
    std::cout << "Usage: " << progName << " <command> [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  layer1 <glb>           - Validate GLB contract structure (Layer 1)\n";
    std::cout << "  layer2 <spz> <glb>     - Payload extraction & byte equality (Layer 2)\n";
    std::cout << "  layer3 <spz> <glb>     - Decoding consistency & v4 checks (Layer 3)\n";
    std::cout << "  all <spz> <glb>        - Run all three layers\n";
    std::cout << "  verify <spz> <glb>     - Alias for 'all'\n";
}

void printSummary(const spz::VerifyResult& result) {
    std::cout << "============================================================\n";
    std::cout << "Summary:\n";
    std::cout << "  Layer 1 (GLB Structure): " << (result.layer1_passed ? "PASSED" : "FAILED") << "\n";
    std::cout << "  Layer 2 (Binary Lossless): " << (result.layer2_passed ? "PASSED" : "FAILED") << "\n";
    std::cout << "  Layer 3 (Decoding): " << (result.layer3_passed ? "PASSED" : "FAILED") << "\n";
    std::cout << "============================================================\n";
}

bool runLayer1(spz::Verifier& verifier, const std::string& glbPath) {
    std::vector<uint8_t> glbBytes;

    std::ifstream file(glbPath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "[ERROR] Cannot open GLB file: " << glbPath << "\n";
        return false;
    }
    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    glbBytes.resize(static_cast<size_t>(size));
    if (!glbBytes.empty() && !file.read(reinterpret_cast<char*>(glbBytes.data()), size)) {
        std::cerr << "[ERROR] Failed to read GLB file: " << glbPath << "\n";
        return false;
    }

    std::string detail;
    const bool ok = verifier.verify_layer1(glbBytes, detail);
    std::cout << detail;
    std::cout << (ok ? "[PASS] Layer 1 validation passed\n" : "[FAIL] Layer 1 validation failed\n");
    return ok;
}

} // namespace

int main(int argc, char** argv) {
    // CLI 只负责参数分发与输出；三层规则全部下沉到 Verifier，避免再次分叉。
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string command = argv[1];
    spz::Verifier verifier;

    if (command == "layer1" && argc >= 3) {
        return runLayer1(verifier, argv[2]) ? 0 : 1;
    }

    if (command == "layer2" && argc >= 4) {
        const auto result = verifier.verify_files(argv[2], argv[3]);
        std::cout << result.layer2_detail;
        std::cout << (result.layer2_passed ? "[PASS] Layer 2 validation passed\n" : "[FAIL] Layer 2 validation failed\n");
        return result.layer2_passed ? 0 : 1;
    }

    if (command == "layer3" && argc >= 4) {
        const auto result = verifier.verify_files(argv[2], argv[3]);
        std::cout << result.layer3_detail;
        std::cout << (result.layer3_passed ? "[PASS] Layer 3 validation passed\n" : "[FAIL] Layer 3 validation failed\n");
        return result.layer3_passed ? 0 : 1;
    }

    if ((command == "all" || command == "verify") && argc >= 4) {
        const auto result = verifier.verify_files(argv[2], argv[3]);
        std::cout << result.layer1_detail << "\n";
        std::cout << result.layer2_detail << "\n";
        std::cout << result.layer3_detail << "\n";
        printSummary(result);
        if (result.all_passed()) {
            std::cout << "[SUCCESS] All 3 layers validation passed\n";
            return 0;
        }
        std::cout << "[FAILED] Some layer validations failed\n";
        return 1;
    }

    printUsage(argv[0]);
    return 1;
}

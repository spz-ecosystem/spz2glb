// Copyright (c) 2026 Pu Junhan
// SPDX-License-Identifier: MIT
// Project: SPZ-ecosystem
// Repository: https://github.com/spz-ecosystem/spz2glb
// Cultural Note: Huangdi Era 4723, Year of the Red Fire Horse (Bingwu, 丙午)

#include "spz_verifier.h"

#include <fstream>
#include <iostream>
#include <string>

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
    std::cout << "    verify <spz> <glb>     - Alias for 'all'\n";
}

void printSummary(const spz::VerifyResult& result) {
    std::cout << "============================================================\n";
    std::cout << "Summary:\n";
    std::cout << "  Layer 1 (GLB Structure & KHR Extension): " << (result.layer1_passed ? "PASSED" : "FAILED") << "\n";
    std::cout << "  Layer 2 (Binary Lossless): " << (result.layer2_passed ? "PASSED" : "FAILED") << "\n";
    std::cout << "  Layer 3 (Decoding): " << (result.layer3_passed ? "PASSED" : "FAILED") << "\n";
    std::cout << "  Layer 4 (Metadata): " << (result.layer4_passed ? "PASSED" : "FAILED") << "\n";
    std::cout << "  Layer 5 (ILV Extensions): " << (result.layer5_passed ? "PASSED" : "FAILED") << "\n";
    std::cout << "============================================================\n";
}

bool runLayer1(spz::Verifier& verifier, const std::string& glbPath) {
    std::string detail;
    const bool ok = verifier.verify_layer1_file(glbPath, detail);
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

    if (command == "layer4" && argc >= 4) {
        const auto result = verifier.verify_files(argv[2], argv[3]);
        std::cout << result.layer4_detail;
        std::cout << (result.layer4_passed ? "[PASS] Layer 4 validation passed\n" : "[FAIL] Layer 4 validation failed\n");
        return result.layer4_passed ? 0 : 1;
    }

    if (command == "layer5" && argc >= 3) {
        spz::VerifyResult result;
        std::vector<uint8_t> spzData;
        // L5 only needs SPZ data
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

    if ((command == "all" || command == "verify") && argc >= 4) {
        const auto result = verifier.verify_files(argv[2], argv[3]);
        std::cout << result.layer1_detail << "\n";
        std::cout << result.layer2_detail << "\n";
        std::cout << result.layer3_detail << "\n";
        std::cout << result.layer4_detail << "\n";
        std::cout << result.layer5_detail << "\n";
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

// Copyright (c) 2026 Pu Junhan
// SPDX-License-Identifier: MIT
// Project: SPZ-ecosystem
// Repository: https://github.com/spz-ecosystem/spz2glb
// Cultural Note: Huangdi Era 4723, Year of the Red Fire Horse (Bingwu, 丙午)
//
// SPZ Verifier - Three-layer verification for SPZ to GLB conversion

#ifndef SPZ_VERIFIER_H
#define SPZ_VERIFIER_H

#include <cstdint>
#include <string>
#include <vector>

namespace spz {

struct VerifyResult {
    bool layer1_passed;
    bool layer2_passed;
    bool layer3_passed;
    bool layer4_passed;  // GLB metadata ↔ SPZ header 一致性
    bool layer5_passed;  // ILV 扩展完整性
    std::string layer1_detail;
    std::string layer2_detail;
    std::string layer3_detail;
    std::string layer4_detail;
    std::string layer5_detail;

    bool all_passed() const {
        return layer1_passed && layer2_passed && layer3_passed &&
               layer4_passed && layer5_passed;
    }
};

class Verifier {
public:
    Verifier() = default;

    // 三层总入口：按固定顺序执行 L1/L2/L3，供 CLI 与测试共享。
    VerifyResult verify(const std::vector<uint8_t>& spz_data,
                        const std::vector<uint8_t>& glb_data);

    // 文件入口：负责读盘后转到内存入口，避免重复实现读取逻辑。
    VerifyResult verify_files(const std::string& spz_path,
                              const std::string& glb_path);

    // L1：仅验证 GLB 契约与结构完整性，不做字节对比。
    bool verify_layer1(const std::vector<uint8_t>& glb_data, std::string& detail);
    // L1 文件入口：用于 CLI 直接按路径执行单层校验。
    bool verify_layer1_file(const std::string& glb_path, std::string& detail);
    // L2：验证从 GLB 抽取出的 SPZ payload 与输入 SPZ 字节级一致。
    bool verify_layer2(const std::vector<uint8_t>& spz_data,
                       const std::vector<uint8_t>& glb_data,
                       std::string& detail);
    // L3：在 L1/L2 基础上校验解码一致性与 v4 头/尾可跳过语义。
    bool verify_layer3(const std::vector<uint8_t>& spz_data,
                       const std::vector<uint8_t>& glb_data,
                       std::string& detail);
    // L4：GLB extensions 元数据与 SPZ header 一致性校验。
    bool verify_layer4(const std::vector<uint8_t>& spz_data,
                       const std::vector<uint8_t>& glb_data,
                       std::string& detail);
    // L5：ILV 扩展完整性校验（可解析 + 003 值域 [0,16]）。
    bool verify_layer5(const std::vector<uint8_t>& spz_data,
                       std::string& detail);

private:
    bool layer1_validate_glb_structure(const std::vector<uint8_t>& glb_data,
                                       std::string& detail);

    bool layer2_verify_lossless(const std::vector<uint8_t>& spz_data,
                                const std::vector<uint8_t>& glb_data,
                                std::string& detail);

    bool layer3_verify_decoding(const std::vector<uint8_t>& spz_data,
                                const std::vector<uint8_t>& glb_data,
                                std::string& detail);
    // L4 实现：解析 GLB JSON 中的 KHR_gaussian_splatting_compression_spz_2 扩展字段
    // 与 SPZ header 中的 version/compression 对比
    bool layer4_verify_metadata(const std::vector<uint8_t>& spz_data,
                                const std::vector<uint8_t>& glb_data,
                                std::string& detail);
    // L5 实现：扫描 SPZ v4 header zone ILV 记录, 验证可解析性 + 003 值域
    bool layer5_verify_extensions(const std::vector<uint8_t>& spz_data,
                                  std::string& detail);
};

} // namespace spz

#endif // SPZ_VERIFIER_H

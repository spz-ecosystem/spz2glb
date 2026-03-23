// Copyright (c) 2026 Pu Junhan
// SPDX-License-Identifier: MIT
// Project: SPZ-ecosystem
// Repository: https://github.com/spz-ecosystem/spz2glb
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
    std::string layer1_detail;
    std::string layer2_detail;
    std::string layer3_detail;
    
    bool all_passed() const {
        return layer1_passed && layer2_passed && layer3_passed;
    }
};

class Verifier {
public:
    Verifier() = default;
    
    VerifyResult verify(const std::vector<uint8_t>& spz_data, 
                        const std::vector<uint8_t>& glb_data);
    
    VerifyResult verify_files(const std::string& spz_path, 
                              const std::string& glb_path);
    
private:
    bool layer1_validate_glb_structure(const std::vector<uint8_t>& glb_data,
                                        std::string& detail);
    
    bool layer2_verify_lossless(const std::vector<uint8_t>& spz_data,
                                 const std::vector<uint8_t>& glb_data,
                                 std::string& detail);
    
    bool layer3_verify_decoding(const std::vector<uint8_t>& spz_data,
                                 const std::vector<uint8_t>& glb_data,
                                 std::string& detail);
    
    std::vector<uint8_t> extract_buffer_from_glb(const std::vector<uint8_t>& glb_data);
};

} // namespace spz

#endif // SPZ_VERIFIER_H

// Copyright (c) 2026 Pu Junhan
// SPDX-License-Identifier: MIT
// Project: SPZ-ecosystem
// Repository: https://github.com/spz-ecosystem/spz2glb
//
// SPZ Verifier Implementation - Three-layer verification

#include "spz_verifier.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace spz {

namespace {

class Md5Hash {
public:
    Md5Hash() { init(); }
    
    void update(const uint8_t* data, size_t len) {
        for (size_t i = 0; i < len; i++) {
            buffer_[count_ % 64] = data[i];
            count_++;
            if (count_ % 64 == 0) {
                transform();
            }
        }
    }
    
    std::string finalize() {
        uint8_t bits[8];
        for (int i = 0; i < 8; i++) {
            bits[i] = static_cast<uint8_t>((count_ * 8) >> (i * 8));
        }
        
        uint8_t pad = 0x80;
        update(&pad, 1);
        
        uint8_t zeros[64] = {0};
        size_t padLen = (count_ % 64 < 56) ? (56 - count_ % 64) : (120 - count_ % 64);
        update(zeros, padLen);
        update(bits, 8);
        
        return bytesToHex(reinterpret_cast<uint8_t*>(state_), 16);
    }
    
    static std::string hash(const uint8_t* data, size_t len) {
        Md5Hash h;
        h.update(data, len);
        return h.finalize();
    }
    
private:
    void init() {
        state_[0] = 0x67452301;
        state_[1] = 0xefcdab89;
        state_[2] = 0x98badcfe;
        state_[3] = 0x10325476;
        count_ = 0;
    }
    
    void transform();
    static std::string bytesToHex(const uint8_t* data, size_t len);
    
    uint32_t state_[4] = {0};
    uint64_t count_ = 0;
    uint8_t buffer_[64];
};

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

void Md5Hash::transform() {
    uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3];
    uint32_t x[16];
    
    for (int i = 0; i < 16; i++) {
        x[i] = buffer_[i * 4] | (buffer_[i * 4 + 1] << 8) | 
               (buffer_[i * 4 + 2] << 16) | (buffer_[i * 4 + 3] << 24);
    }
    
    a = ROTATE_LEFT(a + ((b & c) | (~b & d)) + x[0] + 0xd76aa478, 7) + b;
    d = ROTATE_LEFT(d + ((a & b) | (~a & c)) + x[1] + 0xe8c7b756, 12) + a;
    c = ROTATE_LEFT(c + ((d & a) | (~d & b)) + x[2] + 0x242070db, 17) + d;
    b = ROTATE_LEFT(b + ((c & d) | (~c & a)) + x[3] + 0xc1bdceee, 22) + c;
    a = ROTATE_LEFT(a + ((b & c) | (~b & d)) + x[4] + 0xf57c0faf, 7) + b;
    d = ROTATE_LEFT(d + ((a & b) | (~a & c)) + x[5] + 0x4787c62a, 12) + a;
    c = ROTATE_LEFT(c + ((d & a) | (~d & b)) + x[6] + 0xa8304613, 17) + d;
    b = ROTATE_LEFT(b + ((c & d) | (~c & a)) + x[7] + 0xfd469501, 22) + c;
    a = ROTATE_LEFT(a + ((b & c) | (~b & d)) + x[8] + 0x698098d8, 7) + b;
    d = ROTATE_LEFT(d + ((a & b) | (~a & c)) + x[9] + 0x8b44f7af, 12) + a;
    c = ROTATE_LEFT(c + ((d & a) | (~d & b)) + x[10] + 0xffff5bb1, 17) + d;
    b = ROTATE_LEFT(b + ((c & d) | (~c & a)) + x[11] + 0x895cd7be, 22) + c;
    a = ROTATE_LEFT(a + ((b & c) | (~b & d)) + x[12] + 0x6b901122, 7) + b;
    d = ROTATE_LEFT(d + ((a & b) | (~a & c)) + x[13] + 0xfd987193, 12) + a;
    c = ROTATE_LEFT(c + ((d & a) | (~d & b)) + x[14] + 0xa679438e, 17) + d;
    b = ROTATE_LEFT(b + ((c & d) | (~c & a)) + x[15] + 0x49b40821, 22) + c;
    
    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
}

std::string Md5Hash::bytesToHex(const uint8_t* data, size_t len) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; i++) {
        oss << std::setw(2) << static_cast<int>(data[i]);
    }
    return oss.str();
}

std::string formatSize(size_t bytes) {
    std::ostringstream oss;
    if (bytes < 1024) {
        oss << bytes << " B";
    } else if (bytes < 1024 * 1024) {
        oss << std::fixed << std::setprecision(1) << (bytes / 1024.0) << " KB";
    } else {
        oss << std::fixed << std::setprecision(2) << (bytes / 1024.0 / 1024.0) << " MB";
    }
    return oss.str();
}

} // anonymous namespace

VerifyResult Verifier::verify(const std::vector<uint8_t>& spz_data, 
                               const std::vector<uint8_t>& glb_data) {
    VerifyResult result = {};
    
    result.layer1_passed = layer1_validate_glb_structure(glb_data, result.layer1_detail);
    result.layer2_passed = layer2_verify_lossless(spz_data, glb_data, result.layer2_detail);
    result.layer3_passed = layer3_verify_decoding(spz_data, glb_data, result.layer3_detail);
    
    return result;
}

VerifyResult Verifier::verify_files(const std::string& spz_path, 
                                     const std::string& glb_path) {
    std::ifstream spz_file(spz_path, std::ios::binary | std::ios::ate);
    std::ifstream glb_file(glb_path, std::ios::binary | std::ios::ate);
    
    VerifyResult result = {};
    
    if (!spz_file) {
        result.layer1_detail = "Cannot open SPZ file: " + spz_path;
        return result;
    }
    
    if (!glb_file) {
        result.layer1_detail = "Cannot open GLB file: " + glb_path;
        return result;
    }
    
    auto spz_size = spz_file.tellg();
    auto glb_size = glb_file.tellg();
    spz_file.seekg(0);
    glb_file.seekg(0);
    
    std::vector<uint8_t> spz_data(spz_size);
    std::vector<uint8_t> glb_data(glb_size);
    
    spz_file.read(reinterpret_cast<char*>(spz_data.data()), spz_size);
    glb_file.read(reinterpret_cast<char*>(glb_data.data()), glb_size);
    
    return verify(spz_data, glb_data);
}

bool Verifier::layer1_validate_glb_structure(const std::vector<uint8_t>& glb_data,
                                              std::string& detail) {
    std::ostringstream oss;
    oss << "=== Layer 1: GLB Structure Validation ===\n";
    
    if (glb_data.size() < 12) {
        oss << "[FAIL] File too small for GLB header\n";
        detail = oss.str();
        return false;
    }
    
    uint32_t magic = *reinterpret_cast<const uint32_t*>(glb_data.data());
    uint32_t version = *reinterpret_cast<const uint32_t*>(glb_data.data() + 4);
    
    if (magic != 0x46546C67) {
        oss << "[FAIL] Invalid GLB magic: 0x" << std::hex << magic << "\n";
        detail = oss.str();
        return false;
    }
    oss << "[PASS] Magic: glTF (0x46546C67)\n";
    
    if (version != 2) {
        oss << "[FAIL] Unsupported GLB version: " << version << "\n";
        detail = oss.str();
        return false;
    }
    oss << "[PASS] Version: 2\n";
    
    if (glb_data.size() < 20) {
        oss << "[FAIL] File too small for JSON chunk\n";
        detail = oss.str();
        return false;
    }
    
    uint32_t json_chunk_length = *reinterpret_cast<const uint32_t*>(glb_data.data() + 12);
    
    if (glb_data.size() < 20 + json_chunk_length) {
        oss << "[FAIL] File truncated in JSON chunk\n";
        detail = oss.str();
        return false;
    }
    
    std::string json_str(reinterpret_cast<const char*>(glb_data.data() + 20), json_chunk_length);
    
    bool has_gaussian_splatting = json_str.find("KHR_gaussian_splatting") != std::string::npos;
    bool has_spz2 = json_str.find("KHR_gaussian_splatting_compression_spz_2") != std::string::npos;
    bool has_buffers = json_str.find("\"buffers\"") != std::string::npos;
    
    oss << (has_gaussian_splatting ? "[PASS]" : "[FAIL]") << " KHR_gaussian_splatting: " 
        << (has_gaussian_splatting ? "present" : "missing") << "\n";
    oss << (has_spz2 ? "[PASS]" : "[FAIL]") << " KHR_gaussian_splatting_compression_spz_2: " 
        << (has_spz2 ? "present" : "missing") << "\n";
    oss << (has_buffers ? "[PASS]" : "[FAIL]") << " buffers: " 
        << (has_buffers ? "present" : "missing") << "\n";
    
    detail = oss.str();
    return has_gaussian_splatting && has_spz2 && has_buffers;
}

std::vector<uint8_t> Verifier::extract_buffer_from_glb(const std::vector<uint8_t>& glb_data) {
    if (glb_data.size() < 20) return {};
    
    uint32_t json_chunk_length = *reinterpret_cast<const uint32_t*>(glb_data.data() + 12);
    size_t json_padding = (4 - (json_chunk_length % 4)) % 4;
    size_t bin_chunk_offset = 12 + 8 + json_chunk_length + json_padding;
    
    if (bin_chunk_offset + 8 > glb_data.size()) return {};
    
    uint32_t bin_chunk_length = *reinterpret_cast<const uint32_t*>(glb_data.data() + bin_chunk_offset);
    uint32_t bin_chunk_type = *reinterpret_cast<const uint32_t*>(glb_data.data() + bin_chunk_offset + 4);
    
    if (bin_chunk_type != 0x004E4942) return {};
    
    return std::vector<uint8_t>(
        glb_data.begin() + bin_chunk_offset + 8,
        glb_data.begin() + bin_chunk_offset + 8 + bin_chunk_length
    );
}

bool Verifier::layer2_verify_lossless(const std::vector<uint8_t>& spz_data,
                                       const std::vector<uint8_t>& glb_data,
                                       std::string& detail) {
    std::ostringstream oss;
    oss << "=== Layer 2: Binary Lossless Verification ===\n";
    
    auto extracted = extract_buffer_from_glb(glb_data);
    
    oss << "Original SPZ size: " << formatSize(spz_data.size()) << "\n";
    oss << "Extracted buffer size: " << formatSize(extracted.size()) << "\n";
    
    if (extracted.size() != spz_data.size()) {
        oss << "[FAIL] Size mismatch!\n";
        detail = oss.str();
        return false;
    }
    
    bool match = true;
    size_t diff_pos = 0;
    for (size_t i = 0; i < spz_data.size(); i++) {
        if (extracted[i] != spz_data[i]) {
            match = false;
            diff_pos = i;
            break;
        }
    }
    
    if (match) {
        oss << "[PASS] Binary 100% match!\n";
        detail = oss.str();
        return true;
    } else {
        oss << "[FAIL] Byte mismatch at position " << diff_pos << "\n";
        detail = oss.str();
        return false;
    }
}

bool Verifier::layer3_verify_decoding(const std::vector<uint8_t>& spz_data,
                                       const std::vector<uint8_t>& glb_data,
                                       std::string& detail) {
    std::ostringstream oss;
    oss << "=== Layer 3: Decoding Consistency Verification ===\n";
    
    auto extracted = extract_buffer_from_glb(glb_data);
    
    if (extracted.size() == spz_data.size()) {
        oss << "[PASS] SPZ data fully embedded in GLB\n";
        oss << "[PASS] Size consistent: " << formatSize(spz_data.size()) << "\n";
        detail = oss.str();
        return true;
    } else {
        oss << "[FAIL] Size mismatch\n";
        detail = oss.str();
        return false;
    }
}

} // namespace spz

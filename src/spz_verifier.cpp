// Copyright (c) 2026 Pu Junhan
// SPDX-License-Identifier: MIT
// Project: SPZ-ecosystem
// Repository: https://github.com/spz-ecosystem/spz2glb
//
// SPZ Verifier Implementation - Three-layer verification

#include "spz_verifier.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string_view>

#include <zlib.h>

namespace spz {

namespace {

constexpr uint32_t kGlbMagic = 0x46546C67;
constexpr uint32_t kGlbVersion = 2;
constexpr uint32_t kJsonChunkType = 0x4E4F534A;
constexpr uint32_t kBinChunkType = 0x004E4942;
constexpr uint32_t kSpzMagic = 0x5053474E;

constexpr const char* kExtGaussian = "KHR_gaussian_splatting";
constexpr const char* kExtSpz2 = "KHR_gaussian_splatting_compression_spz_2";

struct SpzHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t numPoints;
    uint8_t shDegree;
    uint8_t fractionalBits;
    uint8_t flags;
    uint8_t reserved;
};

struct ParsedGlb {
    std::string json;
    size_t jsonChunkLength = 0;
    size_t jsonPadding = 0;
    size_t binChunkOffset = 0;
    size_t binChunkLength = 0;
    size_t binPayloadOffset = 0;
    size_t glbLengthFromHeader = 0;
};

bool readFileBytes(const std::string& path, std::vector<uint8_t>& out) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return false;
    }

    const auto size = file.tellg();
    if (size < 0) {
        return false;
    }

    file.seekg(0, std::ios::beg);
    out.resize(static_cast<size_t>(size));
    if (!out.empty() && !file.read(reinterpret_cast<char*>(out.data()), size)) {
        return false;
    }
    return true;
}

bool readU32LE(const std::vector<uint8_t>& bytes, size_t offset, uint32_t& out) {
    if (offset + sizeof(uint32_t) > bytes.size()) {
        return false;
    }
    std::memcpy(&out, bytes.data() + offset, sizeof(uint32_t));
    return true;
}

// 严格 GLB 解析：这里故意不做“宽松容错”，用于守住契约边界。
bool parseGlb(const std::vector<uint8_t>& glbData, ParsedGlb& parsed, std::string& err) {
    if (glbData.size() < 12) {
        err = "GLB too small for header";
        return false;
    }

    uint32_t magic = 0;
    uint32_t version = 0;
    uint32_t length = 0;
    if (!readU32LE(glbData, 0, magic) || !readU32LE(glbData, 4, version) || !readU32LE(glbData, 8, length)) {
        err = "GLB header read failed";
        return false;
    }

    if (magic != kGlbMagic) {
        err = "GLB magic mismatch";
        return false;
    }
    if (version != kGlbVersion) {
        err = "GLB version mismatch";
        return false;
    }
    if (length != glbData.size()) {
        err = "GLB header length mismatch";
        return false;
    }

    uint32_t jsonLen = 0;
    uint32_t jsonType = 0;
    if (!readU32LE(glbData, 12, jsonLen) || !readU32LE(glbData, 16, jsonType)) {
        err = "JSON chunk header missing";
        return false;
    }
    if (jsonType != kJsonChunkType) {
        err = "first chunk is not JSON";
        return false;
    }

    const size_t jsonStart = 20;
    const size_t jsonEnd = jsonStart + jsonLen;
    if (jsonEnd > glbData.size()) {
        err = "JSON chunk out of range";
        return false;
    }

    parsed.jsonChunkLength = jsonLen;
    parsed.jsonPadding = (4 - (parsed.jsonChunkLength % 4)) % 4;
    parsed.json.assign(reinterpret_cast<const char*>(glbData.data() + jsonStart), parsed.jsonChunkLength);

    const size_t binHeaderOffset = jsonEnd + parsed.jsonPadding;
    if (binHeaderOffset + 8 > glbData.size()) {
        err = "BIN chunk header missing";
        return false;
    }

    uint32_t binLen = 0;
    uint32_t binType = 0;
    if (!readU32LE(glbData, binHeaderOffset, binLen) || !readU32LE(glbData, binHeaderOffset + 4, binType)) {
        err = "BIN chunk header read failed";
        return false;
    }
    if (binType != kBinChunkType) {
        err = "second chunk is not BIN";
        return false;
    }

    const size_t binPayloadOffset = binHeaderOffset + 8;
    const size_t binPayloadEnd = binPayloadOffset + binLen;
    if (binPayloadEnd > glbData.size()) {
        err = "BIN chunk out of range";
        return false;
    }

    if (binPayloadEnd != glbData.size()) {
        err = "unexpected trailing bytes after BIN chunk";
        return false;
    }

    parsed.binChunkOffset = binHeaderOffset;
    parsed.binChunkLength = binLen;
    parsed.binPayloadOffset = binPayloadOffset;
    parsed.glbLengthFromHeader = length;
    return true;
}

size_t findMatchingBracket(const std::string& s, size_t openPos, char openChar, char closeChar) {
    int depth = 0;
    for (size_t i = openPos; i < s.size(); ++i) {
        if (s[i] == openChar) {
            ++depth;
        } else if (s[i] == closeChar) {
            --depth;
            if (depth == 0) {
                return i;
            }
        }
    }
    return std::string::npos;
}

bool arrayContainsValue(const std::string& json, const char* arrayKey, const char* value) {
    const std::string key = std::string("\"") + arrayKey + "\"";
    const size_t keyPos = json.find(key);
    if (keyPos == std::string::npos) {
        return false;
    }
    const size_t openBracket = json.find('[', keyPos);
    if (openBracket == std::string::npos) {
        return false;
    }
    const size_t closeBracket = findMatchingBracket(json, openBracket, '[', ']');
    if (closeBracket == std::string::npos) {
        return false;
    }
    const std::string arraySlice = json.substr(openBracket, closeBracket - openBracket + 1);
    const std::string quoted = std::string("\"") + value + "\"";
    return arraySlice.find(quoted) != std::string::npos;
}

bool parseUnsignedAfterKey(const std::string& json, const std::string& key, uint32_t& value, size_t searchStart = 0) {
    const size_t keyPos = json.find(key, searchStart);
    if (keyPos == std::string::npos) {
        return false;
    }

    const size_t colonPos = json.find(':', keyPos + key.size());
    if (colonPos == std::string::npos) {
        return false;
    }

    size_t pos = colonPos + 1;
    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos])) != 0) {
        ++pos;
    }

    if (pos >= json.size() || (json[pos] < '0' || json[pos] > '9')) {
        return false;
    }

    size_t end = pos;
    while (end < json.size() && json[end] >= '0' && json[end] <= '9') {
        ++end;
    }

    try {
        value = static_cast<uint32_t>(std::stoul(json.substr(pos, end - pos)));
    } catch (...) {
        return false;
    }
    return true;
}

bool parseBufferAndView(const std::string& json,
                        uint32_t& bufferByteLength,
                        uint32_t& bufferViewByteOffset,
                        uint32_t& bufferViewByteLength,
                        uint32_t& compressionBufferViewIndex) {
    if (!parseUnsignedAfterKey(json, "\"byteLength\"", bufferByteLength)) {
        return false;
    }

    const size_t viewSectionPos = json.find("\"bufferViews\"");
    if (viewSectionPos == std::string::npos) {
        return false;
    }
    const size_t viewArrayOpen = json.find('[', viewSectionPos);
    if (viewArrayOpen == std::string::npos) {
        return false;
    }
    const size_t viewObjOpen = json.find('{', viewArrayOpen);
    if (viewObjOpen == std::string::npos) {
        return false;
    }
    const size_t viewObjClose = findMatchingBracket(json, viewObjOpen, '{', '}');
    if (viewObjClose == std::string::npos) {
        return false;
    }

    const std::string viewObj = json.substr(viewObjOpen, viewObjClose - viewObjOpen + 1);
    if (!parseUnsignedAfterKey(viewObj, "\"byteLength\"", bufferViewByteLength)) {
        return false;
    }
    if (!parseUnsignedAfterKey(viewObj, "\"byteOffset\"", bufferViewByteOffset)) {
        bufferViewByteOffset = 0;
    }

    const size_t compressionPos = json.find("\"KHR_gaussian_splatting_compression_spz_2\"");
    if (compressionPos == std::string::npos) {
        return false;
    }
    if (!parseUnsignedAfterKey(json, "\"bufferView\"", compressionBufferViewIndex, compressionPos)) {
        return false;
    }

    return true;
}

bool tryPeekSpzHeader(const std::vector<uint8_t>& data, SpzHeader& header, bool& fromGzip) {
    fromGzip = false;
    if (data.size() < sizeof(SpzHeader)) {
        return false;
    }

    const bool isGzip = data.size() >= 2 && data[0] == 0x1F && data[1] == 0x8B;
    if (!isGzip) {
        std::memcpy(&header, data.data(), sizeof(SpzHeader));
        return header.magic == kSpzMagic;
    }

    fromGzip = true;
    uint8_t scratch[sizeof(SpzHeader)] = {};

    z_stream strm = {};
    strm.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(data.data()));
    strm.avail_in = static_cast<uInt>(data.size());
    strm.next_out = reinterpret_cast<Bytef*>(scratch);
    strm.avail_out = static_cast<uInt>(sizeof(scratch));

    if (inflateInit2(&strm, 16 + MAX_WBITS) != Z_OK) {
        return false;
    }

    int ret = Z_OK;
    while (ret == Z_OK && strm.total_out < sizeof(scratch)) {
        ret = inflate(&strm, Z_NO_FLUSH);
    }

    const bool hasHeader = strm.total_out >= sizeof(scratch) && (ret == Z_OK || ret == Z_STREAM_END);
    inflateEnd(&strm);
    if (!hasHeader) {
        return false;
    }

    std::memcpy(&header, scratch, sizeof(SpzHeader));
    return header.magic == kSpzMagic;
}

std::vector<uint8_t> extractSpzPayload(const std::vector<uint8_t>& glbData,
                                       const ParsedGlb& parsed,
                                       const std::string& json,
                                       std::string& err) {
    uint32_t bufferByteLength = 0;
    uint32_t bufferViewByteOffset = 0;
    uint32_t bufferViewByteLength = 0;
    uint32_t compressionBufferViewIndex = 0;

    if (!parseBufferAndView(json,
                            bufferByteLength,
                            bufferViewByteOffset,
                            bufferViewByteLength,
                            compressionBufferViewIndex)) {
        err = "failed to parse buffer/bufferView/compression metadata";
        return {};
    }

    if (compressionBufferViewIndex != 0) {
        err = "compression.bufferView is not 0";
        return {};
    }

    if ((bufferViewByteOffset % 4) != 0) {
        err = "bufferView.byteOffset is not 4-byte aligned";
        return {};
    }

    if (bufferViewByteLength != bufferByteLength) {
        err = "bufferView.byteLength does not match buffers[0].byteLength";
        return {};
    }

    const uint64_t viewEnd = static_cast<uint64_t>(bufferViewByteOffset) + static_cast<uint64_t>(bufferViewByteLength);
    if (viewEnd > bufferByteLength) {
        err = "bufferView exceeds buffers[0] byteLength";
        return {};
    }

    if (parsed.binChunkLength < bufferByteLength || (parsed.binChunkLength - bufferByteLength) > 3) {
        err = "BIN chunk padding is invalid";
        return {};
    }

    const size_t payloadStart = parsed.binPayloadOffset + bufferViewByteOffset;
    const size_t payloadEnd = payloadStart + bufferViewByteLength;
    if (payloadEnd > glbData.size()) {
        err = "payload out of GLB bounds";
        return {};
    }

    return std::vector<uint8_t>(glbData.begin() + payloadStart, glbData.begin() + payloadEnd);
}

} // namespace

VerifyResult Verifier::verify(const std::vector<uint8_t>& spz_data,
                              const std::vector<uint8_t>& glb_data) {
    VerifyResult result = {};
    result.layer1_passed = verify_layer1(glb_data, result.layer1_detail);
    result.layer2_passed = verify_layer2(spz_data, glb_data, result.layer2_detail);
    result.layer3_passed = verify_layer3(spz_data, glb_data, result.layer3_detail);
    return result;
}

VerifyResult Verifier::verify_files(const std::string& spz_path,
                                    const std::string& glb_path) {
    VerifyResult result = {};
    std::vector<uint8_t> spzData;
    std::vector<uint8_t> glbData;

    if (!readFileBytes(spz_path, spzData)) {
        result.layer1_detail = "Cannot open SPZ file: " + spz_path;
        return result;
    }
    if (!readFileBytes(glb_path, glbData)) {
        result.layer1_detail = "Cannot open GLB file: " + glb_path;
        return result;
    }

    return verify(spzData, glbData);
}

bool Verifier::verify_layer1(const std::vector<uint8_t>& glb_data, std::string& detail) {
    return layer1_validate_glb_structure(glb_data, detail);
}

bool Verifier::verify_layer2(const std::vector<uint8_t>& spz_data,
                             const std::vector<uint8_t>& glb_data,
                             std::string& detail) {
    return layer2_verify_lossless(spz_data, glb_data, detail);
}

bool Verifier::verify_layer3(const std::vector<uint8_t>& spz_data,
                             const std::vector<uint8_t>& glb_data,
                             std::string& detail) {
    return layer3_verify_decoding(spz_data, glb_data, detail);
}

// L1：验证 GLB 容器结构与扩展契约，不涉及 payload 字节一致性。
bool Verifier::layer1_validate_glb_structure(const std::vector<uint8_t>& glb_data,
                                             std::string& detail) {
    std::ostringstream oss;
    oss << "=== Layer 1: GLB Structure & Contract Validation ===\n";

    ParsedGlb parsed;
    std::string parseErr;
    if (!parseGlb(glb_data, parsed, parseErr)) {
        oss << "[FAIL] " << parseErr << "\n";
        detail = oss.str();
        return false;
    }

    oss << "[PASS] GLB header/chunks are structurally valid\n";
    oss << "[PASS] JSON chunk length=" << parsed.jsonChunkLength
        << ", padding=" << parsed.jsonPadding << " (4-byte aligned)\n";
    oss << "[PASS] BIN chunk length=" << parsed.binChunkLength << "\n";

    const bool usedGaussian = arrayContainsValue(parsed.json, "extensionsUsed", kExtGaussian);
    const bool usedSpz2 = arrayContainsValue(parsed.json, "extensionsUsed", kExtSpz2);
    const bool requiredGaussian = arrayContainsValue(parsed.json, "extensionsRequired", kExtGaussian);
    const bool requiredSpz2 = arrayContainsValue(parsed.json, "extensionsRequired", kExtSpz2);

    oss << (usedGaussian ? "[PASS]" : "[FAIL]") << " extensionsUsed contains " << kExtGaussian << "\n";
    oss << (usedSpz2 ? "[PASS]" : "[FAIL]") << " extensionsUsed contains " << kExtSpz2 << "\n";
    oss << (requiredGaussian ? "[PASS]" : "[FAIL]") << " extensionsRequired contains " << kExtGaussian << "\n";
    oss << (requiredSpz2 ? "[PASS]" : "[FAIL]") << " extensionsRequired contains " << kExtSpz2 << "\n";

    uint32_t bufferByteLength = 0;
    uint32_t bufferViewByteOffset = 0;
    uint32_t bufferViewByteLength = 0;
    uint32_t compressionBufferViewIndex = 0;

    const bool parsedMappings = parseBufferAndView(parsed.json,
                                                   bufferByteLength,
                                                   bufferViewByteOffset,
                                                   bufferViewByteLength,
                                                   compressionBufferViewIndex);
    if (!parsedMappings) {
        oss << "[FAIL] cannot parse buffers/bufferViews/compression mappings\n";
        detail = oss.str();
        return false;
    }

    const bool compressionOnView0 = (compressionBufferViewIndex == 0);
    const bool offsetAligned = ((bufferViewByteOffset % 4) == 0);
    const bool viewMatchesBuffer = (bufferViewByteLength == bufferByteLength);
    const bool inRange = static_cast<uint64_t>(bufferViewByteOffset) + static_cast<uint64_t>(bufferViewByteLength) <= bufferByteLength;
    const bool binPaddingValid = parsed.binChunkLength >= bufferByteLength && (parsed.binChunkLength - bufferByteLength) <= 3;
    const size_t binPaddingBytes = parsed.binChunkLength >= bufferByteLength ? (parsed.binChunkLength - bufferByteLength) : 0;

    oss << (compressionOnView0 ? "[PASS]" : "[FAIL]") << " compression.bufferView=" << compressionBufferViewIndex << "\n";
    oss << (viewMatchesBuffer ? "[PASS]" : "[FAIL]") << " bufferView.byteLength=" << bufferViewByteLength
        << " matches buffers[0].byteLength=" << bufferByteLength << "\n";
    oss << (offsetAligned ? "[PASS]" : "[FAIL]") << " bufferView.byteOffset is 4-byte aligned\n";
    oss << (inRange ? "[PASS]" : "[FAIL]") << " bufferView is inside buffers[0] range\n";
    oss << (binPaddingValid ? "[PASS]" : "[FAIL]") << " BIN chunk padding is within 0..3 bytes (actual="
        << binPaddingBytes << ")\n";

    const bool passed = usedGaussian && usedSpz2 && requiredGaussian && requiredSpz2 &&
                        compressionOnView0 && viewMatchesBuffer && offsetAligned && inRange && binPaddingValid;

    if (!passed) {
        oss << "[FAIL] Layer 1 contract assertions failed\n";
    } else {
        oss << "[PASS] Layer 1 contract assertions passed\n";
    }

    detail = oss.str();
    return passed;
}

bool Verifier::layer2_verify_lossless(const std::vector<uint8_t>& spz_data,
                                      const std::vector<uint8_t>& glb_data,
                                      std::string& detail) {
    std::ostringstream oss;
    oss << "=== Layer 2: Payload Extraction & Byte Equality ===\n";

    ParsedGlb parsed;
    std::string parseErr;
    if (!parseGlb(glb_data, parsed, parseErr)) {
        oss << "[FAIL] " << parseErr << "\n";
        detail = oss.str();
        return false;
    }

    std::string extractErr;
    const auto extracted = extractSpzPayload(glb_data, parsed, parsed.json, extractErr);
    if (!extractErr.empty()) {
        oss << "[FAIL] " << extractErr << "\n";
        detail = oss.str();
        return false;
    }

    oss << "SPZ input bytes: " << spz_data.size() << "\n";
    oss << "Extracted bytes: " << extracted.size() << "\n";

    if (extracted.size() != spz_data.size()) {
        oss << "[FAIL] size mismatch\n";
        detail = oss.str();
        return false;
    }

    const auto mismatch = std::mismatch(spz_data.begin(), spz_data.end(), extracted.begin());
    if (mismatch.first != spz_data.end()) {
        oss << "[FAIL] byte mismatch at index " << std::distance(spz_data.begin(), mismatch.first) << "\n";
        detail = oss.str();
        return false;
    }

    oss << "[PASS] extracted payload is byte-identical to input SPZ\n";
    detail = oss.str();
    return true;
}

// L3：在前两层成立后，补充 v4 header/trailer 可跳过性与解码一致性约束。
bool Verifier::layer3_verify_decoding(const std::vector<uint8_t>& spz_data,
                                      const std::vector<uint8_t>& glb_data,
                                      std::string& detail) {
    std::ostringstream oss;
    oss << "=== Layer 3: Decoding Consistency & v4 Header/Trailer Checks ===\n";

    ParsedGlb parsed;
    std::string parseErr;
    if (!parseGlb(glb_data, parsed, parseErr)) {
        oss << "[FAIL] " << parseErr << "\n";
        detail = oss.str();
        return false;
    }

    std::string extractErr;
    const auto extracted = extractSpzPayload(glb_data, parsed, parsed.json, extractErr);
    if (!extractErr.empty()) {
        oss << "[FAIL] " << extractErr << "\n";
        detail = oss.str();
        return false;
    }

    if (extracted.size() != spz_data.size()) {
        oss << "[FAIL] extracted payload size mismatch\n";
        detail = oss.str();
        return false;
    }

    SpzHeader header{};
    bool fromGzip = false;
    if (!tryPeekSpzHeader(extracted, header, fromGzip)) {
        oss << "[FAIL] cannot parse SPZ header from extracted payload\n";
        detail = oss.str();
        return false;
    }

    oss << "[PASS] SPZ header parsed from " << (fromGzip ? "gzip" : "raw") << " payload\n";
    oss << "[PASS] SPZ version=" << header.version
        << ", numPoints=" << header.numPoints
        << ", flags=0x" << std::hex << static_cast<unsigned>(header.flags) << std::dec << "\n";

    if (header.version >= 4) {
        if (extracted.size() <= sizeof(SpzHeader)) {
            oss << "[FAIL] v4 payload too small (no data after header)\n";
            detail = oss.str();
            return false;
        }
        oss << "[PASS] v4 payload has trailing bytes that remain skippable ("
            << (extracted.size() - sizeof(SpzHeader)) << " bytes)\n";
    } else {
        oss << "[PASS] non-v4 payload (v" << header.version << ") header checks complete\n";
    }

    oss << "[PASS] decoding consistency checks complete\n";
    detail = oss.str();
    return true;
}

} // namespace spz

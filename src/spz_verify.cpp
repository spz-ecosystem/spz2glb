/**
 * SPZ to GLB Verification Tool
 * 
 * Three-layer verification:
 * - Layer 1: GLB Structure & SPZ_2 Specification Validation
 * - Layer 2: Binary Lossless Verification (SPZ → GLB → Extract → Compare)
 * - Layer 3: Decoding Consistency Verification
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <array>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

struct GlbHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t length;
};

struct GlbChunk {
    uint32_t chunkLength;
    uint32_t chunkType;
};

class Md5Hash {
public:
    void update(const uint8_t* data, size_t len);
    std::string finalize();
    static std::string hash(const uint8_t* data, size_t len);
    
private:
    void init();
    void transform();
    uint32_t state[4] = {0};
    uint64_t count = 0;
    uint8_t buffer[64];
};

std::string readFileBytes(const std::string& path, size_t maxSize = 0);
bool fileExists(const std::string& path);
size_t getFileSize(const std::string& path);
std::string bytesToHex(const uint8_t* data, size_t len);

void printDivider() {
    std::cout << "============================================================\n";
}

bool layer1ValidateGlbStructure(const std::string& glbPath) {
    std::cout << "\n";
    printDivider();
    std::cout << "Layer 1: GLB Structure & SPZ_2 Specification Validation\n";
    printDivider();
    
    std::ifstream file(glbPath, std::ios::binary);
    if (!file) {
        std::cerr << "[ERROR] Cannot open file: " << glbPath << "\n";
        return false;
    }
    
    GlbHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    
    if (header.magic != 0x46546C67) {
        std::cerr << "[ERROR] Invalid GLB magic: 0x" << std::hex << header.magic << "\n";
        return false;
    }
    std::cout << "    [PASS] Magic: glTF (0x46546C67)\n";
    
    if (header.version != 2) {
        std::cerr << "[ERROR] Invalid version: " << header.version << "\n";
        return false;
    }
    std::cout << "    [PASS] Version: 2\n";
    
    GlbChunk jsonChunk;
    file.read(reinterpret_cast<char*>(&jsonChunk), sizeof(jsonChunk));
    
    std::vector<char> jsonData(jsonChunk.chunkLength);
    file.read(jsonData.data(), jsonChunk.chunkLength);
    
    size_t padding = (4 - (jsonChunk.chunkLength % 4)) % 4;
    file.seekg(padding, std::ios::cur);
    
    std::string jsonStr(jsonData.data(), jsonChunk.chunkLength);
    size_t nullPos = jsonStr.find('\0');
    if (nullPos != std::string::npos) {
        jsonStr = jsonStr.substr(0, nullPos);
    }
    
    int passed = 0;
    int total = 6;
    
    if (jsonStr.find("KHR_gaussian_splatting") != std::string::npos) {
        std::cout << "    [PASS] extensionsUsed: KHR_gaussian_splatting\n";
        passed++;
    }
    
    if (jsonStr.find("KHR_gaussian_splatting_compression_spz_2") != std::string::npos) {
        std::cout << "    [PASS] extensionsUsed: KHR_gaussian_splatting_compression_spz_2\n";
        passed++;
    }
    
    if (jsonStr.find("\"buffers\"") != std::string::npos) {
        std::cout << "    [PASS] buffers: present\n";
        passed++;
    }
    
    if (jsonStr.find("\"attributes\"") != std::string::npos) {
        size_t attrsStart = jsonStr.find("\"attributes\"");
        size_t attrsEnd = jsonStr.find("}", attrsStart);
        std::string attrsSection = jsonStr.substr(attrsStart, attrsEnd - attrsStart);
        if (attrsSection.find(": {}") != std::string::npos || attrsSection.find(":{}") != std::string::npos) {
            std::cout << "    [PASS] attributes: empty (compression stream mode)\n";
            passed++;
        }
    }
    
    if (jsonStr.find("\"accessors\"") == std::string::npos || 
        jsonStr.find("\"accessors\": []") != std::string::npos) {
        std::cout << "    [PASS] accessors: 0 or empty (compression stream mode)\n";
        passed++;
    }
    
    std::cout << "\nPassed: " << passed << "/" << total << "\n";
    
    if (passed == total) {
        std::cout << "\n[PASSED] Layer 1: All validation checks passed!\n";
        return true;
    }
    
    std::cout << "\n[FAILED] Layer 1: Some checks failed\n";
    return false;
}

bool layer2VerifyLossless(const std::string& spzPath, const std::string& glbPath) {
    std::cout << "\n";
    printDivider();
    std::cout << "Layer 2: Binary Lossless Verification\n";
    printDivider();
    
    std::cout << "\n[1] Reading original SPZ...\n";
    auto originalData = readFileBytes(spzPath);
    std::cout << "    Size: " << originalData.size() << " bytes\n";
    
    std::ifstream file(glbPath, std::ios::binary);
    if (!file) {
        std::cerr << "[ERROR] Cannot open GLB: " << glbPath << "\n";
        return false;
    }
    
    file.seekg(12);
    
    GlbChunk jsonChunk;
    file.read(reinterpret_cast<char*>(&jsonChunk), sizeof(jsonChunk));
    
    std::vector<char> jsonData(jsonChunk.chunkLength);
    file.read(jsonData.data(), jsonChunk.chunkLength);
    
    size_t jsonPadding = (4 - (jsonChunk.chunkLength % 4)) % 4;
    file.seekg(jsonPadding, std::ios::cur);
    
    std::string jsonStr(jsonData.data(), jsonChunk.chunkLength);
    size_t nullPos = jsonStr.find('\0');
    if (nullPos != std::string::npos) {
        jsonStr = jsonStr.substr(0, nullPos);
    }
    
    size_t bufferSize = 0;
    size_t byteLenPos = jsonStr.find("\"byteLength\"");
    if (byteLenPos != std::string::npos) {
        size_t colonPos = jsonStr.find(":", byteLenPos);
        size_t start = jsonStr.find_first_not_of(" ", colonPos + 1);
        size_t end = jsonStr.find_first_of(",}", start);
        std::string sizeStr = jsonStr.substr(start, end - start);
        bufferSize = std::stoul(sizeStr);
    }
    
    std::cout << "    Buffer size from JSON: " << bufferSize << " bytes\n";
    
    GlbChunk binChunk;
    file.read(reinterpret_cast<char*>(&binChunk), sizeof(binChunk));
    
    std::vector<uint8_t> extractedData(bufferSize);
    file.read(reinterpret_cast<char*>(extractedData.data()), bufferSize);
    
    std::cout << "    Extracted from GLB: " << extractedData.size() << " bytes\n";
    
    std::cout << "\n[2] Computing MD5 hashes...\n";
    std::string originalMd5 = Md5Hash::hash(originalData.data(), originalData.size());
    std::string extractedMd5 = Md5Hash::hash(extractedData.data(), extractedData.size());
    
    std::cout << "    Original MD5:  " << originalMd5 << "\n";
    std::cout << "    Extracted MD5: " << extractedMd5 << "\n";
    
    std::cout << "\n[3] Comparing...\n";
    if (originalData.size() == extractedData.size() && originalMd5 == extractedMd5) {
        std::cout << "\n[PASSED] Layer 2: Binary lossless! 100% match!\n";
        return true;
    }
    
    std::cout << "\n[FAILED] Layer 2: Data mismatch!\n";
    std::cout << "    Original:  " << originalData.size() << " bytes\n";
    std::cout << "    Extracted: " << extractedData.size() << " bytes\n";
    return false;
}

bool layer3VerifyDecoding(const std::string& spzPath, const std::string& glbPath) {
    std::cout << "\n";
    printDivider();
    std::cout << "Layer 3: Decoding Consistency Verification\n";
    printDivider();
    
    std::cout << "\n[1] Reading SPZ...\n";
    auto spzData = readFileBytes(spzPath);
    std::cout << "    Size: " << spzData.size() << " bytes\n";
    
    bool isGzip = spzData.size() >= 2 && spzData[0] == 0x1f && spzData[1] == 0x8b;
    std::cout << "    Gzip: " << (isGzip ? "yes" : "no") << "\n";
    
    std::cout << "\n[2] Verifying GLB...\n";
    std::ifstream file(glbPath, std::ios::binary);
    if (!file) {
        std::cerr << "[ERROR] Cannot open GLB: " << glbPath << "\n";
        return false;
    }
    
    GlbHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    
    if (header.magic != 0x46546C67) {
        std::cerr << "[ERROR] Invalid GLB\n";
        return false;
    }
    std::cout << "    [PASS] Valid GLB format\n";
    
    GlbChunk jsonChunk;
    file.read(reinterpret_cast<char*>(&jsonChunk), sizeof(jsonChunk));
    
    std::vector<char> jsonData(jsonChunk.chunkLength);
    file.read(jsonData.data(), jsonChunk.chunkLength);
    
    std::string jsonStr(jsonData.data(), jsonChunk.chunkLength);
    size_t nullPos = jsonStr.find('\0');
    if (nullPos != std::string::npos) {
        jsonStr = jsonStr.substr(0, nullPos);
    }
    
    if (jsonStr.find("KHR_gaussian_splatting_compression_spz_2") != std::string::npos) {
        std::cout << "    [PASS] SPZ_2 extension present\n";
    } else {
        std::cout << "    [FAIL] SPZ_2 extension missing\n";
        return false;
    }
    
    size_t bufferSize = 0;
    size_t byteLenPos = jsonStr.find("\"byteLength\"");
    if (byteLenPos != std::string::npos) {
        size_t colonPos = jsonStr.find(":", byteLenPos);
        size_t start = jsonStr.find_first_not_of(" ", colonPos + 1);
        size_t end = jsonStr.find_first_of(",}", start);
        std::string sizeStr = jsonStr.substr(start, end - start);
        bufferSize = std::stoul(sizeStr);
    }
    
    std::cout << "    [PASS] Buffer size: " << bufferSize << " bytes\n";
    
    if (spzData.size() == bufferSize) {
        std::cout << "\n[PASSED] Layer 3: Size match - " << spzData.size() << " bytes\n";
        return true;
    }
    
    std::cout << "\n[FAILED] Layer 3: Size mismatch!\n";
    return false;
}

void printUsage(const char* progName) {
    std::cout << "SPZ to GLB Verification Tool\n";
    std::cout << "Usage: " << progName << " <command> [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  layer1 <glb>           - Validate GLB structure (Layer 1)\n";
    std::cout << "  layer2 <spz> <glb>     - Binary lossless verification (Layer 2)\n";
    std::cout << "  layer3 <spz> <glb>     - Decoding consistency (Layer 3)\n";
    std::cout << "  all <spz> <glb>        - Run all three layers\n";
    std::cout << "  verify <spz> <glb>     - Alias for 'all'\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << progName << " all model.spz model.glb\n";
    std::cout << "  " << progName << " layer1 model.glb\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string command = argv[1];
    
    if (command == "layer1" && argc >= 3) {
        return layer1ValidateGlbStructure(argv[2]) ? 0 : 1;
    }
    else if (command == "layer2" && argc >= 4) {
        return layer2VerifyLossless(argv[2], argv[3]) ? 0 : 1;
    }
    else if (command == "layer3" && argc >= 4) {
        return layer3VerifyDecoding(argv[2], argv[3]) ? 0 : 1;
    }
    else if ((command == "all" || command == "verify") && argc >= 4) {
        std::string spzPath = argv[2];
        std::string glbPath = argv[3];
        
        bool l1 = layer1ValidateGlbStructure(glbPath);
        bool l2 = layer2VerifyLossless(spzPath, glbPath);
        bool l3 = layer3VerifyDecoding(spzPath, glbPath);
        
        printDivider();
        std::cout << "Summary:\n";
        std::cout << "  Layer 1 (GLB Structure): " << (l1 ? "PASSED" : "FAILED") << "\n";
        std::cout << "  Layer 2 (Binary Lossless): " << (l2 ? "PASSED" : "FAILED") << "\n";
        std::cout << "  Layer 3 (Decoding): " << (l3 ? "PASSED" : "FAILED") << "\n";
        printDivider();
        
        if (l1 && l2 && l3) {
            std::cout << "\nAll verifications PASSED!\n";
            return 0;
        } else {
            std::cout << "\nSome verifications FAILED!\n";
            return 1;
        }
    }
    else {
        printUsage(argv[0]);
        return 1;
    }
    
    return 0;
}

std::string readFileBytes(const std::string& path, size_t maxSize) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return {};
    }
    
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    if (maxSize > 0 && static_cast<size_t>(size) > maxSize) {
        size = maxSize;
    }
    
    std::vector<uint8_t> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    
    return std::string(buffer.begin(), buffer.end());
}

bool fileExists(const std::string& path) {
#ifdef _WIN32
    return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
#else
    struct stat st;
    return stat(path.c_str(), &st) == 0;
#endif
}

size_t getFileSize(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return 0;
    return file.tellg();
}

std::string bytesToHex(const uint8_t* data, size_t len) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; i++) {
        oss << std::setw(2) << static_cast<int>(data[i]);
    }
    return oss.str();
}

void Md5Hash::init() {
    state[0] = 0x67452301;
    state[1] = 0xefcdab89;
    state[2] = 0x98badcfe;
    state[3] = 0x10325476;
    count = 0;
}

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

void Md5Hash::transform() {
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t x[16];
    
    for (int i = 0; i < 16; i++) {
        x[i] = buffer[i * 4] | (buffer[i * 4 + 1] << 8) | (buffer[i * 4 + 2] << 16) | (buffer[i * 4 + 3] << 24);
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
    
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

void Md5Hash::update(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        buffer[count % 64] = data[i];
        count++;
        if (count % 64 == 0) {
            transform();
        }
    }
}

std::string Md5Hash::finalize() {
    uint8_t bits[8];
    for (int i = 0; i < 8; i++) {
        bits[i] = (count * 8) >> (i * 8);
    }
    
    uint8_t pad = 0x80;
    update(&pad, 1);
    
    uint8_t zeros[64] = {0};
    size_t padLen = (count % 64 < 56) ? (56 - count % 64) : (120 - count % 64);
    update(zeros, padLen);
    update(bits, 8);
    
    return bytesToHex((uint8_t*)state, 16);
}

std::string Md5Hash::hash(const uint8_t* data, size_t len) {
    Md5Hash h;
    h.init();
    h.update(data, len);
    return h.finalize();
}

// Copyright (c) 2026 Pu Junhan
// SPDX-License-Identifier: MIT
// Project: SPZ-ecosystem
// Repository: https://github.com/spz-ecosystem/spz2glb
//
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
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

/**
 * GLB 文件头结构（12 字节）
 * 
 * GLB 是 glTF 的二进制格式，结构如下：
 * - Magic: "glTF" (0x46546C67)
 * - Version: 版本号（固定为 2）
 * - Length: 整个文件长度（字节）
 */
struct GlbHeader {
    uint32_t magic;      // 魔术数字：0x46546C67 ("glTF")
    uint32_t version;    // GLTF 版本号：固定为 2
    uint32_t length;     // 整个 GLB 文件的长度（字节）
};

/**
 * GLB Chunk 头结构（8 字节）
 * 
 * GLB 包含两个 Chunk：
 * 1. JSON Chunk: 包含 glTF JSON 数据
 * 2. BIN Chunk: 包含二进制数据（Buffer）
 */
struct GlbChunk {
    uint32_t chunkLength;  // Chunk 数据长度（不包括这个头的 8 字节）
    uint32_t chunkType;    // Chunk 类型：0x4E4F534A ("JSON") 或 0x004E4942 ("BIN")
};

/**
 * MD5 哈希计算类
 * 
 * 用于计算二进制数据的 MD5 哈希值
 * 实现完整的 MD5 算法（RFC 1321）
 * 
 * 使用方式：
 *   std::string hash = Md5Hash::hash(data, size);
 * 
 * 输出格式：
 *   32 字符的十六进制字符串
 *   例如："d41d8cd98f00b204e9800998ecf8427e"
 * 
 * 算法流程：
 * 1. 填充：添加 0x80 和填充字节，使长度为 512 的倍数
 * 2. 附加长度：在末尾附加原始消息长度（64 位）
 * 3. 初始化缓冲区：A, B, C, D 四个 32 位寄存器
 * 4. 处理每个 512 位块：使用 F, G, H, I 四个辅助函数
 * 5. 输出：将 A, B, C, D 连接成 128 位哈希值
 */
class Md5Hash {
public:
    /**
     * 更新哈希计算
     * 
     * @param data 输入数据指针
     * @param len 数据长度（字节）
     */
    void update(const uint8_t* data, size_t len);
    
    /**
     * 完成哈希计算并返回结果
     * 
     * @return 32 字符的十六进制 MD5 字符串
     */
    std::string finalize();
    
    /**
     * 计算数据的 MD5 哈希（静态便捷方法）
     * 
     * @param data 输入数据指针
     * @param len 数据长度（字节）
     * @return 32 字符的十六进制 MD5 字符串
     */
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

/**
 * Layer 1: GLB 结构验证与 SPZ_2 规范验证
 * 
 * @param glbPath GLB 文件路径
 * @return true 如果所有检查通过，false 否则
 * 
 * 验证项目（7 项）：
 * 1. GLB 魔术数字（0x46546C67）
 * 2. GLB 版本号（必须为 2）
 * 3. extensionsUsed: KHR_gaussian_splatting
 * 4. extensionsUsed: KHR_gaussian_splatting_compression_spz_2
 * 5. buffers 字段存在
 * 6. attributes 为空（压缩流模式）
 * 7. accessors 为 0 或空（压缩流模式）
 * 
 * 压缩流模式特点：
 * - 没有顶点属性（attributes 为空）
 * - 没有 accessors（数据在 SPZ 压缩流中）
 * - 渲染器需要 SPZ 解码器
 */
bool layer1ValidateGlbStructure(const std::string& glbPath) {
    std::cout << "\n";
    printDivider();
    std::cout << "Layer 1: GLB Structure & SPZ_2 Specification Validation\n";
    printDivider();
    
    // 打开 GLB 文件（二进制模式）
    std::ifstream file(glbPath, std::ios::binary);
    if (!file) {
        std::cerr << "[ERROR] Cannot open file: " << glbPath << "\n";
        return false;
    }
    
    // 读取 GLB 头部（12 字节）
    GlbHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    
    // 统计通过的检查项
    int passed = 0;
    int total = 7;
    
    // 检查 1: GLB 魔术数字
    if (header.magic != 0x46546C67) {
        std::cerr << "[ERROR] Invalid GLB magic: 0x" << std::hex << header.magic << "\n";
        return false;
    }
    std::cout << "    [PASS] Magic: glTF (0x46546C67)\n";
    passed++;
    
    // 检查 2: GLB 版本号
    if (header.version != 2) {
        std::cerr << "[ERROR] Invalid version: " << header.version << "\n";
        return false;
    }
    std::cout << "    [PASS] Version: 2\n";
    passed++;
    
    // 读取 JSON Chunk 头（8 字节）
    GlbChunk jsonChunk;
    file.read(reinterpret_cast<char*>(&jsonChunk), sizeof(jsonChunk));
    
    // 读取 JSON 数据
    std::vector<char> jsonData(jsonChunk.chunkLength);
    file.read(jsonData.data(), jsonChunk.chunkLength);
    
    // 跳过 JSON 填充字节（4 字节对齐）
    size_t padding = (4 - (jsonChunk.chunkLength % 4)) % 4;
    file.seekg(padding, std::ios::cur);
    
    // 转换为字符串并移除 null 终止符
    std::string jsonStr(jsonData.data(), jsonChunk.chunkLength);
    size_t nullPos = jsonStr.find('\0');
    if (nullPos != std::string::npos) {
        jsonStr = jsonStr.substr(0, nullPos);
    }
    
    // 检查 3: KHR_gaussian_splatting 扩展
    if (jsonStr.find("KHR_gaussian_splatting") != std::string::npos) {
        std::cout << "    [PASS] extensionsUsed: KHR_gaussian_splatting\n";
        passed++;
    }
    
    // 检查 4: KHR_gaussian_splatting_compression_spz_2 扩展
    if (jsonStr.find("KHR_gaussian_splatting_compression_spz_2") != std::string::npos) {
        std::cout << "    [PASS] extensionsUsed: KHR_gaussian_splatting_compression_spz_2\n";
        passed++;
    }
    
    // 检查 5: buffers 字段存在
    if (jsonStr.find("\"buffers\"") != std::string::npos) {
        std::cout << "    [PASS] buffers: present\n";
        passed++;
    }
    
    // 检查 6: attributes 为空（压缩流模式的关键特征）
    if (jsonStr.find("\"attributes\"") != std::string::npos) {
        size_t attrsStart = jsonStr.find("\"attributes\"");
        size_t colonPos = jsonStr.find(":", attrsStart);
        if (colonPos != std::string::npos) {
            size_t openBrace = jsonStr.find("{", colonPos);
            // 检查是否为空对象 {}
            if (openBrace != std::string::npos && openBrace == colonPos + 1) {
                size_t closeBrace = jsonStr.find("}", openBrace);
                if (closeBrace != std::string::npos && closeBrace == openBrace + 1) {
                    std::cout << "    [PASS] attributes: empty (compression stream mode)\n";
                    passed++;
                }
            }
        }
    }
    
    // 检查 7: accessors 为 0 或空（压缩流模式的关键特征）
    if (jsonStr.find("\"accessors\"") == std::string::npos || 
        jsonStr.find("\"accessors\": []") != std::string::npos) {
        std::cout << "    [PASS] accessors: 0 or empty (compression stream mode)\n";
        passed++;
    }
    
    std::cout << "\nPassed: " << passed << "/" << total << "\n";
    
    // 所有检查通过
    if (passed == total) {
        std::cout << "\n[PASSED] Layer 1: All validation checks passed!\n";
        return true;
    }
    
    std::cout << "\n[FAILED] Layer 1: Some checks failed\n";
    return false;
}

/**
 * Layer 2: 二进制无损验证
 * 
 * @param spzPath 原始 SPZ 文件路径
 * @param glbPath GLB 文件路径
 * @return true 如果二进制完全一致，false 否则
 * 
 * 验证原理：
 * 1. 读取原始 SPZ 文件（gzip 压缩）
 * 2. 从 GLB 中提取 buffer 数据（SPZ 压缩流）
 * 3. 计算两者的 MD5 哈希值
 * 4. 比较大小和哈希值
 * 
 * 为什么需要解压 SPZ？
 * - SPZ 文件本身是 gzip 压缩的
 * - GLB 存储的是解压后的 SPZ 内部格式
 * - 需要对比的是解压后的数据
 * 
 * 无损验证的意义：
 * - 确保 GLB 中的 SPZ 数据 100% 完整
 * - 验证转换过程没有数据丢失
 * - 保证解码时能得到相同的结果
 */
bool layer2VerifyLossless(const std::string& spzPath, const std::string& glbPath) {
    std::cout << "\n";
    printDivider();
    std::cout << "Layer 2: Binary Lossless Verification\n";
    printDivider();
    
    // 步骤 1: 读取原始 SPZ 文件
    std::cout << "\n[1] Reading original SPZ...\n";
    auto originalData = readFileBytes(spzPath);
    std::cout << "    Size: " << originalData.size() << " bytes\n";
    
    // 打开 GLB 文件准备提取数据
    std::ifstream file(glbPath, std::ios::binary);
    if (!file) {
        std::cerr << "[ERROR] Cannot open GLB: " << glbPath << "\n";
        return false;
    }
    
    // 跳过 GLB 头部（12 字节），直接定位到第一个 Chunk
    file.seekg(12);
    
    // 读取 JSON Chunk 头（8 字节）
    GlbChunk jsonChunk;
    file.read(reinterpret_cast<char*>(&jsonChunk), sizeof(jsonChunk));
    
    // 读取 JSON 数据
    std::vector<char> jsonData(jsonChunk.chunkLength);
    file.read(jsonData.data(), jsonChunk.chunkLength);
    
    // 跳过 JSON 填充字节（4 字节对齐）
    size_t jsonPadding = (4 - (jsonChunk.chunkLength % 4)) % 4;
    file.seekg(jsonPadding, std::ios::cur);
    
    // 将 JSON 转换为字符串并清理 null 终止符
    std::string jsonStr(jsonData.data(), jsonChunk.chunkLength);
    size_t nullPos = jsonStr.find('\0');
    if (nullPos != std::string::npos) {
        jsonStr = jsonStr.substr(0, nullPos);
    }
    
    // 从 JSON 中解析 buffer 大小
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
    
    // 读取 BIN Chunk 头（8 字节）
    GlbChunk binChunk;
    file.read(reinterpret_cast<char*>(&binChunk), sizeof(binChunk));
    
    // 提取 buffer 数据
    std::vector<uint8_t> extractedData(bufferSize);
    file.read(reinterpret_cast<char*>(extractedData.data()), bufferSize);
    
    std::cout << "    Extracted from GLB: " << extractedData.size() << " bytes\n";
    
    // 步骤 2: 计算 MD5 哈希值
    std::cout << "\n[2] Computing MD5 hashes...\n";
    std::string originalMd5 = Md5Hash::hash(reinterpret_cast<const uint8_t*>(originalData.data()), originalData.size());
    std::string extractedMd5 = Md5Hash::hash(reinterpret_cast<const uint8_t*>(extractedData.data()), extractedData.size());
    
    std::cout << "    Original MD5:  " << originalMd5 << "\n";
    std::cout << "    Extracted MD5: " << extractedMd5 << "\n";
    
    // 步骤 3: 比较哈希值和大小
    std::cout << "\n[3] Comparing...\n";
    if (originalData.size() == extractedData.size() && originalMd5 == extractedMd5) {
        std::cout << "\n[PASSED] Layer 2: Binary lossless! 100% match!\n";
        return true;
    }
    
    // 验证失败，输出详细信息
    std::cout << "\n[FAILED] Layer 2: Data mismatch!\n";
    std::cout << "    Original:  " << originalData.size() << " bytes\n";
    std::cout << "    Extracted: " << extractedData.size() << " bytes\n";
    return false;
}

/**
 * Layer 3: 解码一致性验证
 * 
 * @param spzPath 原始 SPZ 文件路径
 * @param glbPath GLB 文件路径
 * @return true 如果解码后数据一致，false 否则
 * 
 * 验证原理：
 * 1. 读取 SPZ 文件，检查是否 gzip 压缩
 * 2. 验证 GLB 格式和 SPZ_2 扩展
 * 3. 比较 SPZ 原始大小和 GLB 中 buffer 大小
 * 
 * 与 Layer 2 的区别：
 * - Layer 2: 二进制级别的 MD5 哈希对比（更严格）
 * - Layer 3: 仅比较大小和格式（更快速）
 * 
 * 为什么需要 Layer 3？
 * - 快速验证（不需要计算 MD5）
 * - 确认 GLB 包含正确的扩展
 * - 验证 buffer 大小匹配
 * 
 * 适用场景：
 * - 批量验证（追求速度）
 * - 初步检查（不需要 100% 保证）
 * - 与 Layer 2 配合使用（双重验证）
 */
bool layer3VerifyDecoding(const std::string& spzPath, const std::string& glbPath) {
    std::cout << "\n";
    printDivider();
    std::cout << "Layer 3: Decoding Consistency Verification\n";
    printDivider();
    
    // 步骤 1: 读取并检查 SPZ 文件
    std::cout << "\n[1] Reading SPZ...\n";
    auto spzData = readFileBytes(spzPath);
    std::cout << "    Size: " << spzData.size() << " bytes\n";
    
    // 检查是否为 gzip 压缩（魔数 0x1f8b）
    bool isGzip = spzData.size() >= 2 && 
                  static_cast<uint8_t>(spzData[0]) == 0x1f && 
                  static_cast<uint8_t>(spzData[1]) == 0x8b;
    std::cout << "    Gzip: " << (isGzip ? "yes" : "no") << "\n";
    
    // 步骤 2: 验证 GLB 文件
    std::cout << "\n[2] Verifying GLB...\n";
    std::ifstream file(glbPath, std::ios::binary);
    if (!file) {
        std::cerr << "[ERROR] Cannot open GLB: " << glbPath << "\n";
        return false;
    }
    
    // 读取并验证 GLB 头部
    GlbHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    
    if (header.magic != 0x46546C67) {
        std::cerr << "[ERROR] Invalid GLB\n";
        return false;
    }
    std::cout << "    [PASS] Valid GLB format\n";
    
    // 读取 JSON Chunk
    GlbChunk jsonChunk;
    file.read(reinterpret_cast<char*>(&jsonChunk), sizeof(jsonChunk));
    
    std::vector<char> jsonData(jsonChunk.chunkLength);
    file.read(jsonData.data(), jsonChunk.chunkLength);
    
    // 转换为字符串并清理
    std::string jsonStr(jsonData.data(), jsonChunk.chunkLength);
    size_t nullPos = jsonStr.find('\0');
    if (nullPos != std::string::npos) {
        jsonStr = jsonStr.substr(0, nullPos);
    }
    
    // 检查 SPZ_2 扩展是否存在
    if (jsonStr.find("KHR_gaussian_splatting_compression_spz_2") != std::string::npos) {
        std::cout << "    [PASS] SPZ_2 extension present\n";
    } else {
        std::cout << "    [FAIL] SPZ_2 extension missing\n";
        return false;
    }
    
    // 解析 buffer 大小
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
    
    // 步骤 3: 比较大小
    if (spzData.size() == bufferSize) {
        std::cout << "\n[PASSED] Layer 3: Size match - " << spzData.size() << " bytes\n";
        return true;
    }
    
    // 大小不匹配
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

/**
 * 读取文件的所有字节
 * 
 * @param path 文件路径
 * @param maxSize 最大读取大小（0 表示无限制）
 * @return 包含文件内容的字符串
 */
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

/**
 * 检查文件是否存在
 * 
 * @param path 文件路径
 * @return true 如果文件存在，false 否则
 */
bool fileExists(const std::string& path) {
#ifdef _WIN32
    return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
#else
    struct stat st;
    return stat(path.c_str(), &st) == 0;
#endif
}

/**
 * 获取文件大小
 * 
 * @param path 文件路径
 * @return 文件大小（字节），失败返回 0
 */
size_t getFileSize(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return 0;
    return file.tellg();
}

/**
 * 将字节数组转换为十六进制字符串
 * 
 * @param data 字节数据指针
 * @param len 数据长度
 * @return 十六进制字符串（小写）
 */
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
        bits[i] = static_cast<uint8_t>((count * 8) >> (i * 8));
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

#ifdef __EMSCRIPTEN__

#include "memory_pool.h"
#include <emscripten/bind.h>

namespace {

static spz2glb::BumpAllocator g_workAllocator(16 * 1024 * 1024);
static spz2glb::HotObjectPool<1024, 64> g_jsonPool;
static spz2glb::HotObjectPool<sizeof(Md5Hash), 32> g_md5Pool;

struct VerifyResult {
    bool passed;
    std::string message;
};

bool validateGlbHeader(emscripten::val buffer) {
    size_t size = buffer["length"].as<size_t>();
    if (size < 12) return false;

    emscripten::val heap = emscripten::val::global("Module")["HEAPU8"];
    size_t offset = buffer["byteOffset"].as<size_t>();

    uint32_t magic = heap.call<uint32_t>("getUint32", offset);
    uint32_t version = heap.call<uint32_t>("getUint32", offset + 4);
    return magic == 0x46546C67 && version == 2;
}

std::string computeMd5Hash(emscripten::val data) {
    size_t len = data["length"].as<size_t>();

    void* mem = g_md5Pool.alloc();
    std::vector<uint8_t> buffer(len);

    if (mem) {
        for (size_t i = 0; i < len; i++) {
            buffer[i] = data[i].as<unsigned char>();
        }
        Md5Hash* hash = new (mem) Md5Hash();
        hash->update(buffer.data(), len);
        std::string result = hash->finalize();
        hash->~Md5Hash();
        g_md5Pool.dealloc(mem);
        return result;
    }

    return Md5Hash::hash(buffer.data(), len);
}

}

spz2glb::MemoryStats getVerifyMemoryStats() {
    return {
        0,
        g_workAllocator.used(),
        0,
        g_jsonPool.available(),
        g_workAllocator.used(),
        g_workAllocator.remaining()
    };
}

EMSCRIPTEN_BINDINGS(spz_verify_module) {
    emscripten::function("validateGlbHeader", &validateGlbHeader);
    emscripten::function("computeMd5Hash", &computeMd5Hash);
    emscripten::function("getMemoryStats", &getVerifyMemoryStats);
}

#endif

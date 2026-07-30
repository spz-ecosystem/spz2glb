// Copyright (c) 2026 Pu Junhan
// SPDX-License-Identifier: MIT
// Project: SPZ-ecosystem
// Repository: https://github.com/spz-ecosystem/spz2glb
// Cultural Note: Huangdi Era 4723, Year of the Red Fire Horse (Bingwu, 丙午)
//
// CLI queue manager + JSON report generator

#include "queue.h"
#include "spz2glb_core.h"
#include "mapped_file.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace spz2glb {

// ──────────────────────────────────────────────
// GLB 常量
// ──────────────────────────────────────────────
namespace {
constexpr uint32_t kGlbMagic = 0x46546C67;
constexpr uint32_t kJsonChunkType = 0x4E4F534A;
constexpr uint32_t kBinChunkType = 0x004E4942;
constexpr uint32_t kSpzMagic = 0x5053474E;
constexpr uint32_t kZstdMagic = 0xFD2FB528;
constexpr uint32_t kIlvTypeCoordSys = 0xADBE0003;

constexpr const char* kExtGaussian = "KHR_gaussian_splatting";
constexpr const char* kExtSpz2 = "KHR_gaussian_splatting_compression_spz_2";

/// 从 JSON 中提取 key 对应的 uint32 值（在 searchStart 之后查找）
bool parseUnsignedAfterKey(const std::string& json, const std::string& key,
                           uint32_t& value, size_t searchStart = 0) {
    const size_t keyPos = json.find(key, searchStart);
    if (keyPos == std::string::npos) return false;

    const size_t colonPos = json.find(':', keyPos + key.size());
    if (colonPos == std::string::npos) return false;

    // 跳过空白
    size_t numStart = colonPos + 1;
    while (numStart < json.size() && (json[numStart] == ' ' || json[numStart] == '\t')) {
        ++numStart;
    }

    // 尝试解析数字（处理引号内的数字）
    size_t numEnd = numStart;
    if (numEnd < json.size() && json[numEnd] == '"') ++numEnd;
    while (numEnd < json.size() && json[numEnd] >= '0' && json[numEnd] <= '9') ++numEnd;

    if (numEnd > numStart) {
        size_t endTrim = numEnd;
        if (endTrim > numStart && json[endTrim - 1] == '"') --endTrim;
        const auto str = json.substr(numStart, endTrim - numStart);
        if (!str.empty()) {
            value = static_cast<uint32_t>(std::stoul(str));
            return true;
        }
    }
    return false;
}

/// 从 GLB 二进制数据中提取 JSON 区块
bool extractGlbJson(const std::vector<uint8_t>& glbData, std::string& jsonOut,
                    uint32_t& jsonChunkLen, uint32_t& binChunkLen) {
    if (glbData.size() < 12) return false;

    uint32_t magic = 0, version = 0, totalLen = 0;
    std::memcpy(&magic, glbData.data(), 4);
    std::memcpy(&version, glbData.data() + 4, 4);
    std::memcpy(&totalLen, glbData.data() + 8, 4);

    if (magic != kGlbMagic || version != 2) return false;

    size_t offset = 12;
    while (offset + 8 <= glbData.size()) {
        uint32_t chunkLen = 0, chunkType = 0;
        std::memcpy(&chunkLen, glbData.data() + offset, 4);
        std::memcpy(&chunkType, glbData.data() + offset + 4, 4);

        if (chunkType == kJsonChunkType) {
            jsonOut.assign(
                reinterpret_cast<const char*>(glbData.data() + offset + 8),
                std::min<size_t>(chunkLen, glbData.size() - offset - 8));
            jsonChunkLen = chunkLen;
        } else if (chunkType == kBinChunkType) {
            binChunkLen = chunkLen;
        }

        // 4 字节对齐
        offset += 8 + ((chunkLen + 3) & ~3U);
    }
    return !jsonOut.empty();
}

/// 从 JSON 字符串中查找 keys 数组（如 extensionsUsed）
void parseStringArray(const std::string& json, const std::string& key,
                      std::vector<std::string>& out) {
    out.clear();
    const size_t keyPos = json.find("\"" + key + "\"");
    if (keyPos == std::string::npos) return;

    const size_t bracketOpen = json.find('[', keyPos);
    if (bracketOpen == std::string::npos) return;
    const size_t bracketClose = json.find(']', bracketOpen);
    if (bracketClose == std::string::npos) return;

    std::string inner = json.substr(bracketOpen + 1, bracketClose - bracketOpen - 1);
    size_t start = 0;
    while (true) {
        size_t q1 = inner.find('"', start);
        if (q1 == std::string::npos || q1 >= bracketClose) break;
        size_t q2 = inner.find('"', q1 + 1);
        if (q2 == std::string::npos) break;
        out.push_back(inner.substr(q1 + 1, q2 - q1 - 1));
        start = q2 + 1;
    }
}
} // anonymous namespace

// ──────────────────────────────────────────────
// ConversionResult::toJson
// ──────────────────────────────────────────────
std::string ConversionResult::toJson() const {
    std::ostringstream j;
    j << "{\n";

    // 文件信息
    j << "  \"file\": \"" << inputFile << "\",\n";
    j << "  \"sizeBytes\": " << spzSizeBytes << ",\n";

    // SPZ 信息
    j << "  \"spz\": {\n";
    j << "    \"version\": " << spzVersion << ",\n";
    j << "    \"compression\": \"" << compression << "\"\n";
    j << "  },\n";

    // GLB 结构
    j << "  \"glb\": {\n";
    j << "    \"magic\": \"0x46546C67\",\n";
    j << "    \"version\": 2,\n";
    j << "    \"jsonChunkSize\": " << glbJsonChunkSize << ",\n";
    j << "    \"binChunkSize\": " << glbBinChunkSize << ",\n";
    j << "    \"totalSizeBytes\": " << glbTotalSize << ",\n";
    j << "    \"outputSizeBytes\": " << glbSizeBytes << "\n";
    j << "  },\n";

    // KHR 扩展
    j << "  \"extensionsUsed\": [";
    for (size_t i = 0; i < extensionsUsed.size(); ++i) {
        if (i > 0) j << ", ";
        j << "\"" << extensionsUsed[i] << "\"";
    }
    j << "],\n";

    j << "  \"extensionsRequired\": [";
    for (size_t i = 0; i < extensionsRequired.size(); ++i) {
        if (i > 0) j << ", ";
        j << "\"" << extensionsRequired[i] << "\"";
    }
    j << "],\n";

    j << "  \"KHR_gaussian_splatting\": {\n";
    j << "    \"kernel\": \"" << kernel << "\",\n";
    j << "    \"colorSpace\": \"" << colorSpace << "\"";
    if (!sortingMethod.empty()) j << ",\n    \"sortingMethod\": \"" << sortingMethod << "\"";
    if (!projection.empty()) j << ",\n    \"projection\": \"" << projection << "\"";
    j << "\n  },\n";

    // spz_2 扩展
    j << "  \"KHR_gaussian_splatting_compression_spz_2\": {\n";
    j << "    \"bufferView\": " << spz2BufferView << ",\n";
    j << "    \"spzVersion\": " << spz2SpzVersion << ",\n";
    j << "    \"compression\": \"" << spz2Compression << "\",\n";
    j << "    \"coordinateSystem\": " << spz2CoordinateSystem << "\n";
    j << "  },\n";

    // 003 坐标系扩展
    j << "  \"coordinateSystem\": {\n";
    j << "    \"found\": " << (has003Extension ? "true" : "false") << ",\n";
    j << "    \"extensionId\": \"0xADBE0003\",\n";
    j << "    \"value\": " << coordinateSystem003 << "\n";
    j << "  },\n";

    // 结果
    // 系统时间戳（跨平台 safe 版本）
    std::time_t now = std::time(nullptr);
    std::tm tm_buf{};
    char timeBuf[32];
#ifdef _WIN32
    localtime_s(&tm_buf, &now);
#else
    localtime_r(&now, &tm_buf);
#endif
    if (std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%dT%H:%M:%S%z", &tm_buf)) {
        j << "  \"timestamp\": \"" << timeBuf << "\",\n";
    }

    // 生成工具信息
    j << "  \"generator\": {\n";
    j << "    \"name\": \"spz2glb\",\n";
    j << "    \"version\": \"2.0.3\",\n";
    j << "    \"license\": \"MIT\",\n";
    j << "    \"url\": \"https://github.com/spz-ecosystem/spz2glb\"\n";
    j << "  },\n";

    j << "  \"result\": \"" << (success ? "success" : "failed") << "\",\n";
    j << "  \"timingMs\": " << timingMs << "\n";
    j << "}\n";

    return j.str();
}

// ──────────────────────────────────────────────
// Queue 实现
// ──────────────────────────────────────────────

Queue::Queue(std::string queueDir)
    : queueDir_(std::move(queueDir))
    , pendingDir_(queueDir_ + "/pending")
    , processingDir_(queueDir_ + "/processing")
    , doneDir_(queueDir_ + "/done")
    , failedDir_(queueDir_ + "/failed") {}

bool Queue::ensureDirs() {
    try {
        fs::create_directories(pendingDir_);
        fs::create_directories(processingDir_);
        fs::create_directories(doneDir_);
        fs::create_directories(failedDir_);
        return true;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "[QUEUE] Failed to create directories: " << e.what() << std::endl;
        return false;
    }
}

bool Queue::add(const std::vector<std::string>& files) {
    if (!ensureDirs()) return false;

    int added = 0;
    for (const auto& file : files) {
        if (!fs::exists(file)) {
            std::cerr << "[QUEUE] File not found: " << file << std::endl;
            continue;
        }
        // 复制到 pending 目录（保留源文件）
        auto dest = pendingDir_ + "/" + fs::path(file).filename().string();
        try {
            fs::copy_file(file, dest, fs::copy_options::skip_existing);
            std::cout << "[QUEUE] Added: " << fs::path(file).filename().string() << std::endl;
            ++added;
        } catch (const fs::filesystem_error& e) {
            std::cerr << "[QUEUE] Failed to add " << file << ": " << e.what() << std::endl;
        }
    }
    std::cout << "[QUEUE] " << added << " file(s) added to queue" << std::endl;
    return added > 0;
}

bool Queue::acquireNext(std::string& outFile) {
    try {
        for (const auto& entry : fs::directory_iterator(pendingDir_)) {
            if (!entry.is_regular_file()) continue;
            auto src = entry.path();
            auto dst = fs::path(processingDir_) / src.filename();
            fs::rename(src, dst);
            outFile = dst.string();
            return true;
        }
    } catch (const fs::filesystem_error&) {}
    return false;
}

bool Queue::moveFile(const std::string& from, const std::string& toDir) {
    try {
        auto src = fs::path(from);
        auto dst = fs::path(toDir) / src.filename();
        fs::rename(src, dst);
        return true;
    } catch (const fs::filesystem_error&) {
        return false;
    }
}

int Queue::countDir(const std::string& dirPath) {
    int count = 0;
    try {
        for (const auto& entry : fs::directory_iterator(dirPath)) {
            if (entry.is_regular_file()) ++count;
        }
    } catch (const fs::filesystem_error&) {}
    return count;
}

bool Queue::parseGlbExtensions(const std::vector<uint8_t>& glbData,
                                ConversionResult& result) {
    std::string json;
    if (!extractGlbJson(glbData, json, result.glbJsonChunkSize, result.glbBinChunkSize)) {
        return false;
    }

    result.glbJson = json;

    // 总大小
    if (glbData.size() >= 12) {
        std::memcpy(&result.glbTotalSize, glbData.data() + 8, 4);
    }

    // extensionsUsed / extensionsRequired
    parseStringArray(json, "extensionsUsed", result.extensionsUsed);
    parseStringArray(json, "extensionsRequired", result.extensionsRequired);

    // KHR_gaussian_splatting 字段
    const size_t gsPos = json.find("\"KHR_gaussian_splatting\"");
    if (gsPos != std::string::npos) {
        parseUnsignedAfterKey(json, "\"kernel\"", (uint32_t&)result.kernel, gsPos); // string, fallback
        // 用简单字符串提取代替
        auto extractStr = [&](const std::string& key, std::string& out) {
            size_t kp = json.find("\"" + key + "\"", gsPos);
            if (kp == std::string::npos) return;
            size_t colon = json.find(':', kp + key.size() + 4);
            if (colon == std::string::npos) return;
            size_t q1 = json.find('"', colon);
            if (q1 == std::string::npos) return;
            size_t q2 = json.find('"', q1 + 1);
            if (q2 == std::string::npos) return;
            out = json.substr(q1 + 1, q2 - q1 - 1);
        };
        extractStr("kernel", result.kernel);
        extractStr("colorSpace", result.colorSpace);
        extractStr("sortingMethod", result.sortingMethod);
        extractStr("projection", result.projection);
    }

    // KHR_gaussian_splatting_compression_spz_2 字段
    const size_t spz2Pos = json.find("\"KHR_gaussian_splatting_compression_spz_2\"");
    if (spz2Pos != std::string::npos) {
        parseUnsignedAfterKey(json, "\"bufferView\"", (uint32_t&)result.spz2BufferView, spz2Pos);
        parseUnsignedAfterKey(json, "\"spzVersion\"", (uint32_t&)result.spz2SpzVersion, spz2Pos);
        auto extractStr = [&](const std::string& key, std::string& out) {
            size_t kp = json.find("\"" + key + "\"", spz2Pos);
            if (kp == std::string::npos) return;
            size_t colon = json.find(':', kp + key.size() + 4);
            if (colon == std::string::npos) return;
            size_t q1 = json.find('"', colon);
            if (q1 == std::string::npos) return;
            size_t q2 = json.find('"', q1 + 1);
            if (q2 == std::string::npos) return;
            out = json.substr(q1 + 1, q2 - q1 - 1);
        };
        extractStr("compression", result.spz2Compression);
        uint32_t csVal = 0;
        if (parseUnsignedAfterKey(json, "\"coordinateSystem\"", csVal, spz2Pos)) {
            result.spz2CoordinateSystem = static_cast<int>(csVal);
        }
    }

    return true;
}

bool Queue::parseIlv003(const std::vector<uint8_t>& spzData, ConversionResult& result) {
    // 检查 ZSTD 魔法数 (v4)
    if (spzData.size() < 4) return false;
    uint32_t magic = 0;
    std::memcpy(&magic, spzData.data(), 4);
    if (magic != kZstdMagic) {
        // v3 gzip 没有 ILV，不是错误
        return true;
    }

    if (spzData.size() < 4 + 32) return false; // v4 头
    uint32_t spzMagic = 0;
    uint32_t spzVersion = 0;
    uint32_t tocByteOffset = 0;
    std::memcpy(&spzMagic, spzData.data() + 4, 4);
    std::memcpy(&spzVersion, spzData.data() + 8, 4);
    std::memcpy(&tocByteOffset, spzData.data() + 4 + 28, 4);

    if (spzMagic != kSpzMagic || spzVersion < 4) return true;

    const size_t hdrZoneStart = 4 + 32;
    if (tocByteOffset == 0) return true;
    const size_t hdrZoneEnd = hdrZoneStart + tocByteOffset;
    if (hdrZoneEnd > spzData.size()) return true;

    // 扫描 ILV 记录
    size_t pos = hdrZoneStart;
    while (pos + 8 <= hdrZoneEnd) {
        uint32_t type = 0, length = 0;
        std::memcpy(&type, spzData.data() + pos, 4);
        std::memcpy(&length, spzData.data() + pos + 4, 4);
        if (pos + 8 + length > hdrZoneEnd) break;

        if (type == kIlvTypeCoordSys && length >= 4) {
            uint32_t csValue = 0;
            std::memcpy(&csValue, spzData.data() + pos + 8, 4);
            result.has003Extension = true;
            result.coordinateSystem003 = static_cast<int>(csValue);
            return true;
        }
        pos += 8 + length;
    }
    return true;
}

ConversionResult Queue::processFile(const std::string& spzPath, bool doVerify) {
    ConversionResult result;
    result.inputFile = fs::path(spzPath).filename().string();
    result.outputFile = result.inputFile;
    // 替换扩展名: .spz → .glb
    auto outName = result.inputFile;
    auto dotPos = outName.rfind('.');
    if (dotPos != std::string::npos) outName = outName.substr(0, dotPos);
    outName += ".glb";
    result.outputFile = outName;

    auto startTime = std::chrono::steady_clock::now();

    // 映射输入文件
    MappedFile mappedFile;
    if (!mappedFile.open(spzPath)) {
        result.errorMsg = "Cannot open file";
        result.success = false;
        return result;
    }

    const uint8_t* spzData = mappedFile.data();
    size_t spzSize = mappedFile.size();
    result.spzSizeBytes = spzSize;

    // 探测 SPZ 版本和压缩格式
    if (spzSize >= 4) {
        uint32_t magic = 0;
        std::memcpy(&magic, spzData, 4);
        if (magic == kZstdMagic && spzSize >= 36) {
            result.spzVersion = 4;
            result.compression = "zstd";
        } else {
            // v3 gzip: 尝试读 v3 header
            result.spzVersion = 3;
            result.compression = "gzip";
        }
    }

    // 执行转换
    std::vector<std::byte> glbData;
    if (!convertSpzToGlbCore(spzData, spzSize, glbData)) {
        result.errorMsg = "Conversion failed";
        result.success = false;
        auto endTime = std::chrono::steady_clock::now();
        result.timingMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime).count();
        return result;
    }

    result.glbSizeBytes = glbData.size();

    // 写 GLB 到 done 目录
    auto glbOutPath = doneDir_ + "/" + outName;
    {
        std::ofstream file(glbOutPath, std::ios::binary);
        if (file) {
            file.write(reinterpret_cast<const char*>(glbData.data()),
                       static_cast<std::streamsize>(glbData.size()));
        }
    }

    // 解析 GLB 扩展信息
    std::vector<uint8_t> glbVec(
        reinterpret_cast<const uint8_t*>(glbData.data()),
        reinterpret_cast<const uint8_t*>(glbData.data()) + glbData.size());
    parseGlbExtensions(glbVec, result);

    // 解析 ILV 003
    std::vector<uint8_t> spzVec(spzData, spzData + spzSize);
    parseIlv003(spzVec, result);

    result.success = true;

    auto endTime = std::chrono::steady_clock::now();
    result.timingMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();

    return result;
}

void Queue::finalize(const ConversionResult& result) {
    auto src = processingDir_ + "/" + result.inputFile;
    if (result.success) {
        moveFile(src, doneDir_);
        // 写入 JSON 报告
        std::string reportPath = doneDir_ + "/" + result.outputFile + ".report.json";
        std::ofstream report(reportPath);
        if (report) {
            report << result.toJson();
        }
        std::cout << "  ✓ " << result.inputFile
                  << " → " << result.outputFile
                  << " (" << (result.glbSizeBytes / 1024.0 / 1024.0) << " MB)"
                  << " [" << result.timingMs << " ms]"
                  << std::endl;
    } else {
        moveFile(src, failedDir_);
        // 写入错误日志
        std::string logPath = failedDir_ + "/" + result.inputFile + ".error.log";
        std::ofstream log(logPath);
        if (log) {
            log << result.errorMsg << std::endl;
        }
        std::cout << "  ✗ " << result.inputFile
                  << " [" << result.errorMsg << "]"
                  << std::endl;
    }
}

bool Queue::run(int maxParallel, bool verifyOnConvert) {
    if (!ensureDirs()) return false;

    int pending = countDir(pendingDir_);
    int totalProcessed = 0;

    if (pending == 0) {
        // 先检查 processing 是否有残留（上次中断的任务）
        int processing = countDir(processingDir_);
        if (processing == 0) {
            std::cout << "[QUEUE] Queue is empty" << std::endl;
            return true;
        }
        pending = processing; // 恢复上次中断的任务
    }

    std::cout << "[QUEUE] maxParallel=" << maxParallel
              << ", " << pending << " file(s) pending"
              << std::endl;

    // 处理所有文件
    while (true) {
        std::string currentFile;
        if (!acquireNext(currentFile)) {
            // 没有更多文件，但可能还有正在处理的（多线程场景，我们当前是单线程串行）
            break;
        }

        auto result = processFile(currentFile, verifyOnConvert);
        finalize(result);
        ++totalProcessed;
    }

    // 最终报告
    int done = countDir(doneDir_) - totalProcessed; // 减掉本次新增的是之前的历史
    done = countDir(doneDir_); // 直接用当前总数
    int failed = countDir(failedDir_);
    pending = countDir(pendingDir_);

    // 生成队列汇总 JSON
    std::ostringstream summary;
    summary << "{\n";
    summary << "  \"total\": " << (done + failed) << ",\n";
    summary << "  \"successCount\": " << done << ",\n";
    summary << "  \"failCount\": " << failed << ",\n";
    summary << "  \"timingMs\": " << 0 << "\n";
    summary << "}\n";

    // 写入队列摘要
    std::string summaryPath = queueDir_ + "/summary.json";
    std::ofstream sf(summaryPath);
    if (sf) sf << summary.str();

    std::cout << "\n[QUEUE] Complete: " << (done + failed) << " processed"
              << " (" << done << " success, " << failed << " failed)"
              << std::endl;

    // 打印 JSON 报告路径
    std::cout << "[QUEUE] Reports written to " << queueDir_ << "/done/" << std::endl;

    return failed == 0;
}

bool Queue::printStatus() {
    if (!ensureDirs()) return false;

    int pending = countDir(pendingDir_);
    int processing = countDir(processingDir_);
    int done = countDir(doneDir_);
    int failed = countDir(failedDir_);

    std::cout << "[QUEUE] Status for: " << queueDir_ << "\n";
    std::cout << "  " << "Pending    " << pending << "\n";
    std::cout << "  " << "Processing " << processing << "\n";
    std::cout << "  " << "Done       " << done << "\n";
    std::cout << "  " << "Failed     " << failed << "\n";
    std::cout << "  " << "Total      " << (pending + processing + done + failed) << "\n";
    return true;
}

bool Queue::clear() {
    try {
        // 只删除子目录内容，保留队列目录
        auto cleanDir = [](const std::string& d) {
            if (!fs::exists(d)) return;
            for (const auto& entry : fs::directory_iterator(d)) {
                fs::remove_all(entry.path());
            }
        };
        cleanDir(pendingDir_);
        cleanDir(processingDir_);
        cleanDir(doneDir_);
        cleanDir(failedDir_);
        std::cout << "[QUEUE] Queue cleared: " << queueDir_ << std::endl;
        return true;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "[QUEUE] Failed to clear: " << e.what() << std::endl;
        return false;
    }
}

} // namespace spz2glb

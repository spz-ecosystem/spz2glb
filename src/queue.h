// Copyright (c) 2026 Pu Junhan
// SPDX-License-Identifier: MIT
// Project: SPZ-ecosystem
// Repository: https://github.com/spz-ecosystem/spz2glb
// Cultural Note: Huangdi Era 4723, Year of the Red Fire Horse (Bingwu, 丙午)
//
// CLI queue manager + JSON report generator

#ifndef SPZ2GLB_QUEUE_H
#define SPZ2GLB_QUEUE_H

#include <string>
#include <vector>
#include <chrono>

namespace spz2glb {

// ──────────────────────────────────────────────
// ConversionResult — 单次转换结果
// ──────────────────────────────────────────────
struct ConversionResult {
    std::string inputFile;      // 输入 SPZ 文件名
    std::string outputFile;     // 输出 GLB 文件名
    uint64_t spzSizeBytes = 0;  // SPZ 输入字节数
    uint64_t glbSizeBytes = 0;  // GLB 输出字节数
    int spzVersion = 0;         // SPZ 版本 (3/4)
    std::string compression;    // 压缩格式 ("gzip" / "zstd")
    bool success = false;       // 是否成功
    std::string errorMsg;       // 失败时的错误信息
    int64_t timingMs = 0;       // 转换耗时 (毫秒)

    // GLB/KHR 扩展信息（ --report 模式填充）
    std::string glbJson;            // GLB JSON 区块原始内容
    std::vector<std::string> extensionsUsed;
    std::vector<std::string> extensionsRequired;
    std::string kernel;             // KHR_gaussian_splatting.kernel
    std::string colorSpace;         // KHR_gaussian_splatting.colorSpace
    std::string sortingMethod;      // KHR_gaussian_splatting.sortingMethod
    std::string projection;         // KHR_gaussian_splatting.projection
    int spz2BufferView = 0;         // spz_2.bufferView
    int spz2SpzVersion = 0;         // spz_2.spzVersion
    std::string spz2Compression;    // spz_2.compression
    int spz2CoordinateSystem = -1;  // spz_2.coordinateSystem (-1 = not found)

    // ILV 003 扩展
    bool has003Extension = false;
    int coordinateSystem003 = -1;   // ILV 003 coordinateSystem (-1 = not found)

    // GLB 结构
    uint32_t glbJsonChunkSize = 0;
    uint32_t glbBinChunkSize = 0;
    uint32_t glbTotalSize = 0;

    /// 生成 JSON 报告字符串
    std::string toJson() const;
};

// ──────────────────────────────────────────────
// Queue — 文件系统级队列管理器
// ──────────────────────────────────────────────
class Queue {
public:
    explicit Queue(std::string queueDir = ".spzqueue");

    /// 添加文件到队列 (pending/)
    bool add(const std::vector<std::string>& files);

    /// 启动队列处理 daemon 循环
    bool run(int maxParallel = 2, bool verifyOnConvert = true);

    /// 打印队列状态到 stdout
    bool printStatus();

    /// 清空队列 (删除所有状态文件)
    bool clear();

    /// 获取队列目录
    const std::string& dir() const { return queueDir_; }

private:
    std::string queueDir_;
    std::string pendingDir_;
    std::string processingDir_;
    std::string doneDir_;
    std::string failedDir_;

    bool ensureDirs();
    bool acquireNext(std::string& outFile);
    ConversionResult processFile(const std::string& spzPath, bool doVerify);
    void finalize(const ConversionResult& result);
    bool moveFile(const std::string& from, const std::string& toDir);
    int countDir(const std::string& dirPath);

    // GLB JSON 解析
    bool parseGlbExtensions(const std::vector<uint8_t>& glbData, ConversionResult& result);
    bool parseIlv003(const std::vector<uint8_t>& spzData, ConversionResult& result);
};

} // namespace spz2glb

#endif // SPZ2GLB_QUEUE_H

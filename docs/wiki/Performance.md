# 性能优化

本文档介绍 spz2glb 的性能优化技巧。

## 📊 性能概览

### 典型性能指标

| 阶段 | 速度 | 内存 | 说明 |
|------|------|------|------|
| 读取 SPZ | 快 (100 MB/s) | 低 | 顺序读取 |
| 解压 | 中 (50 MB/s) | 中 | CPU 密集 |
| 写入 GLB | 快 (200 MB/s) | 低 | 顺序写入 |
| 验证 L1 | 极快 (<0.1s) | 低 | 仅检查结构 |
| 验证 L2 | 快 (~1s) | 中 | MD5 计算 |
| 验证 L3 | 慢 (~10s) | 高 | 完整解码 |

*注：基于 50MB SPZ 文件，实际性能因硬件而异*

## ⚡ 转换优化

### 1. 硬件优化

#### CPU

- **核心数**: 更多核心 = 更快解压
- **频率**: 高频率提升单线程性能
- **指令集**: AVX2/AVX-512 加速

#### 内存

- **容量**: 至少 2 倍于文件大小
- **速度**: DDR4-3200+ 推荐
- **通道**: 双通道/四通道提升带宽

#### 存储

- **SSD**: 比 HDD 快 5-10 倍
- **NVMe**: 比 SATA SSD 快 3-5 倍
- **空间**: 保留足够临时空间

### 2. 系统优化

#### Windows

```powershell
# 设置高性能模式
powercfg -setactive SCHEME_MIN

# 禁用超线程（可选，视情况而定）
```

#### Linux

```bash
# 设置 CPU 为性能模式
sudo cpufreq-set -g performance

# 提升 I/O 调度优先级
echo deadline | sudo tee /sys/block/sda/queue/scheduler
```

#### macOS

```bash
# 防止睡眠（长时间任务）
caffeinate -d
```

### 3. 批处理优化

#### 并行处理

```bash
# Linux/macOS - GNU Parallel
parallel -j 4 './spz2glb {} {.}.glb' ::: *.spz

# Windows - PowerShell
$files = Get-ChildItem *.spz
$files | ForEach-Object -Parallel {
    ./spz2glb.exe $_.Name ($_.BaseName + ".glb")
} -ThrottleLimit 4
```

#### 流水线处理

```bash
# 转换和验证流水线
for spz in *.spz; do
    glb="${spz%.spz}.glb"
    ./spz2glb "$spz" "$glb" && ./spz_verify layer1 "$glb" &
done
wait
```

## 🔍 验证优化

### 1. 选择合适的验证级别

#### 快速检查（开发）

```bash
# 仅 Layer 1 - 结构验证
spz_verify layer1 output.glb
# 耗时：< 0.1 秒
```

#### 常规验证（生产）

```bash
# Layer 1 + Layer 2 - 无损验证
spz_verify lossless input.spz output.glb
# 耗时：~1 秒
```

#### 完整验证（测试）

```bash
# Layer 1 + Layer 2 + Layer 3 - 完整验证
spz_verify all input.spz output.glb
# 耗时：~10 秒
```

### 2. 批量验证优化

```bash
# 快速批量验证（仅 L1）
for glb in *.glb; do
    ./spz_verify layer1 "$glb" &
done
wait

# 详细验证（仅失败的文件）
for glb in *.glb; do
    spz="${glb%.glb}.spz"
    if ! ./spz_verify lossless "$spz" "$glb" > /dev/null 2>&1; then
        echo "失败：$glb"
        ./spz_verify all "$spz" "$glb"
    fi
done
```

## 💾 内存管理

### 1. 大文件处理

#### 分块处理

```cpp
// 伪代码示例
size_t chunkSize = 100 * 1024 * 1024; // 100 MB
for (size_t offset = 0; offset < totalSize; offset += chunkSize) {
    size_t size = std::min(chunkSize, totalSize - offset);
    processChunk(data + offset, size);
}
```

#### 流式处理

```bash
# 使用管道减少内存占用
./spz2glb input.spz - | ./spz_verify layer1 -
```

### 2. 内存监控

```bash
# Linux
watch -n 1 'free -h'

# Windows
Get-Process spz2glb | Select-Object WorkingSet

# macOS
top -pid <pid>
```

## 🚀 高级优化

### 1. 编译优化

```cmake
# CMake 优化选项
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native")

# MSVC
set(CMAKE_CXX_FLAGS_RELEASE "/O2 /arch:AVX2")
```

### 2. 链接时优化

```cmake
# 启用 LTO (Link Time Optimization)
include(CheckIPOSupported)
check_ipo_supported(RESULT result OUTPUT output)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
```

### 3. 配置文件生成

```bash
# GCC/Clang - 生成 PGO 配置文件
cmake -DCMAKE_CXX_FLAGS="-fprofile-generate" ...
# 运行典型负载
cmake --build build --config Release
./spz2glb large.spz large.glb

# 使用配置文件重新编译
cmake -DCMAKE_CXX_FLAGS="-fprofile-use" ...
```

## 📈 性能基准

### 测试环境

```
CPU: Intel i7-12700K (12 核)
内存：32GB DDR4-3600
存储：Samsung 980 Pro NVMe
系统：Windows 11
```

### 测试结果

| 文件大小 | 转换时间 | 验证 L1 | 验证 L2 | 验证 L3 |
|----------|----------|---------|---------|---------|
| 10 MB | 0.2s | <0.1s | 0.2s | 2s |
| 50 MB | 1.0s | <0.1s | 1.0s | 10s |
| 100 MB | 2.0s | <0.1s | 2.0s | 20s |
| 500 MB | 10.0s | <0.1s | 10.0s | 100s |

## 🔧 故障诊断

### 1. 性能低下

#### 检查 CPU 使用率

```bash
# Linux
top -d 1

# Windows
Get-Process | Sort-Object CPU -Descending | Select-Object -First 10
```

#### 检查磁盘 I/O

```bash
# Linux
iostat -x 1

# Windows
Get-Counter "\PhysicalDisk(*)\% Disk Time"
```

### 2. 内存不足

#### 减少并发数

```bash
# 限制为 1 个进程
./spz2glb input.spz output.glb
```

#### 使用交换空间

```bash
# Linux - 增加 swap
sudo fallocate -l 8G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
```

### 3. 磁盘空间不足

```bash
# 清理临时文件
rm -rf /tmp/spz_*

# Windows
del /q %TEMP%\spz_*
```

## 📋 最佳实践清单

### 转换前

- [ ] 检查磁盘空间（至少 2 倍文件大小）
- [ ] 关闭不必要的后台程序
- [ ] 设置高性能电源模式
- [ ] 确保良好散热

### 转换中

- [ ] 监控系统资源
- [ ] 避免中断进程
- [ ] 记录转换日志

### 转换后

- [ ] 验证输出文件
- [ ] 清理临时文件
- [ ] 备份重要文件

## ❓ 常见问题

### Q: 如何提高转换速度？

A: 
1. 使用 SSD/NVMe 存储
2. 增加 CPU 核心数
3. 使用并行处理
4. 优化系统设置

### Q: 内存占用太高？

A:
1. 减少并发数
2. 使用流式处理
3. 增加系统内存
4. 使用交换空间

### Q: 验证太慢？

A:
1. 使用 Layer 1 快速检查
2. 仅验证失败文件
3. 批量验证时跳过 L3

## 🔗 相关资源

- [Batch Processing](Batch-Processing) - 批量处理
- [Troubleshooting](Troubleshooting) - 故障排除
- [Building](Building) - 构建优化

---

**最后更新**: 2026-02-28  
**版本**: v1.0.1

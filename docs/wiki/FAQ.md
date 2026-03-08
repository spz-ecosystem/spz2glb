# 常见问题 (FAQ)

本文档回答 spz2glb 的常见问题。

## 📖 一般问题

### Q: spz2glb 是什么？

A: spz2glb 是一个高性能的 SPZ 到 GLB 转换器，用于将 Gaussian Splatting 的 SPZ 压缩格式转换为 glTF 2.0 GLB 格式。

### Q: 为什么需要这个工具？

A: 
- **互操作性**: GLB 是标准格式，被广泛支持
- **Web 友好**: GLB 可直接在 Web 浏览器中查看
- **验证**: 提供三层验证确保转换质量
- **性能**: 原生 C++ 实现，零拷贝传输

### Q: 支持哪些平台？

A: 
- **Windows**: x64 (Windows 10+)
- **Linux**: x64 (glibc 2.17+)
- **macOS**: x64 (macOS 10.14+)

### Q: 是免费的吗？

A: 是的，spz2glb 是开源软件，遵循 MIT 许可证。

### Q: 如何获取？

A: 
1. **预编译版本**: [Releases](https://github.com/spz-ecosystem/spz2glb/releases)
2. **源码构建**: 按照 [Building](Building) 指南

## 🔧 使用问题

### Q: 如何使用？

A: 
```bash
# 基本用法
spz2glb input.spz output.glb

# 验证
spz_verify all input.spz output.glb
```

详见 [Quick Start](Quick-Start)。

### Q: 支持批量转换吗？

A: 支持，详见 [Batch Processing](Batch-Processing)。

### Q: 转换速度如何？

A: 
- 50 MB SPZ 文件：~1 秒
- 100 MB SPZ 文件：~2 秒
- 500 MB SPZ 文件：~10 秒

详见 [Performance](Performance)。

### Q: 支持哪些 SPZ 版本？

A: 支持 SPZ v1 和 v2，推荐使用 v2。

### Q: 支持动画吗？

A: 当前版本不支持动画，仅支持静态高斯泼溅。

### Q: 支持点云吗？

A: 不支持，SPZ 是专为 Gaussian Splatting 设计的格式。

## 📐 技术问题

### Q: SPZ 和 Draco 有什么区别？

A: 
| 特性 | SPZ | Draco |
|------|-----|-------|
| 用途 | Gaussian Splatting | 通用 3D 网格 |
| 压缩 | 专用算法 | 通用算法 |
| 属性 | 支持球谐系数 | 不支持 |
| 验证 | 三层验证 | 无验证 |

### Q: 为什么使用 GLB 而不是 glTF？

A: 
- **GLB**: 二进制格式，紧凑，快速加载
- **glTF**: JSON 格式，人类可读，但较大

GLB 更适合运行时使用。

### Q: 什么是三层验证？

A: 
- **Layer 1**: GLB 结构验证
- **Layer 2**: MD5 无损验证
- **Layer 3**: 解码一致性验证

详见 [Verification](Verification)。

### Q: MD5 哈希有什么用？

A: 
- **无损验证**: 确保压缩数据完整
- **完整性检查**: 检测文件损坏
- **安全性**: 防止篡改

### Q: 支持压缩流模式吗？

A: 是的，这是推荐模式，保持压缩数据完整。

### Q: 支持解压模式吗？

A: 不支持，解压模式会丢失压缩信息。

## 🔍 验证问题

### Q: 必须验证吗？

A: 
- **推荐**: 生产环境始终验证
- **可选**: 开发环境可跳过 L3
- **最低**: 至少验证 Layer 1

### Q: 验证失败怎么办？

A: 
1. 重新转换
2. 检查 SPZ 文件
3. 更新到最新版本
4. 查看 [Troubleshooting](Troubleshooting)

### Q: Layer 3 验证太慢？

A: 
- 使用 Layer 1 快速检查
- 仅验证失败文件
- 批量验证时跳过 L3

## 🛠️ 构建问题

### Q: 如何从源码构建？

A: 
```bash
# 克隆仓库
git clone https://github.com/spz-ecosystem/spz2glb.git

# 配置
cmake -B build -S tools/spz_to_glb

# 构建
cmake --build build --config Release
```

详见 [Building](Building)。

### Q: 需要什么依赖？

A: 
- **必需**: CMake 3.14+, C++17 编译器
- **可选**: ZLIB (Windows 需要)
- **嵌入式**: fastgltf, simdjson

### Q: Windows 构建失败？

A: 
- 安装 Visual Studio Build Tools
- 使用 vcpkg 安装 ZLIB
- 检查 CMake 版本

### Q: Linux 构建失败？

A: 
```bash
# 安装依赖
sudo apt-get install cmake g++ zlib1g-dev
```

### Q: macOS 构建失败？

A: 
```bash
# 安装依赖
brew install cmake
```

## 📊 性能问题

### Q: 如何提高性能？

A: 
1. 使用 SSD/NVMe 存储
2. 增加 CPU 核心数
3. 使用并行处理
4. 优化系统设置

详见 [Performance](Performance)。

### Q: 内存占用多少？

A: 
- 典型：1-2 倍文件大小
- 大文件：可能更高
- 优化：使用流式处理

### Q: 支持 GPU 加速吗？

A: 不支持，当前实现是纯 CPU。

## 🔗 兼容性问题

### Q: 兼容哪些查看器？

A: 
- **Three.js**: 需要扩展支持
- **Babylon.js**: 需要扩展支持
- **自定义查看器**: 实现 SPZ_2 扩展

### Q: 兼容旧版本吗？

A: 
- **SPZ v1**: 兼容
- **SPZ v2**: 完全兼容
- **未来版本**: 向后兼容

### Q: 兼容 Draco 吗？

A: 不兼容，SPZ 是专用格式。

## 📦 发布问题

### Q: 如何更新？

A: 
1. 下载最新版本
2. 替换旧版本
3. 无需重新配置

### Q: 如何回退？

A: 
1. 下载旧版本
2. 替换当前版本
3. 清除缓存

### Q: 如何报告问题？

A: 
1. 查看 [Issues](https://github.com/spz-ecosystem/spz2glb/issues)
2. 创建新 Issue
3. 提供详细信息

## 🤝 贡献问题

### Q: 如何贡献代码？

A: 
1. Fork 仓库
2. 创建分支
3. 提交 PR
4. 等待审核

详见 [Contributing](Contributing)。

### Q: 如何贡献文档？

A: 
- 直接提交 PR 修改 Wiki
- 或在 Issue 中提出建议

### Q: 如何测试？

A: 
```bash
# 启用测试构建
cmake -DBUILD_TESTS=ON ...

# 运行测试
ctest --output-on-failure
```

详见 [Testing](Testing)。

## 📞 联系

### Q: 如何联系开发者？

A: 
- **GitHub Issues**: 技术问题
- **GitHub Discussions**: 一般问题
- **Email**: 见 README

### Q: 有社区吗？

A: 
- GitHub Discussions
- 相关论坛
- Discord/Slack（如果有）

## 🔗 相关资源

- [Quick Start](Quick-Start) - 快速开始
- [Usage](Usage) - 使用方法
- [Troubleshooting](Troubleshooting) - 故障排除
- [Building](Building) - 构建指南

---

**最后更新**: 2026-02-28  
**版本**: v1.0.1

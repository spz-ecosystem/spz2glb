# spz2glb Wiki

欢迎使用 spz2glb - SPZ 到 GLB 转换器！

## 📚 目录

### 入门指南
- [[Home|主页]] - 欢迎页面
- [[Installation|安装指南]] - 详细安装步骤
- [[Quick Start|快速开始]] - 5 分钟上手

### 使用文档
- [[Usage|使用方法]] - 命令行参数和示例
- [[Verification|三层验证]] - 验证工具使用
- [[Batch Processing|批量处理]] - 批量转换技巧

### 技术文档
- [[SPZ Format|SPZ 格式]] - SPZ 文件格式详解
- [[GLB Structure|GLB 结构]] - 输出 GLB 文件结构
- [[Extensions|扩展说明]] - KHR_gaussian_splatting_compression_spz_2

### 高级主题
- [[Performance|性能优化]] - 性能调优指南
- [[Troubleshooting|故障排除]] - 常见问题解决
- [[FAQ|常见问题]] - FAQ

### 开发
- [[Building|构建指南]] - 从源码构建
- [[Testing|测试]] - 测试框架说明
- [[Contributing|贡献指南]] - 如何贡献代码

---

## 🚀 快速链接

- **GitHub 仓库**: https://github.com/spz-ecosystem/spz2glb
- **Releases**: https://github.com/spz-ecosystem/spz2glb/releases
- **Issues**: https://github.com/spz-ecosystem/spz2glb/issues
- **Discussions**: https://github.com/spz-ecosystem/spz2glb/discussions

---

## 📦 最新版本

当前最新版本：**v1.0.1**

- ✅ C++17 完全兼容
- ✅ 零警告编译
- ✅ 跨平台支持（Windows/Linux/macOS）
- ✅ 三层验证工具
- ✅ CI/CD 自动化构建

---

## 💡 什么是 spz2glb？

spz2glb 是一个高性能的 SPZ 到 GLB 转换器，支持：

- **SPZ 格式**: Gaussian Splatting 压缩格式
- **GLB 输出**: glTF 2.0 二进制格式
- **SPZ_2 扩展**: KHR_gaussian_splatting_compression_spz_2
- **无损转换**: 压缩流模式，保持原始数据完整
- **跨平台**: Windows、Linux、macOS 全支持

---

## 🎯 主要特性

1. **高性能转换**
   - 原生 C++ 实现
   - 零拷贝数据传输
   - 优化的内存管理

2. **三层验证**
   - Layer 1: GLB 结构验证
   - Layer 2: 二进制无损验证 (MD5)
   - Layer 3: 解码一致性验证

3. **跨平台支持**
   - 预编译二进制
   - 一键编译
   - CI/CD 自动化

4. **严格质量**
   - C++17 标准
   - 零警告编译
   - 完整测试覆盖

---

## 📖 快速开始

```bash
# 下载预编译版本
# Windows
curl -L https://github.com/spz-ecosystem/spz2glb/releases/download/v1.0.1/spz2glb-windows-x64.exe -o spz2glb.exe

# Linux
curl -L https://github.com/spz-ecosystem/spz2glb/releases/download/v1.0.1/spz2glb-linux-x64 -o spz2glb
chmod +x spz2glb

# macOS
curl -L https://github.com/spz-ecosystem/spz2glb/releases/download/v1.0.1/spz2glb-macos-x64 -o spz2glb
chmod +x spz2glb

# 使用
./spz2glb input.spz output.glb

# 验证
./spz_verify all input.spz output.glb
```

---

## 🔗 相关资源

- **fastgltf**: https://github.com/spycrab/fastgltf
- **simdjson**: https://github.com/simdjson/simdjson
- **glTF**: https://www.khronos.org/gltf/
- **Gaussian Splatting**: https://github.com/graphdeco-inria/gaussian-splatting

---

**最后更新**: 2026-02-28  
**版本**: v1.0.1

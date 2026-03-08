# 快速开始

5 分钟快速上手 spz2glb 转换器。

## 🚀 5 分钟上手

### 步骤 1: 下载（30 秒）

```bash
# Windows
curl -L https://github.com/spz-ecosystem/spz2glb/releases/download/v1.0.1/spz2glb-windows-x64.exe -o spz2glb.exe

# Linux
curl -L https://github.com/spz-ecosystem/spz2glb/releases/download/v1.0.1/spz2glb-linux-x64 -o spz2glb
chmod +x spz2glb

# macOS
curl -L https://github.com/spz-ecosystem/spz2glb/releases/download/v1.0.1/spz2glb-macos-x64 -o spz2glb
chmod +x spz2glb
```

### 步骤 2: 准备 SPZ 文件（1 分钟）

确保你有一个 `.spz` 文件，例如：
- `gaussian_splat.spz` - SPZ 压缩的高斯泼溅数据
- `scene.spz` - SPZ 压缩的 3D 场景

### 步骤 3: 转换（10 秒）

```bash
# 基本用法
spz2glb input.spz output.glb

# 示例
spz2glb gaussian_splat.spz gaussian_splat.glb
```

### 步骤 4: 验证（2 分钟）

```bash
# 三层完整验证
spz_verify all input.spz output.glb

# 快速验证（仅 Layer 1）
spz_verify layer1 output.glb

# 无损验证（Layer 1 + Layer 2）
spz_verify lossless input.spz output.glb
```

### 步骤 5: 查看（1 分钟）

使用支持 glTF 2.0 的查看器打开 GLB 文件：

- **Three.js Viewer**: https://threejs.org/editor/
- **Babylon.js Sandbox**: https://sandbox.babylonjs.com/
- **glTF Viewer**: https://gltf-viewer.donmccurdy.com/

## 📝 完整示例

```bash
# 1. 下载示例文件（如果有）
curl -O https://example.com/sample.spz

# 2. 转换为 GLB
spz2glb sample.spz sample.glb

# 3. 验证转换质量
spz_verify all sample.spz sample.glb

# 4. 查看结果
# 在浏览器中打开 sample.glb
```

## 🎯 常用命令

### 转换器

```bash
# 基本转换
spz2glb input.spz output.glb

# 显示帮助
spz2glb --help

# 显示版本
spz2glb --version

# 详细输出
spz2glb -v input.spz output.glb
```

### 验证器

```bash
# 完整验证
spz_verify all input.spz output.glb

# 仅结构验证
spz_verify layer1 output.glb

# 无损验证
spz_verify lossless input.spz output.glb

# 解码验证
spz_verify decode input.spz output.glb

# 显示帮助
spz_verify --help
```

## 💡 提示

1. **文件扩展名**: 确保输入文件使用 `.spz` 扩展名
2. **输出路径**: GLB 文件可以放在任意目录
3. **验证顺序**: 建议始终运行完整验证确保质量
4. **批量处理**: 多个文件使用脚本批量处理（见 [Batch Processing](Batch-Processing)）

## ⚠️ 注意事项

- **文件大小**: SPZ 文件可能很大（几百 MB 到几 GB），确保有足够磁盘空间
- **内存**: 转换大文件可能需要大量内存
- **时间**: 首次转换可能需要几分钟，取决于文件大小

## 🆘 遇到问题？

```bash
# 检查工具是否正常
spz2glb --version

# 查看详细错误
spz2glb -v input.spz output.glb

# 验证 SPZ 文件
spz_verify layer1 input.spz

# 查看完整文档
# https://github.com/spz-ecosystem/spz2glb/wiki
```

## 🔗 下一步

- [安装指南](Installation) - 详细安装步骤
- [使用方法](Usage) - 完整命令行参考
- [三层验证](Verification) - 验证工具详解
- [故障排除](Troubleshooting) - 常见问题解决

---

**最后更新**: 2026-02-28  
**版本**: v1.0.1

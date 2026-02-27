# spz2glb - SPZ to GLB Converter

将 SPZ (Gaussian Splatting Compression) 文件转换为 glTF 2.0 GLB 格式的工具。

## 特性

- **SPZ 转 GLB**: 支持将压缩的 SPZ 文件转换为标准 GLB 格式
- **KHR_gaussian_splatting_compression_spz_2**: 集成 SPZ_2 压缩扩展
- **无损转换**: 压缩流模式，保持原始 SPZ 数据完整
- **跨平台**: 支持 Windows、Linux、macOS (x64 + ARM)
- **自动构建**: GitHub Actions 提供预编译二进制

## 安装

### 方式一：下载预编译版本

从 [Releases](https://github.com/spz-ecosystem/spz2glb/releases) 下载对应平台的二进制文件：

- Windows: `spz2glb-windows-x64.exe`
- Linux: `spz2glb-linux-x64`
- macOS x64: `spz2glb-macos-x64`
- macOS ARM: `spz2glb-macos-arm64`

### 方式二：从源码编译

```bash
# 克隆仓库
git clone https://github.com/spz-ecosystem/spz2glb.git
cd spz2glb

# 创建构建目录
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 编译
cmake --build build --config Release -j$(nproc)

# 运行
./build/spz2glb <input.spz> <output.glb>
```

### 依赖

- CMake 3.15+
- C++20 编译器
- ZLIB

## 使用方法

```bash
spz2glb input.spz output.glb
```

### 示例

```bash
# 转换文件
./spz2glb model.spz model.glb

# 输出
[INFO] Loading SPZ: model.spz
[INFO] SPZ version: 2
[INFO] Num points: 100000
[INFO] SH degree: 3
[INFO] SPZ size (raw compressed): 15 MB
[INFO] Creating glTF Asset with KHR extensions
[INFO] Exporting GLB...
[SUCCESS] GLB exported: model.glb
[INFO] GLB size: 16 MB
```

## 三层验证

项目包含完整的验证脚本确保转换的正确性：

### Layer 1: GLB 结构验证

验证生成的 GLB 文件符合规范：

```bash
python layer1_validate.py output.glb
```

验证项：
- Magic number (glTF)
- 版本号 (2)
- extensionsUsed 包含 KHR_gaussian_splatting 和 KHR_gaussian_splatting_compression_spz_2
- buffers 配置正确
- 压缩流模式 (attributes 为空)

### Layer 2: 二进制无损验证

确保 SPZ 数据在转换过程中没有丢失：

```bash
python layer2_lossless.py input.spz output.glb ./spz2glb
```

验证原始 SPZ 文件和从 GLB 提取的数据 MD5 完全一致。

### Layer 3: 解码一致性验证

验证 GLB 结构和扩展完整性：

```bash
python layer3_decode_verify.py input.spz output.glb
```

## 技术细节

### 压缩流模式

本工具使用 SPZ_2 规范的压缩流模式：
- SPZ 压缩数据直接存储在 bufferView 中
- 不定义 accessors 或 attributes
- 需要 SPZ 解码器的渲染器才能解析

优势：
- **无损**: 不重新编码，直接复制 SPZ 流
- **最小体积**: SPZ 压缩率约 10x
- **最快加载**: 无需额外编解码开销

### GLB 结构

```
GLB Header (12 bytes)
├── magic: 0x46546C67 ("glTF")
├── version: 2
└── length: total file size

JSON Chunk
├── chunkLength
├── chunkType: 0x4E4F534A ("JSON")
└── glTF JSON ( padded to 4-byte boundary)
    └── KHR_gaussian_splatting_compression_spz_2 extension

BIN Chunk
├── chunkLength
├── chunkType: 0x004E4942 ("BIN\0")
└── Raw SPZ compressed data
```

## 项目结构

```
spz2glb/
├── CMakeLists.txt          # 构建配置
├── LICENSE                 # MIT 许可证
├── src/
│   └── spz_to_glb.cpp      # 主程序源码
├── third_party/           # 依赖 (fastgltf + simdjson)
├── layer1_validate.py      # GLB 结构验证
├── layer2_lossless.py      # 二进制无损验证
├── layer3_decode_verify.py # 解码一致性验证
└── .github/
    └── workflows/
        └── release.yml     # CI/CD 工作流
```

## 许可证

MIT License - 详见 [LICENSE](LICENSE)

## 相关项目

- [fastgltf](https://github.com/spycrab/fastgltf) - 高性能 glTF 库
- [simdjson](https://github.com/simdjson/simdjson) - 极速 JSON 解析库
- [KHR_gaussian_splatting](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_gaussian_splatting) - Khronos Gaussian Splatting 扩展

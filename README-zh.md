# spz2glb - SPZ 转 GLB 转换器

将 SPZ（高斯溅射压缩）文件转换为 glTF 2.0 GLB 格式的工具。

## 功能特点

- **SPZ 转 GLB**: 将压缩的 SPZ 文件转换为标准 GLB 格式
- **KHR_gaussian_splatting_compression_spz_2**: 集成 SPZ_2 压缩扩展
- **无损转换**: 压缩流模式保留原始 SPZ 数据完整性
- **跨平台**: 支持 Windows、Linux、macOS (x64 + ARM)
- **WebAssembly**: WASM 版本可直接在浏览器中运行
- **自动化构建**: 通过 GitHub Actions 预编译二进制文件
- **三层验证**: 完整的 C++ 验证工具确保转换正确

## 下载

最新版本: [https://github.com/spz-ecosystem/spz2glb/releases](https://github.com/spz-ecosystem/spz2glb/releases)

| 平台 | 文件名 | 架构 |
|------|--------|------|
| Windows | `spz2glb-windows-x64.exe` | x64 |
| Linux | `spz2glb-linux-x64` | x64 |
| macOS | `spz2glb-macos-x64` | x64 |
| **WebAssembly** | `spz2glb.js` + `index.html` | WASM |

### 网页演示

**重要**: WASM 版本需要下载**两个文件**:
1. `spz2glb.js`
2. `index.html`

将两个文件放在同一目录，然后在浏览器中打开 `index.html`。

## 快速开始

### 下载预编译二进制

1. 前往 [Releases](https://github.com/spz-ecosystem/spz2glb/releases)
2. 下载对应平台的二进制文件
3. 运行:

```bash
# 转换 SPZ 到 GLB
./spz2glb model.spz model.glb

# 验证转换结果
./spz_verify all model.spz model.glb
```

### 从源码构建

```bash
# 克隆仓库
git clone https://github.com/spz-ecosystem/spz2glb.git
cd spz2glb

# 构建
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)

# 运行
./build/spz2glb input.spz output.glb
```

**平台依赖**:

```bash
# Ubuntu/Debian
sudo apt-get install -y zlib1g-dev

# macOS
brew install zlib

# Windows - 无需手动安装（CI 使用 vcpkg）
```

## WebAssembly 构建

```bash
# 安装 Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

# 构建 WASM
cd spz2glb
emcmake cmake -B build_wasm -DSPZ2GLB_BUILD_WASM=ON -DSPZ2GLB_USE_EMSCRIPTEN_ZLIB=ON
emmake cmake --build build_wasm --config Release --target spz2glb-wasm

# 输出在 build_wasm/dist/
```

## 使用方法

### 转换器 (spz2glb)

```bash
spz2glb <输入.spz> <输出.glb>
```

**示例**:

```bash
# 转换单个文件
./spz2glb model.spz model.glb

# 批量转换
for file in *.spz; do
    ./spz2glb "$file" "${file%.spz}.glb"
done
```

### 验证工具 (spz_verify)

```bash
# 运行三层验证
spz_verify all <输入.spz> <输出.glb>

# 运行单层验证
spz_verify layer1 <输出.glb>              # GLB 结构验证（快速）
spz_verify layer2 <输入.spz> <输出.glb>  # 无损二进制验证（MD5，较慢）
spz_verify layer3 <输入.spz> <输出.glb>  # 解码一致性验证（快速）
```

## 输出示例

```
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

## 技术细节

### 压缩流模式

使用 SPZ_2 规范的压缩流模式:
- SPZ 压缩数据直接存储在 bufferView 中
- 未定义 accessors 或 attributes
- 需要支持 SPZ 解码的渲染器

**优势**:
- **无损**: 无需重新编码，直接复制 SPZ 流
- **最小体积**: SPZ 压缩率约 10x
- **最快加载**: 无额外编解码器开销

### GLB 结构

```
GLB 文件头 (12 字节)
├── magic: 0x46546C67 ("glTF")
├── version: 2
└── length: 文件总大小

JSON Chunk
├── chunkLength
├── chunkType: 0x4E4F534A ("JSON")
└── glTF JSON
    └── KHR_gaussian_splatting_compression_spz_2 扩展

BIN Chunk
├── chunkLength
├── chunkType: 0x004E4942 ("BIN\0")
└── 原始 SPZ 压缩数据
```

## 依赖

- CMake 3.15+
- C++17 编译器
- ZLIB

| 工具 | 依赖 |
|------|------|
| spz2glb | ZLIB, fastgltf, simdjson |
| spz_verify | 仅 ZLIB |

## 项目结构

```
spz2glb/
├── CMakeLists.txt          # 构建配置
├── LICENSE                 # MIT 许可证
├── README.md               # 英文文档
├── README-zh.md           # 中文文档
├── src/
│   ├── spz_to_glb.cpp     # 转换器源码
│   ├── spz_verify.cpp     # 验证工具源码
│   └── emscripten_utils.h # Emscripten 工具（用于 WASM）
├── third_party/           # 依赖库
│   └── (fastgltf, simdjson)
├── dist/                   # WASM 构建输出
└── .github/
    └── workflows/
        └── release.yml    # CI/CD 工作流
```

## 免责声明

**这是一个个人独立开发项目。**

- 本项目由作者以个人身份独立开发
- 本项目**不隶属于**任何大学、机构或雇主
- 本项目**不是**雇佣工作或机构教学成果
- 本项目中表达的观点和意见仅代表作者本人
- 采用 MIT 许可证 - 详见 [LICENSE](LICENSE)

## 许可证

MIT 许可证 - 详见 [LICENSE](LICENSE)

## 相关项目

- [fastgltf](https://github.com/spycrab/fastgltf) - 高性能 glTF 库
- [simdjson](https://github.com/simdjson/simdjson) - 超快速 JSON 解析库
- [KHR_gaussian_splatting](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_gaussian_splatting) - Khronos 高斯溅射扩展

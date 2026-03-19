# spz2glb - SPZ to GLB Converter

A tool to convert SPZ (Gaussian Splatting Compression) files to glTF 2.0 GLB format.

## Features

- **SPZ to GLB**: Convert compressed SPZ files to standard GLB format
- **KHR_gaussian_splatting_compression_spz_2**: Integrated SPZ_2 compression extension
- **Lossless Conversion**: Compression stream mode preserves original SPZ data integrity
- **Cross-Platform**: Supports Windows, Linux, macOS (x64 + ARM)
- **WebAssembly**: Run in browser with WASM build
- **Automated Builds**: Pre-compiled binaries via GitHub Actions
- **Three-Layer Verification**: Complete C++ verification tools ensure correct conversion

## Downloads

Get the latest release from: [https://github.com/spz-ecosystem/spz2glb/releases](https://github.com/spz-ecosystem/spz2glb/releases)

| Platform | File | Architecture |
|----------|------|--------------|
| Windows | `spz2glb-windows-x64.exe` | x64 |
| Linux | `spz2glb-linux-x64` | x64 |
| macOS | `spz2glb-macos-x64` | x64 |
| **WebAssembly** | `spz2glb.js` + `index.html` | WASM |

### Web Demo

**Important**: For the WASM version, you must download **both** files:
1. `spz2glb.js`
2. `index.html`

Place them in the same directory and open `index.html` in your browser.

## Quick Start

### Download Pre-compiled Binaries

1. Go to [Releases](https://github.com/spz-ecosystem/spz2glb/releases)
2. Download the binary for your platform
3. Run:

```bash
# Convert SPZ to GLB
./spz2glb model.spz model.glb

# Verify conversion
./spz_verify all model.spz model.glb
```

### Build from Source

```bash
# Clone the repository
git clone https://github.com/spz-ecosystem/spz2glb.git
cd spz2glb

# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)

# Run
./build/spz2glb input.spz output.glb
```

**Platform Dependencies**:

```bash
# Ubuntu/Debian
sudo apt-get install -y zlib1g-dev

# macOS
brew install zlib

# Windows - No manual installation required (vcpkg in CI)
```

## WebAssembly Build

```bash
# Install Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

# Build WASM
cd spz2glb
emcmake cmake -B build_wasm -DSPZ2GLB_BUILD_WASM=ON -DSPZ2GLB_USE_EMSCRIPTEN_ZLIB=ON
emmake cmake --build build_wasm --config Release --target spz2glb-wasm

# Output in build_wasm/dist/
```

## Usage

### Converter (spz2glb)

```bash
spz2glb <input.spz> <output.glb>
```

**Example**:

```bash
# Convert a single file
./spz2glb model.spz model.glb

# Batch conversion
for file in *.spz; do
    ./spz2glb "$file" "${file%.spz}.glb"
done
```

### Verification Tool (spz_verify)

```bash
# Run all three layers of verification
spz_verify all <input.spz> <output.glb>

# Run individual layer verification
spz_verify layer1 <output.glb>              # GLB structure validation (fast)
spz_verify layer2 <input.spz> <output.glb>  # Lossless binary validation (MD5, slower)
spz_verify layer3 <input.spz> <output.glb>  # Decode consistency validation (fast)
```

## Output Example

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

## Technical Details

### Compression Stream Mode

Uses SPZ_2 specification compression stream mode:
- SPZ compressed data stored directly in bufferView
- No accessors or attributes defined
- Requires renderer with SPZ decoder

**Advantages**:
- **Lossless**: No re-encoding, direct copy of SPZ stream
- **Minimal Size**: SPZ compression ratio ~10x
- **Fastest Loading**: No additional codec overhead

### GLB Structure

```
GLB Header (12 bytes)
├── magic: 0x46546C67 ("glTF")
├── version: 2
└── length: total file size

JSON Chunk
├── chunkLength
├── chunkType: 0x4E4F534A ("JSON")
└── glTF JSON
    └── KHR_gaussian_splatting_compression_spz_2 extension

BIN Chunk
├── chunkLength
├── chunkType: 0x004E4942 ("BIN\0")
└── Raw SPZ compressed data
```

## Dependencies

- CMake 3.15+
- C++17 compiler
- ZLIB

| Tool | Dependencies |
|------|-------------|
| spz2glb | ZLIB, fastgltf, simdjson |
| spz_verify | ZLIB only |

## Project Structure

```
spz2glb/
├── CMakeLists.txt          # Build configuration
├── LICENSE                 # MIT License
├── README.md               # English documentation
├── README-zh.md           # Chinese documentation
├── src/
│   ├── spz_to_glb.cpp     # Converter source code
│   ├── spz_verify.cpp      # Verification tool source code
│   └── emscripten_utils.h  # Emscripten utilities (for WASM)
├── third_party/            # Dependencies
│   └── (fastgltf, simdjson)
├── dist/                   # WASM build output
└── .github/
    └── workflows/
        └── release.yml    # CI/CD workflow
```

## Disclaimer

**This is a personal independent development project.**

- This project is developed independently by the author in their personal capacity
- This project is **not affiliated** with any university, institution, or employer
- This is **not a work-for-hire** or institutional teaching achievement
- The views and opinions expressed in this project are solely those of the author
- MIT License applies - see [LICENSE](LICENSE) for details

## License

MIT License - See [LICENSE](LICENSE) for details

## Related Projects

- [fastgltf](https://github.com/spycrab/fastgltf) - High-performance glTF library
- [simdjson](https://github.com/simdjson/simdjson) - Ultra-fast JSON parsing library
- [KHR_gaussian_splatting](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_gaussian_splatting) - Khronos Gaussian Splatting Extension

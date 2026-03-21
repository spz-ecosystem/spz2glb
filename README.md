# spz2glb - SPZ to GLB Converter

A tool to convert SPZ (Gaussian Splatting Compression) files to glTF 2.0 GLB format.

## 📚 Documentation

- **Wiki**: https://github.com/spz-ecosystem/spz2glb/wiki
  - [Installation Guide](https://github.com/spz-ecosystem/spz2glb/wiki/Installation)
  - [Quick Start](https://github.com/spz-ecosystem/spz2glb/wiki/Quick-Start)
  - [Usage](https://github.com/spz-ecosystem/spz2glb/wiki/Usage)
  - [Three-Layer Verification](https://github.com/spz-ecosystem/spz2glb/wiki/Verification)
  - [Batch Processing](https://github.com/spz-ecosystem/spz2glb/wiki/Batch-Processing)
  - [Performance Optimization](https://github.com/spz-ecosystem/spz2glb/wiki/Performance)
  - [Troubleshooting](https://github.com/spz-ecosystem/spz2glb/wiki/Troubleshooting)
  - [FAQ](https://github.com/spz-ecosystem/spz2glb/wiki/FAQ)
  - [Building Guide](https://github.com/spz-ecosystem/spz2glb/wiki/Building)
  - [Contributing](https://github.com/spz-ecosystem/spz2glb/wiki/Contributing)

## Features

- **SPZ to GLB**: Convert compressed SPZ files to standard GLB format
- **KHR_gaussian_splatting_compression_spz_2**: Integrated SPZ_2 compression extension
- **Lossless Conversion**: Compression stream mode preserves original SPZ data integrity
- **Cross-Platform**: Supports Windows, Linux, macOS (x64 + ARM)
- **Automated Builds**: Pre-compiled binaries via GitHub Actions
- **Three-Layer Verification**: Complete C++ verification tools ensure correct conversion

## 🎬 Demo

### Basic Conversion

```bash
# Convert SPZ to GLB
./build/spz2glb model.spz model.glb
```

### Three-Layer Verification

```bash
# Run all verifications (provide your own SPZ and GLB files)
./build/spz_verify all input.spz output.glb

# Output:
# Layer 1: GLB Structure & SPZ_2 Specification Validation - PASSED (7/7)
# Layer 2: Binary Lossless Verification - PASSED (100% MD5 match)
# Layer 3: Decoding Consistency Verification - PASSED (Size match)
# [SUCCESS] All verifications PASSED!
```

> **Note**: Paths should be relative or absolute paths to your files. Do not use hardcoded paths.

> **Demo**: Demo videos will be released after the first stable version. Stay tuned!

### Batch Processing

```bash
# Batch convert all SPZ files
for file in *.spz; do
    ./build/spz2glb "$file" "${file%.spz}.glb"
done
```

## Quick Start

### Option 1: Download Pre-compiled Binaries

Download binaries for your platform from [Releases](https://github.com/spz-ecosystem/spz2glb/releases):

- Windows: `spz2glb-windows-x64.exe`
- Linux: `spz2glb-linux-x64`
- macOS: `spz2glb-macos-x64`

### Option 2: Build from Source (One-Click Build)

```bash
# 1. Clone the repository
git clone https://github.com/spz-ecosystem/spz2glb.git
cd spz2glb

# 2. One-click build (handles all dependencies automatically)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)

# 3. Run
./build/spz2glb input.spz output.glb
```

**Platform-Specific Dependencies** (install before building):

```bash
# Ubuntu/Debian
sudo apt-get install -y zlib1g-dev

# macOS
brew install zlib

# Windows
# No manual installation required, CI uses vcpkg to install automatically
```

## Usage

### Converter (spz2glb)

```bash
spz2glb <input.spz> <output.glb>
```

**Complete Examples**:

```bash
# Convert a single file
./build/spz2glb model.spz model.glb

# Batch conversion
for file in *.spz; do
    ./build/spz2glb "$file" "${file%.spz}.glb"
done
```

**Output Example**:

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

### Three-Layer Verification Tool (spz_verify)

> **Important Notes**:
> - **Independent Tool**: spz_verify is a standalone verification tool, NOT part of the production conversion pipeline
> - **Development/Testing Use**: Designed for quality assurance, debugging, and testing workflows
> - **Not Required for Daily Use**: Once conversion is verified, you only need spz2glb for production
> - **Layer 2 Performs Decompression**: Layer 2 verification extracts and decompresses data to compute MD5 hashes (slower than Layer 1/3)

```bash
spz_verify <command> [options]
```

**Commands**:

```bash
# Run all three layers of verification
spz_verify all <input.spz> <output.glb>

# Run individual layer verification
spz_verify layer1 <output.glb>              # GLB structure validation (fast)
spz_verify layer2 <input.spz> <output.glb>  # Lossless binary validation (MD5, slower)
spz_verify layer3 <input.spz> <output.glb>  # Decode consistency validation (fast)
```

**Complete Examples**:

```bash
# 1. Convert file
./build/spz2glb model.spz model.glb

# 2. Run all verifications
./build/spz_verify all model.spz model.glb

# Or verify individually
./build/spz_verify layer1 model.glb
./build/spz_verify layer2 model.spz model.glb
./build/spz_verify layer3 model.spz model.glb
```

**Verification Output**:

```
Layer 1: GLB Structure Validation
  ✓ Magic number: 0x46546C67 ("glTF")
  ✓ Version: 2
  ✓ extensionsUsed contains KHR_gaussian_splatting
  ✓ extensionsUsed contains KHR_gaussian_splatting_compression_spz_2
  ✓ buffers configuration correct
  ✓ Compression stream mode (attributes empty)
  [PASS] Layer 1 validation passed

Layer 2: Lossless Binary Validation
  ✓ Original SPZ MD5: abc123...
  ✓ Extracted data MD5: abc123...
  ✓ MD5 match confirmed
  [PASS] Layer 2 validation passed

Layer 3: Decode Consistency Validation
  ✓ GLB structure valid
  ✓ Extension integrity check passed
  [PASS] Layer 3 validation passed

[SUCCESS] All 3 layers validation passed!
```

## Automated Verification Script (Recommended)

Create `verify.sh` or `verify.bat` script to automate conversion + verification:

**Linux/macOS** (`verify.sh`):

```bash
#!/bin/bash
set -e

if [ $# -ne 1 ]; then
    echo "Usage: $0 <input.spz>"
    exit 1
fi

INPUT="$1"
OUTPUT="${INPUT%.spz}.glb"
SPZ2GLB="./build/spz2glb"
VERIFY="./build/spz_verify"

echo "=== SPZ to GLB Conversion & Verification ==="
echo "Input:  $INPUT"
echo "Output: $OUTPUT"
echo ""

# Step 1: Convert
echo "[1/2] Converting SPZ to GLB..."
$SPZ2GLB "$INPUT" "$OUTPUT"
echo ""

# Step 2: Verify
echo "[2/2] Running 3-layer verification..."
$VERIFY all "$INPUT" "$OUTPUT"
echo ""

echo "=== Complete ==="
```

**Windows** (`verify.bat`):

```batch
@echo off
setlocal enabledelayedexpansion

if "%~1"=="" (
    echo Usage: %~0 ^<input.spz^>
    exit /b 1
)

set INPUT=%~1
set OUTPUT=%INPUT:.spz=.glb%
set SPZ2GLB=build\spz2glb.exe
set VERIFY=build\spz_verify.exe

echo === SPZ to GLB Conversion ^& Verification ===
echo Input:  %INPUT%
echo Output: %OUTPUT%
echo.

echo [1/2] Converting SPZ to GLB...
%SPZ2GLB% "%INPUT%" "%OUTPUT%"
echo.

echo [2/2] Running 3-layer verification...
%VERIFY% all "%INPUT%" "%OUTPUT%"
echo.

echo === Complete ===
```

**Using the Script**:

```bash
# Linux/macOS
chmod +x verify.sh
./verify.sh model.spz

# Windows
verify.bat model.spz
```

## WebAssembly Build

### Build WASM Version

```bash
# Install Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

# Build WASM modules
cd tools/spz_to_glb
emcmake cmake -B build_wasm -DSPZ2GLB_BUILD_WASM=ON -DSPZ2GLB_USE_EMSCRIPTEN_ZLIB=ON
emmake cmake --build build_wasm --config Release --target spz2glb-wasm
emmake cmake --build build_wasm --config Release --target spz_verify-wasm

# Output in build_wasm/dist/
# - spz2glb.js, spz2glb.wasm, spz2glb.data
# - spz_verify.js, spz_verify.wasm, spz_verify.data
```

### Web Usage

**Important**: For the WASM version, you must download **all** files:
- `spz2glb.js`
- `spz2glb.wasm`
- `spz2glb.data`

Place them in the same directory and load via HTTP server.

### JavaScript API

```javascript
// Load the module
const Module = await createSpz2GlbModule();

// Convert SPZ to GLB
const spzBuffer = new Uint8Array([...]);  // Your SPZ file data
const glbBuffer = Module.convertSpzToGlb(spzBuffer);

if (glbBuffer) {
    // Success: glbBuffer is Uint8Array
    console.log('Conversion successful!');
} else {
    // Failed
    console.error('Conversion failed');
}

// Get memory statistics (optional)
const stats = Module.getMemoryStats();
console.log(`Peak memory: ${stats.peak_usage / 1024 / 1024} MB`);
```

### spz_verify JavaScript API

```javascript
const verifyModule = await createSpzVerifyModule();

// Layer 1: GLB structure validation
const layer1Result = verifyModule.layer1ValidateGlbStructure(glbBuffer);

// Layer 2: Binary lossless verification
const layer2Result = verifyModule.layer2ValidateLossless(spzBuffer, glbBuffer);

// Layer 3: Decoding consistency
const layer3Result = verifyModule.layer3ValidateDecoding(spzBuffer, glbBuffer);
```

### WASM Memory Configuration

| Setting | Value | Description |
|---------|-------|-------------|
| INITIAL_MEMORY | 64MB | Initial heap size |
| MAXIMUM_MEMORY | 1GB | Maximum heap size |

### Performance Optimizations

The WASM build includes:
- **-O3 -flto**: Link-time optimization
- **-fno-exceptions**: No exception overhead
- **Memory pool**: Bump allocator for fast allocation
- **Hot object pool**: Fixed-size object reuse

## Dependencies

- CMake 3.15+
- C++17 compiler
- ZLIB (automatically installed via system package manager)

**Dependency Details**:

| Tool | Dependencies | Purpose |
|------|--------------|---------|
| spz2glb | ZLIB, fastgltf, simdjson | SPZ to GLB conversion |
| spz_verify | ZLIB only | Three-layer verification |

## Project Structure

```
spz2glb/
├── CMakeLists.txt          # Build configuration
├── LICENSE                 # MIT License
├── README.md               # English documentation
├── README-zh.md            # Chinese documentation
├── src/
│   ├── spz_to_glb.cpp     # Converter source code
│   └── spz_verify.cpp     # Three-layer verification tool source code
├── third_party/            # Customized fastgltf + simdjson
│   ├── CMakeLists.txt
│   ├── include/fastgltf/
│   ├── src/
│   └── deps/simdjson/     # simdjson v4.3.1 (built-in)
└── .github/
    └── workflows/
        └── release.yml    # CI/CD workflow
```

## Technical Details

### Compression Stream Mode

This tool uses SPZ_2 specification compression stream mode:
- SPZ compressed data stored directly in bufferView
- No accessors or attributes defined
- Requires renderer with SPZ decoder to parse

**Advantages**:
- **Lossless**: No re-encoding, direct copy of SPZ stream
- **Minimal Size**: SPZ compression ratio ~10x
- **Fastest Loading**: No additional codec overhead

**Compatibility Note**:
> Any SPZ-derived algorithm that is 100% compatible with the original SPZ format and strictly follows the SPZ_2 extension specification is perfectly supported by this converter.

### GLB Structure

```
GLB Header (12 bytes)
├── magic: 0x46546C67 ("glTF")
├── version: 2
└── length: total file size

JSON Chunk
├── chunkLength
├── chunkType: 0x4E4F534A ("JSON")
└── glTF JSON (padded to 4-byte boundary)
    └── KHR_gaussian_splatting_compression_spz_2 extension

BIN Chunk
├── chunkLength
├── chunkType: 0x004E4942 ("BIN\0")
└── Raw SPZ compressed data
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
- [simdjson](https://github.com/simdjson/simdjson) - Ultra-fast JSON parsing library v4.3.1
- [KHR_gaussian_splatting](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_gaussian_splatting) - Khronos Gaussian Splatting Extension

## Customization Notes

This project uses a **customized version of fastgltf** with the following modifications:

1. **simdjson v4.3.1 Built-in**: Does not search for system libraries, does not download from network, uses built-in source code
2. **KHR_gaussian_splatting_compression_spz_2 Extension**: Supports SPZ_2 compression format
3. **One-Click Build**: Just `cmake && cmake --build`, no manual dependency configuration required

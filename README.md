# spz2glb - SPZ to GLB Converter

**Lossless packaging of SPZ into GLB** — preserves SPZ compressed stream, with dual-scenario collaboration: lightweight web usage + heavy local workloads.

## Release Status

- Current stable line: **v2.x** (for exact version, see [Releases](https://github.com/spz-ecosystem/spz2glb/releases) and repository tags)
- Core positioning: **lossless packaging** (SPZ stream stored as-is in GLB)
- Key enhancement: WASM memory/API capabilities (reserved input, explicit release, stats, dual profile)
- Dual-end collaboration: scenario split — browser side for lightweight preview/quick checks, local CLI for heavy conversion, batch jobs, and deep verification
- Validation closure: built-in 5-layer verification (structure/lossless/decoding consistency/metadata consistency/ILV extension integrity) + cloud browser smoke

## Responsibility Boundary (Fixed)

- `spz2glb` is responsible for only two things: **SPZ→GLB format packaging** and **GLB delivery/distribution workflow**.
- `spz2glb` does not own compression algorithm R&D, rendering-engine capability expansion, or generic 3D editing pipelines.
- GLB compliance and correctness are judged by the **five-layer verification** system (structure / lossless / decoding consistency / metadata consistency / ILV extension integrity).
- The Web path is for lightweight single-file demos by default; batch and heavy workloads belong to the CLI path.

## Core Features

- **Lossless Packaging**: SPZ compressed stream stored as-is in GLB, 100% byte-level fidelity
- **SPZ_2 Extension**: Uses `KHR_gaussian_splatting_compression_spz_2` standard extension
- **Large-Scale Refactor (v2.0.3)**: Unified CLI/WASM core path, removed compile-time flag for KHR_gaussian_splatting (now always enabled per Khronos official extensions directory)
- **KHR Extension Compliance**: Full `KHR_gaussian_splatting` field serialization (`kernel`, `colorSpace`, `sortingMethod`, `projection`); nested `KHR_gaussian_splatting_compression_spz_2` with complete metadata (`spzVersion`, `compression`, `coordinateSystem`)
- **WASM Enhancements**: Reserved input, explicit output release, memory stats, compat/perf-lite dual profile
- **Dual-End Collaboration (scenario split)**: lightweight web interaction and fast feedback on browser side; batch, large-file, and heavy verification workflows on local CLI side
- **Five-Layer Verification**: Structure validation / lossless validation / decoding consistency / metadata consistency / ILV extension integrity
- **Cross-Platform**: Windows, Linux, macOS (x64 + ARM)
- **Zero Runtime Dependencies**: C++17 + WASM, no additional runtime dependencies

## Extension Support Status

### KHR_gaussian_splatting Extension
- **Status**: `KHR_gaussian_splatting` is currently **Release Candidate** — it has been merged into the Khronos official extensions directory (the `KHR_` prefix indicates intent to become a standard), pending ratification vote by the Khronos Board of Promoters. The extension is always enabled — no compile-time flag required.
- **`KHR_gaussian_splatting_compression_spz_2`**: Still in **draft** status.
- **Current Implementation**:
  - ✅ **Export**: Writes the full extension chain with required fields (`kernel`, `colorSpace`), optional properties (`sortingMethod`, `projection`), and nested `KHR_gaussian_splatting_compression_spz_2` with complete metadata (`bufferView`, `spzVersion`, `compression`, `coordinateSystem`).
  - ✅ **Layer 1 Validation**: Field-level checks ensure KHR_gaussian_splatting contains `kernel` and `colorSpace` in the GLB JSON.
  - ⚠️ **Parse**: Draft extensions are **safely ignored** during parsing, which is the expected fallback behavior.
- **Future Changes**: Full parse-time support can be added once the spz_2 extension is ratified.

### Compilation Control
- KHR_gaussian_splatting support is always enabled. The extension is part of the Khronos official extensions directory (Release Candidate), no compile-time flag required.

### ILV 003 Coordinate System Extension
- **Extension ID**: `0xADBE0003`
- **Purpose**: Maps coordinateSystem (uint32) metadata within SPZ ILV (Information-Label-Value) records.
- **Implementation**: spz2glb reads and forwards 003 metadata as descriptor (not instruction), leaving coordinate conversion to the renderer's `coordinateConverter()`.
- **Validation**: Layer 5 (ILV extension integrity) verifies TLV record structure and enforces 003 value range [0, 16].

## Comparison with `splat-transform`

> Note: this section describes **tool positioning differences**, not an absolute quality ranking.

| Dimension | `spz2glb` (v2.0.3) | `splat-transform` (v3.1.7) |
|-----------|-------------------|-----------------------------|
| **Developer** | Independent (Pu Junhan) | PlayCanvas |
| **Core positioning** | **Lossless SPZ→GLB packaging** (SPZ compressed stream preserved as-is) | **Multi-format splat conversion & editing** (decompress-rebuild pipeline) |
| **Language** | C++17 + WASM | TypeScript (ESM/CJS dual) |
| **GLB output** | `KHR_gaussian_splatting_compression_spz_2` extension (original SPZ stream, byte-identical) | Standard `KHR_gaussian_splatting` extension (decompressed float32 attributes) |
| **`spz_2` extension** | ✅ Supported (nested inside KHR_gaussian_splatting) | ❌ Not supported |
| **SPZ handling** | No decompression; SPZ stored as binary stream inside GLB | Pure JS reader (v2-v4, gzip/zstd) + @adobe/spz WASM writer |
| **Data fidelity** | 100% lossless (byte-level SPZ preservation in GLB) | Decompress-rebuild cycle; floating-point precision varies by attribute encoding |
| **Feature scope** | Focused on SPZ↔GLB conversion + 5-layer verification | 9 input formats, 12 output formats, transform/filter/merge/decimate/generate |
| **Streaming** | File-based (load full SPZ, output full GLB) | ChunkSource streaming pipeline (process 100M+ points without full load) |
| **GPU acceleration** | N/A (C++ CPU, no GPU dependency) | WebGPU for SOG compression, voxelization, rendering |
| **Runtime deps** | None (standalone binary + WASM) | Node.js ≥22, @adobe/spz WASM, webgpu |
| **WASM capabilities** | Reserved input, explicit release, memory stats, dual profile | SPZ write-only via @adobe/spz (read is pure JS) |
| **Validation** | Built-in 5-layer verification (structure/lossless/decode/metadata/ILV) + CI browser smoke | 45+ test files (format roundtrip, GLB conformance, CLI) |
| **Cross-platform** | Windows/Linux/macOS (x64 + ARM) native binaries | Cross-platform via Node.js |
| **License** | MIT | MIT |

### Usage Recommendations

| Scenario | Recommended Tool |
|----------|------------------|
| Need to **losslessly embed** SPZ in GLB, preserving original compressed stream byte-for-byte | `spz2glb` |
| Need GLB with directly renderable float32 Gaussian attributes (no SPZ decoder required) | `splat-transform` |
| Need multi-format batch processing (PLY/SOG/SPLAT/KSPLAT/SPZ/CSV/HTML/Voxel) | `splat-transform` |
| Need streaming/huge-scene processing (100M+ points) | `splat-transform` |
| Need splat transformation, filtering, merging, decimation, generation | `splat-transform` |
| Need GPU-accelerated operations (SOG compression, voxelization) | `splat-transform` |
| Need a focused, lightweight converter with release-grade verification | `spz2glb` |

## Basic Conversion

```bash
# Convert SPZ to GLB
spz2glb model.spz model.glb
```

### Five-Layer Verification

```bash
# Run all verifications (provide your own SPZ and GLB files)
spz_verify all input.spz output.glb

# Output:
# Layer 1: GLB Structure & KHR Extension Validation - PASSED
# Layer 2: Binary Lossless Verification - PASSED (byte-identical)
# Layer 3: Decoding Consistency Verification - PASSED (Size match)
# Layer 4: Metadata Consistency Verification - PASSED (SPZ↔GLB metadata)
# Layer 5: ILV Extension Integrity Verification - PASSED (TLV structure)
# [SUCCESS] All 5 verifications PASSED!
```

> **Note**: Use the actual executable path from your build output (for example `build/spz2glb` / `build/spz_verify`) or add binaries to `PATH`.

### Batch Processing

```bash
# Batch convert all SPZ files
for file in *.spz; do
    spz2glb "$file" "${file%.spz}.glb"
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

# 3. Run (use actual binary path or PATH command)
spz2glb input.spz output.glb
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
spz2glb <input.spz> <output.glb> [--verify]
```

**Flags**:

| Flag | Description |
|------|-------------|
| `--verify` | Run 5-layer verification immediately after conversion (invokes spz_verify internally) |

**Complete Examples**:

```bash
# Convert a single file
spz2glb model.spz model.glb

# Convert and verify
spz2glb model.spz model.glb --verify

# Batch conversion
for file in *.spz; do
    spz2glb "$file" "${file%.spz}.glb"
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

### Five-Layer Verification Tool (spz_verify)

> **Important Notes**:
> - **Independent Tool**: spz_verify is a standalone verification tool, NOT part of the production conversion pipeline
> - **Development/Testing Use**: Designed for quality assurance, debugging, and testing workflows
> - **Not Required for Daily Use**: Once conversion is verified, you only need spz2glb for production
> - **Layer 2 Performs Byte-Level Comparison**: Layer 2 verification extracts the SPZ payload from GLB and performs byte-by-byte comparison with the original SPZ file (slower than Layer 1/3/4/5)

```bash
spz_verify <command> [options]
```

**Commands**:

```bash
# Run all five layers of verification
spz_verify all <input.spz> <output.glb>

# Run individual layer verification
spz_verify layer1 <output.glb>              # GLB structure validation (fast)
spz_verify layer2 <input.spz> <output.glb>  # Lossless binary validation (byte-by-byte comparison, slower)
spz_verify layer3 <input.spz> <output.glb>  # Decode consistency validation (fast)
spz_verify layer4 <input.spz> <output.glb>  # Metadata consistency validation (fast)
spz_verify layer5 <input.spz>              # ILV extension integrity validation (fast)
```

**Complete Examples**:

```bash
# 1. Convert file
spz2glb model.spz model.glb

# 2. Run all verifications
spz_verify all model.spz model.glb

# Or verify individually
spz_verify layer1 model.glb
spz_verify layer2 model.spz model.glb
spz_verify layer3 model.spz model.glb
spz_verify layer4 model.spz model.glb
spz_verify layer5 model.spz
```

**Verification Output**:

```
Layer 1: GLB Structure & KHR Extension Validation
  ✓ Magic number: 0x46546C67 ("glTF")
  ✓ Version: 2
  ✓ extensionsUsed contains KHR_gaussian_splatting
  ✓ extensionsUsed contains KHR_gaussian_splatting_compression_spz_2
  ✓ extensionsRequired contains KHR_gaussian_splatting
  ✓ extensionsRequired contains KHR_gaussian_splatting_compression_spz_2
  ✓ KHR_gaussian_splatting has 'kernel' field
  ✓ KHR_gaussian_splatting has 'colorSpace' field
  ✓ compression.bufferView = 0
  ✓ bufferView.byteLength matches buffers[0].byteLength
  ✓ bufferView.byteOffset is 4-byte aligned
  ✓ bufferView inside buffers[0] range
  ✓ BIN chunk padding valid
  [PASS] Layer 1 validation passed

Layer 2: Payload Extraction & Byte Equality
  ✓ SPZ input bytes: 15728640
  ✓ Extracted bytes: 15728640
  ✓ Extracted payload is byte-identical to input SPZ
  [PASS] Layer 2 validation passed

Layer 3: Decode Consistency Validation
  ✓ GLB structure valid
  ✓ Extension integrity check passed
  [PASS] Layer 3 validation passed

Layer 4: GLB Extension Metadata vs SPZ Header Consistency
  ✓ SPZ version consistent: 2
  ✓ GLB coordinateSystem recorded in metadata
  [PASS] Layer 4 validation passed

Layer 5: ILV Extension Completeness
  ✓ ILV 0xADBE0003 coordinateSystem=1 (valid, range [0,16])
  ✓ All ILV records pass integrity checks
  [PASS] Layer 5 validation passed

[SUCCESS] All 5 layers validation passed!
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
SPZ2GLB="spz2glb"
VERIFY="spz_verify"

echo "=== SPZ to GLB Conversion & Verification ==="
echo "Input:  $INPUT"
echo "Output: $OUTPUT"
echo ""

# Step 1: Convert
echo "[1/2] Converting SPZ to GLB..."
$SPZ2GLB "$INPUT" "$OUTPUT"
echo ""

# Step 2: Verify
echo "[2/2] Running 5-layer verification..."
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

echo [2/2] Running 5-layer verification...
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
./emsdk install 6.0.3
./emsdk activate 6.0.3
source ./emsdk_env.sh

# Build WASM modules
cd tools/spz_to_glb
emcmake cmake -B build_wasm -DSPZ2GLB_BUILD_WASM=ON
emmake cmake --build build_wasm --config Release --target spz2glb-wasm
emmake cmake --build build_wasm --config Release --target spz_verify-wasm

# Output in build_wasm/dist/ (exact files depend on profile/toolchain)
# - spz2glb.js, spz2glb.wasm
# - spz_verify.js, spz_verify.wasm
# - optional side files may appear; deploy files from the same build together
```

### Web Usage

**Important**: Keep `spz2glb.js` and `spz2glb.wasm` from the same build output in the same directory and load via HTTP server. If your build also produces side files, deploy them together with the same version set.

### JavaScript API

```javascript
import { loadSpz2Glb } from './spz2glb_bindings.js';

const api = await loadSpz2Glb('./spz2glb.wasm');
const result = api.convert(spzUint8Array);

if (!result) throw new Error('Conversion failed');

const glbBytes = result.bytes;        // Uint8Array view on WASM memory
const glbBlob = result.toBlob('model/gltf-binary');
const stats = api.getMemoryStats();
console.log('Peak MB:', (stats.peakUsageBytes / 1024 / 1024).toFixed(2));

result.release(); // Required: release WASM output buffer
```

> Note: `spz_verify` is currently provided as CLI/WASM artifact for verification workflows. The README does not claim a stable browser JS API surface for it.

### WASM Memory Configuration

| Profile | INITIAL_MEMORY | ALLOW_MEMORY_GROWTH | MAXIMUM_MEMORY | Description |
|---------|----------------|---------------------|----------------|-------------|
| `compat` | 64MB | `1` | 1GB | Better compatibility on diverse devices |
| `perf-lite` | 128MB | `0` | N/A | Stable memory ceiling for light/medium files |

### Smart Memory Allocation (Browser + WASM)

The smart memory strategy is a 3-layer pipeline, not a single fixed threshold:

1. **Device tiering**: classify device capability (low/medium/high) from `navigator.deviceMemory`, `hardwareConcurrency`, and UA.
2. **File budgeting**: compute `recommendedMaxFileSize` and `hardMaxFileSize` from tier + browser heap limits (e.g. high tier defaults to 64MB hard cap).
3. **WASM-side reservation**: reserve one contiguous input region (`reserve_input`), stream file chunks into it, run `convert_reserved_input`, then explicitly release output buffers.

Goal: maximize stability across devices (avoid OOM/browser stalls) while keeping memory telemetry visible (current/peak usage, failures, workspace usage).

### Why WASM64 (memory64) is not enabled by default

`WASM64` is intentionally not enabled by default for stability/compatibility reasons:

- **ABI contract**: current JS bindings + C API assume 32-bit `size_t` (`size_t == 4`); switching to memory64 would break ABI expectations.
- **Browser/toolchain consistency**: the memory64 ecosystem is improving but still less uniform than wasm32 across browser versions/toolchains.
- **Product scope fit**: browser mode in this project targets lightweight preview and quick conversion; current wasm32 limits + compat profile are sufficient for this scope.

If large-file browser processing becomes a hard requirement, a separate WASM64 build target can be evaluated while preserving backward compatibility.

### Performance Optimizations

The WASM build includes:
- **-O3 + strict warnings**: Optimized build with warning-clean gate
- **-Oz**: WASM-specific size optimization (reduces .wasm binary size)
- **-fno-exceptions**: No exception overhead
- **compat/perf-lite dual profile**: Configurable memory behavior by runtime target
- **Memory pool**: Bump allocator for fast allocation
- **Hot object pool**: Fixed-size object reuse

> Example: `dunhuang_000000.spz` (24.78 MB) converts successfully in browser in about `506 ms`, with peak memory around `49.56 MB`.

![Browser conversion success screenshot](./docs/examples/images/dunhuang_000000_spz_web_success.png)

## Dependencies

- CMake 3.15+
- C++17 compiler
- ZLIB (automatically installed via system package manager)
- ZSTD (v4 SPZ format support)

**Dependency Details**:

| Tool | Dependencies | Purpose |
|------|--------------|---------|
| spz2glb | ZLIB, ZSTD, fastgltf, simdjson | SPZ to GLB conversion |
| spz_verify | ZLIB, ZSTD | Five-layer verification |

## Project Structure

```
spz2glb/
├── CMakeLists.txt              # Build configuration
├── LICENSE                     # MIT License
├── README.md / README-zh.md    # Documentation
├── src/
│   ├── spz2glb_core.cpp/.h     # Core conversion logic (v2.0.3 unified entry)
│   ├── spz2glb_wasm_c_api.cpp/.h  # WASM C API (reserve/release/stats)
│   ├── memory_pool.cpp/.h      # Memory pool and hot object pool
│   ├── mapped_file.h           # Cross-platform memory-mapped file reader (RAII)
│   ├── spz_to_glb.cpp          # CLI main entry (with --batch mode)
│   ├── spz_verify.cpp          # Verification tool main entry
│   ├── spz_verifier.cpp/.h     # Five-layer verification implementation
│   └── base64.{h,cpp}          # Base64 codec
├── third_party/                # Customized fastgltf + simdjson
│   ├── include/fastgltf/
│   ├── src/
│   └── deps/simdjson/         # simdjson v4.6.4 (built-in)
├── tests/
│   ├── gen_fixture.mjs           # Synthetic fixture generator
│   ├── data/
│   │   └── bench/                # Benchmark dataset
│   └── ...                       # Test scripts and fixtures
├── scripts/
│   ├── wasm-pre-check.sh         # WASM pre-build environment check
│   └── ...                       # Utility scripts
└── .github/workflows/            # CI/CD workflows
```

## Test Data

The `tests/` directory includes:

- **Synthetic fixtures** (`tests/gen_fixture.mjs`): Generates minimal valid SPZ files for unit tests, covering v3/v4 SPZ variants and edge cases (empty files, truncated headers, malformed extensions). v2 support requires extending the generator (currently not implemented).
- **Benchmark dataset** (`tests/data/bench/`): A set of representative SPZ files of varying sizes and compression profiles used for performance benchmarking and regression testing.

Both are regenerable and do not bundle real user data.

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

### GLB Structure Example

```
GLB Header (12 bytes)
├── magic: 0x46546C67 ("glTF")
├── version: 2
└── length: total file size

JSON Chunk
├── chunkLength
├── chunkType: 0x4E4F534A ("JSON")
└── glTF JSON (padded to 4-byte boundary)
    └── `KHR_gaussian_splatting` extension
        └── nested `KHR_gaussian_splatting_compression_spz_2` extension

BIN Chunk
├── chunkLength
├── chunkType: 0x004E4942 ("BIN\0")
└── Raw SPZ compressed data
```

## Author & Copyright

- Copyright owner: **Pu Junhan**
- Start year: **2026**

## Independent Work Statement

This project is developed independently by the author in their personal capacity and is not affiliated with any university, institution, or employer.

This project depends on the following public technical specifications:
- SPZ file format (Niantic, Inc.)
- glTF 2.0 / KHR_gaussian_splatting (Khronos Group)
- KHR_gaussian_splatting_compression_spz_2 extension draft (SPZ ecosystem public specification)

This project is not affiliated with, endorsed by, or connected to Niantic, Inc. or its affiliates.

**DOI**: [10.5281/zenodo.20849112](https://doi.org/10.5281/zenodo.20849112)

## Citation

If you use this project in your research, please cite:

> Pu Junhan. Zero-Trust HL Harness: Governing Autonomous Agents through Self-Referential Evolution. 中国科学院科技论文预发布平台, ChinaXiv:202607.00158V1.

## License

MIT License - See [LICENSE](LICENSE) for details

## Ecosystem Position

`spz2glb` is a **downstream project in the SPZ ecosystem**. Its relationship with the upstream `spz_gatekeeper` is:

- **Gatekeeper (upstream)**: Defines and enforces SPZ format legality, extension compatibility, and governance standards (L2 validation, TLV extension registry, compliance auditing).
- **`spz2glb` (downstream)**: Performs lossless packaging of compliant SPZ files into GLB, adhering to the compatibility constraints defined by the gatekeeper.

> In short: **gatekeeper governs "admission and standards"; `spz2glb` handles "conversion and delivery."**

Related projects:

- [spz_gatekeeper](https://github.com/spz-ecosystem/spz_gatekeeper) - SPZ Gatekeeper: format legality validation and ecosystem governance
- [spz-anime-text2scene-bench](https://github.com/spz-ecosystem/spz-anime-text2scene-bench) - Anime-style text-to-scene benchmark dataset in SPZ format
- [fastgltf](https://github.com/spnda/fastgltf) - High-performance glTF library (by Sean Apeler, MIT License)
- [simdjson](https://github.com/simdjson/simdjson) - Ultra-fast JSON parsing library v4.6.4
- [KHR_gaussian_splatting](https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_gaussian_splatting) - Khronos Gaussian Splatting Extension

## Customization Notes

This project uses a **customized version of fastgltf** (based on upstream [spnda/fastgltf](https://github.com/spnda/fastgltf)) with the following modifications:

1. **simdjson v4.6.4 Built-in**: Downloaded simdjson v4.6.4 single-header release from upstream and embedded as source code in `third_party/deps/simdjson/`. Does not search system libraries, does not download from network, does not rely on package managers — ensuring fully offline reproducible builds.
2. **KHR_gaussian_splatting struct fields**: Added `kernel` (default `"ellipse"`), `colorSpace` (default `"srgb_rec709_display"`), `sortingMethod` (optional, default `"cameraDistance"`), `projection` (optional, default `"perspective"`) to `GaussianSplatExtension`
3. **KHR_gaussian_splatting JSON serialization**: Writes full extension chain on export — outer `KHR_gaussian_splatting` with required/optional properties, nested `KHR_gaussian_splatting_compression_spz_2` with complete metadata (`bufferView`, `spzVersion`, `compression`, `coordinateSystem`)
4. **Compile flag removed**: Removed `FASTGLTF_ENABLE_KHR_GAUSSIAN_SPLATTING` conditional compilation — the extension is in the Khronos official extensions directory (Release Candidate), always enabled
5. **One-Click Build**: Just `cmake && cmake --build`, no manual dependency configuration required

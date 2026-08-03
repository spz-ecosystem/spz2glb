# spz2glb - SPZ to GLB Converter

**Lossless packaging of SPZ into GLB** — preserves SPZ compressed stream, with dual-scenario collaboration: lightweight web usage + heavy local workloads.

## Release Status

- Current stable line: **v2.x** (for exact version, see [Releases](https://github.com/spz-ecosystem/spz2glb/releases) and repository tags)
- Core positioning: **lossless packaging** (SPZ stream stored as-is in GLB)
- Key enhancement: WASM memory/API capabilities (reserved input, explicit release, stats, dual profile) + CLI queue/batch processing
- Dual-end collaboration: scenario split — browser side for lightweight preview/quick checks, local CLI for heavy conversion, batch jobs, queue processing, and deep verification
- Validation closure: built-in 5-layer verification (structure/lossless/decoding consistency/metadata consistency/ILV extension integrity) + cloud browser smoke + JSON report validation

## Responsibility Boundary (Fixed)

- `spz2glb` is responsible for only two things: **SPZ→GLB format packaging** and **GLB delivery/distribution workflow**.
- `spz2glb` does not own compression algorithm R&D, rendering-engine capability expansion, or generic 3D editing pipelines.
- Conversion correctness and GLB compliance are verified by the **five-layer verification** system + optional `--report` JSON report validation (structure / lossless / decoding consistency / metadata consistency / ILV extension integrity).
- The Web path targets lightweight interactive demos with queue support (max 2 concurrent); the CLI path handles batch, queue, and heavy verification workflows.

## Core Features

- **Lossless Packaging**: SPZ compressed stream stored as-is in GLB, 100% byte-level fidelity
- **SPZ_2 Extension**: Uses `KHR_gaussian_splatting_compression_spz_2` standard extension
- **Large-Scale Refactor (v2.0.4)**: Unified CLI/WASM core path, removed compile-time flag for KHR_gaussian_splatting (now always enabled per Khronos official extensions directory)
- **KHR Extension Compliance**: Full `KHR_gaussian_splatting` field serialization (`kernel`, `colorSpace`, `sortingMethod`, `projection`); nested `KHR_gaussian_splatting_compression_spz_2` with complete metadata (`spzVersion`, `compression`, `coordinateSystem`)
- **CLI Queue Processing**: Built-in file-system queue (`--queue-add`/`--queue`/`--queue-status`/`--queue-clear`) with pending/processing/done/failed state management
- **JSON Conversion Report**: Auto-generated JSON report per conversion with full SPZ/GLB/KHR metadata and timestamp
- **WASM Enhancements**: Reserved input, explicit output release, memory stats, compat/perf-lite dual profile + runtime performance panel (19-row telemetry)
- **Dual-End Collaboration (scenario split)**: lightweight web interaction and fast feedback on browser side; batch, queue, large-file, and heavy verification workflows on local CLI side
- **Five-Layer Verification**: Structure validation / lossless validation / decoding consistency / metadata consistency / ILV extension integrity + optional `--report` JSON report validation
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
- **v4 header alignment**: Layer 5 locates the ILV header zone via `tocByteOffset` in the SPZ v4 header, whose field layout is aligned with the upstream Niantic `NgspFileHeader` (32-byte) spec — identical to gatekeeper's verified parsing.

## Comparison with `splat-transform`

> Note: this section describes **tool positioning differences**, not an absolute quality ranking.

| Dimension | `spz2glb` (v2.0.5) | `splat-transform` (v3.1.7) |
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
# Option 1: shell loop (traditional)
for file in *.spz; do
    spz2glb "$file" "${file%.spz}.glb"
done

# Option 2: single-process batch (recommended, avoids repeated process startup overhead)
spz2glb --batch .spz --verify

# Option 3: queue processing (file-system queue with resume and report export)
spz2glb --queue-add scene1.spz scene2.spz scene3.spz
spz2glb --queue                             # serial conversion
spz2glb --queue-status                      # view queue state
# Per-file JSON reports written to queue/done/, validated by spz_verify --report
```

> CLI mode uses serial conversion. JSON report format is identical to the Web demo. Reports can be consumed by `spz_verify --report` on the CLI side.

## Quick Start

### Option 1: Download Pre-compiled Binaries

Download from [Releases](https://github.com/spz-ecosystem/spz2glb/releases):

- **CLI**: `spz2glb-windows-x64.exe` / `spz2glb-linux-x64` / `spz2glb-macos-x64` / `spz2glb-macos-arm64`
- **WASM**: `spz2glb-compat.js + .wasm` / `spz2glb-perf-lite.js + .wasm` (bundled Web demo)
- **Verification**: `spz_verify-compat.js + .wasm`

### Option 2: Build from Source

```bash
# 1. Clone and enter build directory
git clone https://github.com/spz-ecosystem/spz2glb.git
cd spz2glb/tools/spz_to_glb

# 2. One-click build (all dependencies handled automatically)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)

# 3. Run examples
./build/spz2glb input.spz output.glb --verify   # convert + 5-layer verify
./build/spz2glb --queue-add input.spz             # add to queue
./build/spz2glb --queue                           # process queue serially
./build/spz_verify all input.spz output.glb       # standalone verification
```

**Platform Dependencies** (install before building):

```bash
# Ubuntu/Debian
sudo apt-get install -y zlib1g-dev libzstd-dev

# macOS
brew install zlib zstd

# Windows
# No manual installation required; vcpkg handles dependencies automatically
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
| `--batch EXT` | Batch convert all files matching EXT (e.g. `.spz`) in a single process |
| `--queue-add` | Add file(s) to conversion queue (pending directory) |
| `--queue` | Process the queue (convert all pending files sequentially) |
| `--queue-status` | Show queue state (pending/processing count, done/failed history) |
| `--queue-clear` | Clear completed queue results |

**Complete Examples**:

```bash
# Convert a single file
spz2glb model.spz model.glb

# Convert and verify
spz2glb model.spz model.glb --verify

# Batch conversion (single process)
spz2glb --batch .spz --verify

# Queue workflow
spz2glb --queue-add scene1.spz scene2.spz scene3.spz
spz2glb --queue
spz2glb --queue-status
```

**Output Example** (single file):

```
[INFO] Converting to GLB...
[INFO] Writing GLB: model.glb
[SUCCESS] GLB exported: model.glb
[INFO] GLB size: 15.73 MB
```

With `--verify`, verification summary follows:

```
============================================================
Running Five-Layer Verification...
============================================================
...
============================================================
Summary:
  Layer 1 (GLB Structure): PASSED
  Layer 2 (Binary Lossless): PASSED
  Layer 3 (Decoding): PASSED
  Layer 4 (Metadata): PASSED
  Layer 5 (ILV Extension): PASSED
============================================================
[SUCCESS] All verifications PASSED!
```

**Queue Output Example**:

```
[QUEUE] 3 file(s) added to queue

[QUEUE] maxParallel=1, 3 file(s) pending
[QUEUE] Processing: scene1.spz
[QUEUE] Processing: scene2.spz
[QUEUE] Processing: scene3.spz

[QUEUE] Complete: 3 processed (3 success, 0 failed)
[QUEUE] Reports written to queue/done/
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

**Options**:

| Option | Description |
|--------|-------------|
| `--report <file.json>` | Validate conversion report JSON against actual conversion result |

**Commands**:

```bash
# Run all five layers of verification
spz_verify all <input.spz> <output.glb>

# Run all five layers with report validation
spz_verify all <input.spz> <output.glb> --report report.json

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
=== Layer 1: GLB Structure & KHR Extension Validation ===
[PASS] GLB header/chunks are structurally valid
[PASS] JSON chunk length=1024, padding=0 (4-byte aligned)
[PASS] BIN chunk length=15728640
[PASS] extensionsUsed contains KHR_gaussian_splatting
[PASS] extensionsUsed contains KHR_gaussian_splatting_compression_spz_2
[PASS] extensionsRequired contains KHR_gaussian_splatting
[PASS] extensionsRequired contains KHR_gaussian_splatting_compression_spz_2
[PASS] KHR_gaussian_splatting has 'kernel' field
[PASS] KHR_gaussian_splatting has 'colorSpace' field
[PASS] compression.bufferView=0
[PASS] bufferView.byteLength=15728640 matches buffers[0].byteLength=15728640
[PASS] bufferView.byteOffset is 4-byte aligned
[PASS] bufferView is inside buffers[0] range
[PASS] BIN chunk padding is within 0..3 bytes (actual=0)
[PASS] Layer 1 contract assertions passed

=== Layer 2: Payload Extraction & Byte Equality ===
SPZ input bytes: 15728640
Extracted bytes: 15728640
[PASS] extracted payload is byte-identical to input SPZ

=== Layer 3: Decoding Consistency & v4 Header/Trailer Checks ===
[PASS] SPZ header parsed from gzip payload
[PASS] SPZ version=3, numPoints=100000, flags=0x0
[PASS] non-v4 payload (v3) header checks complete
[PASS] decoding consistency checks complete

=== Layer 4: GLB Extension Metadata vs SPZ Header Consistency ===
[PASS] SPZ version consistent: 3
[INFO] GLB coordinateSystem=1 (recorded in metadata)
[PASS] Layer 4 metadata consistency checks passed

=== Layer 5: ILV Extension Completeness ===
[PASS] Not a v4 ZSTD SPZ, no ILV records expected
[PASS] Layer 5 ILV extension checks passed

============================================================
Summary:
  Layer 1 (GLB Structure): PASSED
  Layer 2 (Binary Lossless): PASSED
  Layer 3 (Decoding): PASSED
  Layer 4 (Metadata): PASSED
  Layer 5 (ILV Extension): PASSED
============================================================
[SUCCESS] All 5 verifications PASSED!
```

### JSON Conversion Report

Each successful conversion (CLI `--queue` or Web demo) produces a JSON report with full metadata:

```json
{
  "file": "model.spz",
  "sizeBytes": 15728640,
  "spz": {
    "version": 3,
    "compression": "gzip"
  },
  "glb": {
    "magic": "0x46546C67",
    "version": 2,
    "jsonChunkSize": 1024,
    "binChunkSize": 15728640,
    "totalSizeBytes": 15730688,
    "outputSizeBytes": 15729664
  },
  "extensionsUsed": [
    "KHR_gaussian_splatting",
    "KHR_gaussian_splatting_compression_spz_2"
  ],
  "extensionsRequired": [
    "KHR_gaussian_splatting"
  ],
  "KHR_gaussian_splatting": {
    "kernel": "3D_GAUSSIAN",
    "colorSpace": "SRGB",
    "sortingMethod": "SPZ_ORDER"
  },
  "KHR_gaussian_splatting_compression_spz_2": {
    "bufferView": 0,
    "spzVersion": 3,
    "compression": "gzip",
    "coordinateSystem": 1
  },
  "coordinateSystem": {
    "found": true,
    "extensionId": "0xADBE0003",
    "value": 1
  },
  "timestamp": "2026-07-30T15:00:00+0800",
  "generator": {
    "name": "spz2glb",
    "version": "2.0.5",
    "license": "MIT",
    "url": "https://github.com/spz-ecosystem/spz2glb"
  },
  "result": "success",
  "timingMs": 1523
}
```

> The report JSON can be validated by `spz_verify --report report.json`. The format is identical between CLI queue and Web demo, enabling cross-end report verification.

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

### Web Demo Features

The bundled `index.html` provides a full-featured demo:
- **Single-file conversion**: Upload one `.spz` file, download a single `.glb`
- **Multi-file queue** (`MAX_PARALLEL = 1`): Drag-drop or multi-select any number of files; they are queued and processed serially (SPZ→GLB). The WASM converter is single-instance (reserved input buffer + mutex), so concurrency is capped at 1; queued files wait without contaminating their conversion timing.
- **Supports v3 and v4**: Gzip-compressed (v3) and ZSTD-compressed (v4) SPZ files.
- **SPZ header pre-check**: Before converting, the JS-side `detectSpzVersion` reads the magic bytes (v3 `1F8B` / v4 `NGSP`) and rejects invalid files early — no wasted WASM conversion. The panel also shows the detected SPZ version (v3 gzip / v4 zstd) of the last conversion.
- **Segmented timing**: The performance panel splits WASM conversion time vs JS-side overhead, pinpointing where time actually goes.
- **i18n (zh/en) + dark theme**: Top toolbar language toggle and day/night theme switching, both persisted in localStorage.
- **Cold-start note**: The panel notes that the first conversion includes a cold start (v3 gzip first-run zlib decompression init) and later ones are faster.
- **Build/load timestamp + cache-busting**: The panel shows the CI build time and WASM load timestamp so you can confirm you are not on a stale cached binary.
- **Auto-download & queue auto-clean**: Completed conversions auto-download their GLB + JSON report and auto-clean the queue after a delay; finished items slide out.
- **JSON report export**: Each completed conversion produces a downloadable JSON report with full SPZ/GLB/KHR metadata and timestamp.
- **CLI compatibility**: Exported JSON reports can be validated by `spz_verify --report <file.json>`.
- **Runtime performance panel**: Auto-displayed telemetry (WASM version, device info, memory stats, alloc/free/fail counts, hot pool, work area usage, recommended file size limit, SPZ version).
- **Smart memory allocation**: Device-aware tiering automatically adjusts memory budget.

### JavaScript API

```javascript
import { loadSpz2Glb } from './spz2glb_bindings.js';

const api = await loadSpz2Glb('./spz2glb.wasm');
const result = await api.convert(spzUint8Array);

if (!result) throw new Error('Conversion failed');

const glbBytes = result.bytes;        // Uint8Array view on WASM memory
const glbBlob = result.toBlob('model/gltf-binary');

// Memory telemetry (also displayed in the built-in performance panel)
const stats = api.getMemoryStats();
console.log('Peak MB:', (stats.peakUsageBytes / 1024 / 1024).toFixed(2));

// The web demo also provides:
// - JSON conversion report with full SPZ/GLB/KHR metadata
// - Runtime performance stats panel (19 rows: WASM version, build/load timestamps,
//   SPZ version, segmented WASM vs JS timing, device tier, memory stats, hot pool, ...)
// - SPZ header pre-check (detectSpzVersion) + zh/en toggle + light/dark theme
// - Multi-file queue with drag-drop support and per-file download

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
- **Zero-copy output handling** (v2.0.5): `parseGlbJson` reads the WASM output handle's byte view directly and `toBlob()` creates the Blob from that view — removing a 26.6MB JS big-object copy that was a Major-GC timing jitter source.
- **Segmented timing** (v2.0.5): conversion time is split into WASM conversion vs JS-side overhead, so you can see where time actually goes.
- **Explicit serial queue** (v2.0.5): `MAX_PARALLEL=1` (the WASM converter is single-instance); queued files' waiting time is excluded from their conversion timing.
- **Runtime performance panel**: 19-row telemetry — WASM version, build time, load timestamp, last-conversion file, SPZ version (v3 gzip / v4 zstd), WASM conversion ms, total ms, JS-side overhead, conversion result, device tier, peak/current memory, alloc/free/fail counts, hot pool available, work area used/peak, recommended file size limit — plus a cold-start note and a max-parallel / recommended-file-size hint.

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
│   ├── spz2glb_core.cpp/.h     # Core conversion logic (v2.0.4 unified entry)
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

**DOI**: [10.5281/zenodo.20849134](https://doi.org/10.5281/zenodo.20849134)

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

## Future Plans

This project intends to donate to the **OpenAtom Foundation** at an appropriate stage, to foster broader community collaboration and governance of the SPZ ecosystem. The specific timing and method will be determined as the project matures to community-operation standards.

## Customization Notes

This project uses a **customized version of fastgltf** (based on upstream [spnda/fastgltf](https://github.com/spnda/fastgltf)) with the following modifications:

1. **simdjson v4.6.4 Built-in**: Downloaded simdjson v4.6.4 single-header release from upstream and embedded as source code in `third_party/deps/simdjson/`. Does not search system libraries, does not download from network, does not rely on package managers — ensuring fully offline reproducible builds.
2. **KHR_gaussian_splatting struct fields**: Added `kernel` (default `"ellipse"`), `colorSpace` (default `"srgb_rec709_display"`), `sortingMethod` (optional, default `"cameraDistance"`), `projection` (optional, default `"perspective"`) to `GaussianSplatExtension`
3. **KHR_gaussian_splatting JSON serialization**: Writes full extension chain on export — outer `KHR_gaussian_splatting` with required/optional properties, nested `KHR_gaussian_splatting_compression_spz_2` with complete metadata (`bufferView`, `spzVersion`, `compression`, `coordinateSystem`)
4. **Compile flag removed**: Removed `FASTGLTF_ENABLE_KHR_GAUSSIAN_SPLATTING` conditional compilation — the extension is in the Khronos official extensions directory (Release Candidate), always enabled
5. **One-Click Build**: Just `cmake && cmake --build`, no manual dependency configuration required

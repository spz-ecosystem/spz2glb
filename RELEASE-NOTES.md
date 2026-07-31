# spz2glb v2.0.4 Release Notes

**Release Date**: July 31, 2026  
**Tag**: v2.0.4  
**Type**: SPZ v4 Header Fix, CLI Queue, Web Mirror, macOS x64/ARM64 CI Matrix, WASM Fixes & Code Quality

## Overview

This release aligns the SPZ v4 header parsing with the upstream Niantic `NgspFileHeader` spec (fixing ILV `0xADBE0003` coordinate-system detection), adds a file-system based CLI queue with web mirror, splits the macOS CI matrix into Intel x64 + Apple Silicon ARM64 builds, fixes SPZ version detection and KHR report field naming, and strengthens code quality with enhanced WASM pre-check dead-code detection.

## Changes Since v2.0.3

### SPZ v4 Header Alignment (fix)

- **`SpzV4Header` byte offsets** in `spz2glb_core.cpp` / `spz_verifier.cpp` aligned with the upstream Niantic `NgspFileHeader` (32-byte) spec:
  - byte 15: `numStreams` (was `reserved`)
  - bytes 16-19: `tocByteOffset` (was `pointCount`)
  - bytes 20-31: `reserved[12]` (was `shBandCount` / `chunkConfig` / `attributeOffsets`)
- **Impact**: previously `tocByteOffset` read from the reserved region (always 0), so Layer 5 could not locate the header zone and ILV `0xADBE0003` detection silently failed on v4 ZSTD files. Layout now matches gatekeeper's verified parsing.

### CLI Queue & Web Mirror (PR #15)

- **File-system queue**: `--queue-add`, `--queue`, `--queue-status`, `--queue-clear` CLI commands using pending/processing/done/failed directories.
- **JSON report**: Per-file conversion report with full metadata (SPZ version/compression, GLB structure, KHR extensions, coordinate system, timing, generator info).
- **Web queue mirror**: Multi-file selection (`<input multiple>` + drag-drop), 4-slot queue status bar, per-file GLB download + JSON report export.
- **MSVC compatibility**: Remove unused variables, replace `localtime` with safe variants.

### Deploy-Pages Refactoring (PR #16, #18)

- **Independent pages workflow**: Extract deploy-pages from `release.yml` to dedicated `pages.yml` (build_pages + deploy_pages), mirroring gatekeeper project structure.
- **Tag trigger**: Pages deployment now runs on `v*` tag pushes, not just `workflow_dispatch` or push to main.

### WASM / Web Fixes (PR #19, #20)

- **SPZ version detection**: Fix `detectSpzVersion()` — use NGSP magic (`0x5053474E`) for v4 and gzip magic (`0x1F8B`) for v3, replacing incorrect ZSTD header (`0xFD2FB528`) detection.
- **WASM runtime version**: Fix `spz2glb_get_version()` patch number (was still returning 2.0.2), later bumped again to 2.0.4 for this release.
- **KHR report field names**: Use standard `KHR_gaussian_splatting` / `KHR_gaussian_splatting_compression_spz_2` in JSON reports (was `khrGaussianSplatting` / `spzCompression`). Parse from primitive-level extensions, not root-level.
- **Runtime performance panel**: Collapsible 11-dimension stats (WASM memory, device info, alloc/free/fail counts, hot pool, work area, recommended file size limit).
- **Optional `--report` validation**: `spz_verify` CLI validates `extensionsUsed/Required` and KHR sub-fields against conversion result via `--report <file.json>`.

### Version Bump & macOS CI Matrix

- **Version 2.0.3 → 2.0.4**: Update version strings across `CMakeLists.txt`, `src/queue.cpp` (report `generator.version`), `src/spz_to_glb.cpp` (CLI banner), and `docs/examples/spz2glb_bindings.js` (WASM demo).
- **Split macOS matrix**: `macos-latest` now defaults to ARM64 — the matrix was split into `macos-15-intel` (Intel x64 → `spz2glb-macos-x64`) and `macos-latest` (ARM64 → `spz2glb-macos-arm64`), each producing native CLI + `spz_verify` binaries.
- **Replace retired `macos-13`**: The `macos-13` runner label was retired by GitHub on 2025-12-04; Intel x64 builds now use the official replacement `macos-15-intel` (the last x86_64 macOS image, available until Aug 2027). macOS runner condition switched to `startsWith(matrix.os, 'macos')`.

### Release Assembly & Fixing Follow-up (PR #21, #22)

- **Release assembly (PR #21)**: Aggregates all v2.0.4 feature commits from `clean-pr` into `main` and creates the `v2.0.4` tag.
- **macOS build fix (PR #22)**: Removes unused `kExtGaussian` / `kExtSpz2` constants from `src/queue.cpp` that broke the macOS `-Werror` build (Clang `-Wunused-const-variable`). This had silently shipped the first v2.0.4 release without macOS `spz2glb` binaries — the `| tee` pipeline in the build steps masked the compiler failure.
- **CI unmask (PR #22)**: Adds `set -euo pipefail` + `shell: bash` to every tee-piped build step in `release.yml` / `pages.yml` so build failures can no longer be hidden. Windows PowerShell cannot parse `pipefail`, so the affected steps explicitly pin bash.

### Code Quality & CI Enhancement

- **P7_DEADCODE**: Uninitialized variable checks (`-Wuninitialized`, `-Wmaybe-uninitialized`) and cross-function scope detection in `wasm-pre-check.sh`.
- **Cross-review fixes**: Resolve scope analysis issues in `spz_verify.cpp` incl. unused `layerKey` variable and `outName` cross-function reference in `queue.cpp`.
- **CI triggers**: Add `clean-pr` branch to release and test-wasm-build workflows.

## Upgrade Path

### From v2.0.3
This is a drop-in replacement with no breaking changes.

### From v2.0.2 / earlier
See the v2.0.3 release notes for cumulative changes from v2.0.2 to v2.0.3, then apply this release on top.

## Installation

### Pre-built Binaries
Download from [GitHub Releases](https://github.com/spz-ecosystem/spz2glb/releases):
- Windows: `spz2glb-windows-x64.exe`
- Linux: `spz2glb-linux-x64`
- macOS: `spz2glb-macos-x64` (Intel) / `spz2glb-macos-arm64` (Apple Silicon)
- WASM: `spz2glb-compat.js/.wasm` + `spz2glb-perf-lite.js/.wasm`

### Building from Source
```bash
git clone https://github.com/spz-ecosystem/spz2glb.git
cd spz2glb/tools/spz_to_glb
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Contributors

- **Pu Junhan** - Project maintainer and lead developer

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

**Cultural Note**: This project is released in the year of the Red Fire Horse (Bingwu, 丙午), Huangdi Era 4723. It honors the ancient Chinese lunisolar calendar, a testament to humanity's enduring quest to harmonize with the cosmos.

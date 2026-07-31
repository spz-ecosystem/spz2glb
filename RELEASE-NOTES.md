# spz2glb v2.0.4 Release Notes

**Release Date**: July 30, 2026  
**Tag**: v2.0.4  
**Type**: CLI Queue, Web Mirror, CI/Workflow Refactoring, WASM Fixes & Code Quality

## Overview

This release adds a file-system based CLI queue with web mirror, extracts deploy-pages into an independent workflow, fixes SPZ version detection and KHR report field naming, and strengthens code quality with enhanced WASM pre-check dead-code detection.

## Changes Since v2.0.3

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
- **WASM runtime version**: Fix `spz2glb_get_version()` patch number from 2.0.2 to 2.0.3.
- **KHR report field names**: Use standard `KHR_gaussian_splatting` / `KHR_gaussian_splatting_compression_spz_2` in JSON reports (was `khrGaussianSplatting` / `spzCompression`). Parse from primitive-level extensions, not root-level.
- **Runtime performance panel**: Collapsible 11-dimension stats (WASM memory, device info, alloc/free/fail counts, hot pool, work area, recommended file size limit).
- **Optional `--report` validation**: `spz_verify` CLI validates `extensionsUsed/Required` and KHR sub-fields against conversion result via `--report <file.json>`.

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

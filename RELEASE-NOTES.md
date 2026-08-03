# spz2glb v2.0.5 Release Notes

**Release Date**: August 3, 2026  
**Tag**: v2.0.5  
**Type**: WASM Performance Optimization, Web UI Enhancements, CI Test Hardening

## Overview

This release focuses on the web/WASM experience: it eliminates a 26.6MB JavaScript big-object copy that was the root cause of Major-GC timing jitter (the converter now parses and Blobs directly from the WASM output view), makes queue semantics explicit (`MAX_PARALLEL=1` — the WASM converter is single-instance, so queued files wait without contaminating conversion timing), adds segmented timing (WASM conversion vs JS overhead), and rebuilds the demo UI with i18n (zh/en) + dark theme + gatekeeper-aligned purple styling. The CI suite is hardened so it actually executes: hash-matrix and browser-smoke tests are merged into real single steps (the previous pipeline silently exited early on npm audit ENOLOCK and never ran the tests), fixtures now generate correctly (stride fix), and every fixture is measured 5x with min/median/max/sigma plus full-pack `packMs` timing, emitting JSON reports downloadable straight from CI. Version 2.0.5 is synchronized end-to-end (CMake → JSON report → CLI banner → WASM demo → logic-test assertion) and tagged `v2.0.5`.

## Changes Since v2.0.4.1

### Web Performance Optimization

- **Eliminated the JS big-object copy** (`d0f47df` / `bc028f6`): removed the 26.6MB `glbCopy` (a V8 large object that triggered Major-GC spikes). `parseGlbJson` now reads the WASM output handle's byte view directly (valid until `release()`), and `toBlob()` creates the Blob from that view (the Blob owns an independent copy).
- **Segmented timing** (`22762a7`): the performance panel now splits WASM conversion time vs JS-side overhead, pinpointing where time actually goes.
- **Explicit serial semantics** (`d045d09` / `2843856` / `29c1976`): the WASM converter is single-instance (global reserved input buffer + mutex). Frontend `MAX_PARALLEL=1` — additional files stay `queued` and their conversion timing excludes waiting time (fixes a 700ms over-report). The CLI `Queue::run` `maxParallel` parameter is now documented as API-compatible but serial-only, matching the frontend.
- **Version synced with Git tags** (`30a0802` / `34ff214`): CMake derives the version from `git describe --tags` and injects it into the WASM runtime; CI now uses `fetch-depth: 0` so shallow clones also see tags (fixes the 2.0.3 fallback).

### Web UI Enhancements

- **Performance panel upgrades** (`8c4b047` / `39195a5` / `0d9d4eb`): refreshes immediately after each conversion to keep the latest stats; shows build time (injected by CI), load timestamp, last-conversion record, and SPZ version (v3 gzip / v4 zstd).
- **SPZ header pre-check** (`0d9d4eb`): before converting, the JS-side `detectSpzVersion` reads the magic bytes (v3 `1F8B` / v4 `NGSP`) and rejects invalid files early — deliberately not the WASM `validateSpzHeader`, which needs a full gunzip to read the header and would falsely reject v3 files from a 64-byte probe.
- **i18n (zh/en) + dark theme** (`a02997f`): top toolbar language toggle (I18N dictionary + `data-i18n` + `t()`, persisted in localStorage) and day/night theme (`[data-theme="dark"]` CSS variable overrides). Color scheme unified with the gatekeeper project: brand purple `#6666ff` + light blue-white gradient, white 20px-rounded cards, purple shadow; status badges/bar switched to translucent semantic colors that work in both themes.
- **Cold-start note** (`a02997f` / `75dbea8`): the panel notes that the first conversion includes a cold start (v3 gzip first-run zlib decompression init) and later ones are faster — wording made unambiguous.
- **Queue interactions** (`e806933` / `e8200b2` / `a572b5f`): completed conversions auto-download GLB + JSON report and auto-clean the queue after a delay; completed items slide out (matching gatekeeper `queue-exit`); fixed the `queued` count mapping (pending was stuck at 0) and empty-queue hiding.
- **Cache de-bugging** (`0581fae` / `c01435e`): WASM loading gets cache-busting plus a load-timestamp display so you can confirm you are not on a stale cached binary.

### CI Test Hardening

- **JS logic tests + JSON report artifacts** (`992b4e0`): CI now emits `logic-tests.json` / `wasm_hash_report.json` — download the artifacts and read the results, no need to open the web page.
- **False-green fix** (`38a0c06`): hash matrix and browser smoke tests merged into single real steps with `continue-on-error` removed — previously `npm audit` ENOLOCK made `bash -e` exit early and the playwright/node tests never actually ran.
- **Hash matrix real-execution fixes** (`c655f37` / `cc5c7c0` / `a7800ef`): `convert()` was not awaited (the handle is a Promise), the input must be a `Uint8Array` (an `ArrayBuffer` has no `.buffer`/`.byteOffset` and caused empty writes), fastgltf JSON was missing a comma, and `gen_fixture` had a stride out-of-bounds (`POINT_STRIDE=17` vs the actual 20-byte layout) — synthetic fixtures now actually generate.
- **Variance measurement** (`1c02a7b` / `c6a126f` / `d2a3956`): each fixture is converted 5 times and reported as min/median/max/sigma; a full-pack `packMs` timing covers parse + report + Blob end-to-end; statistics are declared before `generateReportJson` references them (TDZ fix).
- **Workflow triggers** (`b6ba22f` / `cb5cac7` / `6760fae`): `feature/spz2glb-perf-panel-fix` registered on push/pages triggers; `run-name` fixed so non-ASCII commit titles do not render as question marks.

### Version

- **Version 2.0.4.1 → 2.0.5**: version strings updated across `CMakeLists.txt` (project VERSION), `src/queue.cpp` (JSON report `generator.version`), `src/spz_to_glb.cpp` (CLI banner), `docs/examples/spz2glb_bindings.js` (WASM demo), and `tests/logic.test.mjs` (logic-test assertion).

## Upgrade Path

### From v2.0.4 / v2.0.4.1
Drop-in replacement. No breaking changes — no SPZ/GLB format changes, no CLI interface changes. The only behavior change is that the web demo queue is explicitly serial (`MAX_PARALLEL=1`), which matches what was already physically true (single-instance WASM converter).

### From v2.0.3 / earlier
See the v2.0.4 release notes for cumulative changes from v2.0.3 to v2.0.4, then apply this release on top.

---

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

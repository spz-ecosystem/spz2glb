# Changelog

## v2.0.4.1 (2026-07-31)

Changes since v2.0.4 tag (`3a134b4`). Merged PRs: [#23](https://github.com/spz-ecosystem/spz2glb/pull/23), [#24](https://github.com/spz-ecosystem/spz2glb/pull/24).

### Pre-Tag Changelog-Coverage Gate (PR #23)

- **`changelog-check` job in `release.yml`**: On tag push, verify every commit since the previous tag is covered by `CHANGELOG.md`. Squash-merged PRs are matched by their `(#N)` PR number; direct commits fall back to keyword matching; `docs:` commits are self-documenting and exempt.
- **Rationale**: The tag-as-freeze model means PRs merged after a tag never reach that version's changelog — the gate makes incomplete coverage fail the release instead of silently shipping.

### Donation Statement (PR #24)

- **Future Plans / 未来规划 section**: README.md and README-zh.md now declare intent to donate to the **OpenAtom Foundation** at an appropriate stage, to foster broader community collaboration and governance of the SPZ ecosystem (consistent with `spz_gatekeeper`).

## v2.0.4 (2026-07-31)

Changes since v2.0.3 tag (`1e131a6`). Merged PRs: [#15](https://github.com/spz-ecosystem/spz2glb/pull/15), [#16](https://github.com/spz-ecosystem/spz2glb/pull/16), [#18](https://github.com/spz-ecosystem/spz2glb/pull/18), [#19](https://github.com/spz-ecosystem/spz2glb/pull/19), [#20](https://github.com/spz-ecosystem/spz2glb/pull/20) (feature work), [#21](https://github.com/spz-ecosystem/spz2glb/pull/21) (release assembly), [#22](https://github.com/spz-ecosystem/spz2glb/pull/22) (release-fixing follow-up), plus follow-up fixes committed directly on `clean-pr` (SPZ v4 header alignment, version bump, macOS CI matrix).

### SPZ v4 Header Alignment (fix)

- **SpzV4Header byte offset fix**: Align `SpzV4Header` field layout in `spz2glb_core.cpp` / `spz_verifier.cpp` with the upstream Niantic `NgspFileHeader` (32-byte) spec:
  - byte 15: `numStreams` (was `reserved`)
  - bytes 16-19: `tocByteOffset` (was `pointCount`)
  - bytes 20-31: `reserved[12]` (was `shBandCount` / `chunkConfig` / `attributeOffsets`)
- **Bug fixed**: The misaligned layout made `tocByteOffset` read from the reserved region (always 0), so Layer 5 could never locate the header zone and ILV extension detection (`0xADBE0003` coordinate system) silently failed on v4 ZSTD SPZ files. The layout now matches gatekeeper's verified header parsing.
- **Debug output**: `peekSpzHeaderFromZstd` now prints `numStreams` + `tocByteOffset` instead of the previously misaligned fields.

### CLI Queue & Web Mirror (PR #15)

- **File-system queue**: Add `--queue-add`, `--queue`, `--queue-status`, `--queue-clear` CLI commands. Queue directories: pending/processing/done/failed.
- **JSON report**: Per-file conversion report with full metadata (SPZ version/compression, GLB structure, KHR extensions, coordinate system, timing, generator info).
- **Web queue mirror**: Multi-file selection via `<input multiple>` and drag-drop. 4-slot queue status bar. Per-file GLB download + JSON report export.
- **MSVC fixes**: Remove unused variables, replace `localtime` with safe variants for MSVC compatibility.

### Deploy-Pages Refactoring (PR #16, #18)

- **Independent pages workflow**: Extract deploy-pages from `release.yml` to dedicated `pages.yml` (`build_pages` + `deploy_pages`), mirroring gatekeeper structure.
- **Tag trigger**: Add `startsWith(github.ref, 'refs/tags/')` condition so Pages deployment runs on tag push (not just `workflow_dispatch` or push to main).

### WASM / Web Fixes (PR #19, #20)

- **SPZ version detection**: Fix `detectSpzVersion()` — use NGSP magic (`0x5053474E`) for v4 and gzip magic (`0x1F8B`) for v3, replacing incorrect ZSTD header (`0xFD2FB528`) detection.
- **WASM runtime version**: Fix `spz2glb_get_version()` patch number (was still returning 2.0.2).
- **KHR report field names**: Use standard `KHR_gaussian_splatting` / `KHR_gaussian_splatting_compression_spz_2` field names in JSON report (was abbreviated `khrGaussianSplatting` / `spzCompression`). Fix `parseGlbJson` to read from primitive-level extensions (not root-level).
- **Runtime performance stats**: Add collapsible 11-dimension performance panel (WASM memory stats, device info, alloc/free/fail counts, hot pool, work area usage, recommended file size limit).
- **Optional `--report` validation**: Add `--report <file.json>` to `spz_verify` CLI. Validates `extensionsUsed/Required`, KHR sub-fields against actual conversion result.

### Version Bump

- **Version 2.0.3 → 2.0.4**: Update version strings across `CMakeLists.txt` (project VERSION), `src/queue.cpp` (JSON report `generator.version`), `src/spz_to_glb.cpp` (CLI banner), and `docs/examples/spz2glb_bindings.js` (WASM demo).

### macOS CI Matrix (Intel x64 + Apple Silicon ARM64)

- **Split macOS matrix**: `macos-latest` now defaults to ARM64, so the release matrix was split into two entries producing native CLI + `spz_verify` binaries for both architectures:
  - `macos-15-intel` (Intel x64) → `spz2glb-macos-x64`
  - `macos-latest` (ARM64) → `spz2glb-macos-arm64`
- **Replace retired `macos-13`**: GitHub retired the `macos-13` runner label on 2025-12-04. Intel x64 builds now use the official replacement `macos-15-intel` (the last x86_64 macOS image, available until Aug 2027).
- **Runner condition**: macOS conditional switched from `matrix.os == 'macos-latest'` to `startsWith(matrix.os, 'macos')` to cover both runners.

### Release Assembly & Fixing Follow-up (PR #21, #22)

- **Release assembly (PR #21)**: Aggregate all v2.0.4 feature commits from `clean-pr` into `main` and create the `v2.0.4` tag.
- **macOS build fix (PR #22)**: Remove unused `kExtGaussian` / `kExtSpz2` constants from `src/queue.cpp` that broke the macOS `-Werror` build (Clang `-Wunused-const-variable`). This had silently shipped the first v2.0.4 release without macOS `spz2glb` binaries — the `| tee` pipeline in the build steps masked the compiler failure (pipeline returns tee's exit code 0).
- **CI unmask (PR #22)**: Add `set -euo pipefail` + `shell: bash` to every tee-piped build step in `release.yml` / `pages.yml` so build failures can no longer be hidden. Windows PowerShell cannot parse `pipefail` (`set` is a Set-Variable alias), so the affected steps explicitly pin bash (Git Bash ships on GitHub-hosted Windows runners).

### Code Quality & CI Enhancement

- **P7_DEADCODE stage**: Add uninitialized variable checks (`-Wuninitialized`, `-Wmaybe-uninitialized`) and cross-function scope reference detection to `wasm-pre-check.sh`.
- **Cross-review alignment**: Resolve scope analysis issues in `spz_verify.cpp` flagged by cross-review.
- **unused layerKey**: Remove unused `layerKey` variable from `spz_verify.cpp` fixing WASM `-Werror` build.
- **outName scope fix**: Replace `outName` reference in `queue.cpp::finalize()` with `result.outputFile` to fix cross-function scope violation.
- **CI trigger branches**: Add `clean-pr` branch to both `release.yml` and `test-wasm-build.yml` trigger lists.
- **Report filename**: Mark `.glb` suffix in report JSON filenames for clarity.

---

## v2.0.3 (2026-07-30)

### CI/CD Security Hardening

- **Template injection fix**: Replace `actions/download-artifact` with `gh run download` via env vars to eliminate `${{ github.event.inputs.* }}` interpolation in action `with:` parameters
- **Cache poisoning fix**: Remove `actions/cache` from release workflow entirely; replace `${{ github.ref }}` with `${{ github.sha }}` in test workflow cache keys
- **zizmor upgrade**: `1.5.0` → `1.26.0` for improved audit coverage
- **Accessibility audit**: Add `actionlint` to CI security-audit job alongside zizmor
- **actions/github-script**: Upgrade from v7.0.1 to v9.0.0 to resolve `known-vulnerable-actions` finding
- **Workflow pinning**: Pin all third-party actions to full commit SHAs with version comments
- **Artifact retention**: Set `retention-days: 7` on `upload-artifact` steps

### Test Infrastructure

- **Synthetic SPZ fixtures**: Add `tests/gen_fixture.mjs` — deterministic Node.js script generating 4 types of minimal SPZ files (v3 0-point, 1-point, 10-point, and v4 header-only) for CI testing
- **SPZ benchmark dataset**: Include `tests/data/bench/classroom_anime_v3.spz` and `classroom_anime_v4.spz` from the [spz-anime-text2scene-bench](https://github.com/spz-ecosystem/spz-anime-text2scene-bench) dataset as in-repo reference samples
- **Dynamic hash matrix**: Replace hardcoded 4-sample hashes with dynamic fixture file enumeration via `fixtures.json` / `hash_fixtures.json`, eliminating external file dependencies
- **Self-hosted runner removal**: Migrate `private-fixed-samples-validation` from `[self-hosted, linux, x64]` to `ubuntu-latest` with synthetic + benchmark data
- **CI paths trigger**: Add `.github/workflows/release.yml` to Test WASM Build paths filter to trigger security audit on release workflow changes

### WASM Build

- **Emscripten 6.0.3**: Upgrade Emscripten SDK from `5.0.1` to `6.0.3` across all workflows and documentation
- **WASM pre-check**: Add `scripts/wasm-pre-check.sh` — pre-push safety net checking environment, build, symbol exports, artifacts, WASM analysis, and workflow lint
- **Memory constraint**: MAXIMUM_MEMORY set to 512MB (compat profile), consistent with project design limits
- **`-Oz` optimization**: Add WASM-specific size optimization for smaller binary output

### Verification (spz_verify)

- **Five-layer verification**: Extend from 3 layers to 5:
  - L4: GLB metadata vs SPZ header consistency (coordinate system, version alignment)
  - L5: ILV extension integrity (TLV record validation)
- Update all documentation, CLI help, and CI references from "3-layer" to "5-layer"

### CMake & Build System

- **Windows CI compatibility**: Add FetchContent fallback for zlib and zstd to replace `pkg_check_modules` (unavailable on Windows)
- **INTERFACE library**: Add `spz2glb_zlib` INTERFACE library target to fix `set(ZLIB::ZLIB ...)` CMake syntax error
- **CMake syntax validation**: Add regex check in pre-commit to detect `set(XXX::YYY)` patterns (reserved for CMake ALIAS/IMPORTED targets)

### Documentation

- **Five-layer verification**: Update READMEs (EN/ZH) from "3-layer" to "5-layer" across all sections
- **Emscripten version**: Update WASM build instructions from 5.0.1 to 6.0.3
- **Memory config**: Update MAXIMUM_MEMORY from 1GB to 512MB in documentation tables
- **spz_verify CLI**: Add L4 and L5 command documentation with output examples
- **spz2glb CLI**: Document `--verify` flag for one-step convert + verify
- **Dependencies**: Add ZSTD to dependency tables
- **Project structure**: Add `tests/gen_fixture.mjs`, `tests/data/bench/`, `scripts/` to directory tree
- **Test Data section**: New section covering synthetic fixtures and benchmark dataset
- **Ecosystem**: Add spz-anime-text2scene-bench to related projects
- **SPZ v4 support**: Document v4 header-only synthetic fixture and ZSTD-based SPZ v4 format support
- **Zenodo DOI**: Add DOI: 10.5281/zenodo.20849112

### Fixes

- **zizmor `--suppress`**: Remove invalid `--suppress` CLI argument (not supported by zizmor; suppression is via inline comments or config file)
- **YAML indentation**: Fix `run: |` block indentation in fixture list preparation step
- **commit_msg.txt**: Remove accidentally tracked file from repository

### KHR Extension

- **Compile flag removal**: Remove `FASTGLTF_ENABLE_KHR_GAUSSIAN_SPLATTING` from fastgltf fork per upstream feedback (spnda/fastgltf#137). The extension is Release Candidate (pending Khronos Board vote), no compile-time gate needed. Impacts:
  - `third_party/CMakeLists.txt`: Remove `option()` for the flag
  - `types.hpp`: Remove `#ifndef` guard around `GaussianSplatExtension`/`GaussianSplatSpzCompression` — structs always visible
  - `fastgltf.cpp`: Remove `#if FASTGLTF_ENABLE_KHR_GAUSSIAN_SPLATTING` guards — serialization always compiled
  - `CMakeLists.txt`: Remove all `ENABLE_KHR_GAUSSIAN_SPLATTING` → `target_compile_definitions(...)` blocks (5 targets)
  - `tests/CMakeLists.txt`: Remove stale compile definition
- **KHR spec compliance**: Add missing `kernel`/`colorSpace`/`sortingMethod`/`projection` fields to `GaussianSplatExtension` struct in feature branch
- **KHR serialization completeness**: Write `spzVersion`/`compression`/`coordinateSystem` fields to GLB JSON for the nested `KHR_gaussian_splatting_compression_spz_2` extension
- **Layer 1 enhancement**: Add field-level checks for `kernel` and `colorSpace` presence in the parent KHR_gaussian_splatting extension JSON
- **CI debug logging**: Add `Dump build log on failure` steps to both `test-wasm-build.yml` and `release.yml` workflows, capturing build errors directly in CI console output

### Documentation

- **Extension status**: Update READMEs (EN/ZH) — KHR_gaussian_splatting marked as **Release Candidate**, compile flag removed, spz_2 still draft
- **Layer 1 output**: Update CLI help and verification output examples to reflect 12-check L1 validation with kernel/colorSpace field checks
- **Compilation Control**: Replace old `ENABLE_KHR_GAUSSIAN_SPLATTING` option docs with "always enabled" statement
- **Customization Notes**: Expand fastgltf customization section with struct fields, JSON serialization, compile flag removal details, simdjson source embedding approach

### CLI Performance Optimizations

- **simdjson upgrade**: Update embedded simdjson from v4.3.1 to v4.6.4 (2026-05-06 release), syncing with fastgltf upstream compatibility
- **Compiler optimization flags**: Add explicit `-O3` (GCC/Clang) / `/O2` (MSVC) to all CLI targets, ensuring optimized builds even without `-DCMAKE_BUILD_TYPE=Release`
- **Zero-copy verify**: Add pointer-based `Verifier::verify(const uint8_t*, size_t, const uint8_t*, size_t)` overload, eliminating the `std::vector<uint8_t>` memcpy in `--verify` mode. Update CLI output from 3-layer to 5-layer display.
- **Native batch mode**: Add `--batch EXT` option for single-process batch conversion of all files matching a given extension (e.g. `.spz`), avoiding shell loop + process startup overhead
- **Memory-mapped file I/O**: Add `src/mapped_file.h` — cross-platform RAII `MappedFile` class using `CreateFileMapping` (Windows) / `mmap` (POSIX), reducing kernel→userspace copy for large file reads
- **Code cleanup**: Remove unused `loadSpzFile()`, `SpzResult`, `SpzErrorCode` from `spz_to_glb.cpp` (replaced by `MappedFile`)

### CI/Workflow

- **verify-native decoupled**: Remove `needs: [build]` dependency from verify-native job — now runs independently (parallel) instead of waiting for the 3-platform build matrix, preventing native verification from being skipped when a single platform fails
- **MinGW cross-compile check**: Add `x86_64-w64-mingw32-g++ -fsyntax-only` step to verify-native, catching `#ifdef _WIN32` guard mismatches without requiring Windows ZLIB/ZSTD libraries
- **P6 integrity in verify-native**: Replicate UTF-8 encoding and CMake syntax checks from wasm-pre-check.sh into verify-native, forming dual coverage (WASM + native)
- **Rollback security gate**: Add pre-download verification to rollback-artifact-entry — rejects rollback sources not on `main` branch or not completed successfully, with SHA-256 checksum and verified commit SHA in audit manifest
- **Playwright upgrade**: 1.53.0 → 1.62.0
- **Multi-browser WASM testing**: Extend browser smoke tests (hash matrix + glue loading) from Chromium-only to Chromium + Firefox + WebKit, running on all three engines in parallel

---

## v2.0.2 (2026-05-11)

- CI reproducibility: pin emsdk version, remove WASM zlib port drift
- WASM build: restore zlib port compile flags
- Version consistency cleanup across build system
- License: add cultural note
- Documentation: GLB structure examples, CHANGELOG
- Add `docs/plans/` to .gitignore (local planning docs only)
- Release artifacts verified for all platforms (Windows/Linux/macOS x64)

## v2.0.1

- Minor documentation fixes
- Release artifact packaging improvements

## v2.0.0

- **Major refactor (v2.0)**: Unified CLI/WASM core path, reducing dual-end divergence
- **WASM enhancements**: Reserved input buffer, explicit output release, memory stats, dual profile (compat/perf-lite)
- **Three-layer verification**: GLB structure / lossless binary / decoding consistency
- **KHR_gaussian_splatting_compression_spz_2**: Standard extension export
- **Smart memory allocation**: Device tiering + file budgeting + WASM-side reservation
- **Cross-platform**: Windows, Linux, macOS (x64 + ARM) builds
- **Zero runtime dependencies**: C++17 + WASM
- **Custom fastgltf**: simdjson v4.3.1 built-in, no network downloads

## v1.1.0

- Initial WASM build support
- Basic SPZ→GLB conversion pipeline
- CLI tool with file I/O

## v1.0.2 – v1.0.0

- Initial releases with core conversion functionality
- GLB output with KHR_gaussian_splatting extension
- Cross-platform CMake build system

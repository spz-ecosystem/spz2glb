# Changelog

## v2.0.3 (2026-07-29)

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

### KHR Extension (v2.0.3 continuation — 2026-07-30)

- **Compile flag removal**: Remove `FASTGLTF_ENABLE_KHR_GAUSSIAN_SPLATTING` from fastgltf fork per upstream feedback (spnda/fastgltf#137). The extension is ratified, no compile-time gate needed. Impacts:
  - `third_party/CMakeLists.txt`: Remove `option()` for the flag
  - `types.hpp`: Remove `#ifndef` guard around `GaussianSplatExtension`/`GaussianSplatSpzCompression` — structs always visible
  - `fastgltf.cpp`: Remove `#if FASTGLTF_ENABLE_KHR_GAUSSIAN_SPLATTING` guards — serialization always compiled
  - `CMakeLists.txt`: Remove all `ENABLE_KHR_GAUSSIAN_SPLATTING` → `target_compile_definitions(...)` blocks (5 targets)
  - `tests/CMakeLists.txt`: Remove stale compile definition
- **KHR spec compliance**: Add missing `kernel`/`colorSpace`/`sortingMethod`/`projection` fields to `GaussianSplatExtension` struct in feature branch
- **KHR serialization completeness**: Write `spzVersion`/`compression`/`coordinateSystem` fields to GLB JSON for the nested `KHR_gaussian_splatting_compression_spz_2` extension
- **Layer 1 enhancement**: Add field-level checks for `kernel` and `colorSpace` presence in the parent KHR_gaussian_splatting extension JSON
- **CI debug logging**: Add `Dump build log on failure` steps to both `test-wasm-build.yml` and `release.yml` workflows, capturing build errors directly in CI console output

### Documentation (v2.0.3 continuation)

- **Extension status**: Update READMEs (EN/ZH) — KHR_gaussian_splatting marked as **ratified**, compile flag removed, spz_2 still draft
- **Layer 1 output**: Update CLI help and verification output examples to reflect 12-check L1 validation with kernel/colorSpace field checks
- **Compilation Control**: Replace old `ENABLE_KHR_GAUSSIAN_SPLATTING` option docs with "always enabled" statement

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

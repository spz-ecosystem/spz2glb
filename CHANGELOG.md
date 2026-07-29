# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.3] - 2026-07-29

### Added
- **SPZ v4/ZSTD 支持**: ZSTD 检测 (`isZstdData`/`peekSpzHeaderFromZstd`)、v4 32B header 明文解析 (SpzV4Header)
- **ILV 003 coordinateSystem 解析**: header zone ILV 扫描, coordinateSystem 提取与值域校验
- **五层验证器扩展**: L1-L3 保留, 新增 L4 (GLB 元数据 vs SPZ header 一致性)、L5 (ILV 扩展完整性)
- **fastgltf 扩展字段**: `GaussianSplatSpzCompression` 新增 `spzVersion`/`compression`/`coordinateSystem` + JSON 序列化
- **CMake zstd 集成**: 原生构建通过 PkgConfig 链接 libzstd, WASM 通过 `--use-port=zstd`

### Security
- **CI 加固**: zizmor 安全审计、npm audit 供应链检查、SHA-pin 所有 actions、`persist-credentials: false`
- **权限收紧**: workflow 级 `contents: read`, job 级最小权限 (pages/id-token 仅在 deploy-pages)
- **Pages 部署 SHA 升级**: upload-pages-artifact v3→v5.0.0, deploy-pages v4→v5.0.0

## [2.0.2] - 2026-04-21

### Fixed
- **WASM CI reproducibility**: Pin Emscripten to version 5.0.1 and add EM_CACHE to prevent build drift.
- **Remove dead configuration**: Clean up unused `SPZ2GLB_USE_EMSCRIPTEN_ZLIB` CMake option.
- **Restore zlib port flags**: Ensure zlib.h is available during WASM compilation by restoring `--use-port=zlib` flags.

### Changed
- **Documentation alignment**: Update README and troubleshooting docs to reflect fixed Emscripten version.
- **License enhancement**: Add cultural note to LICENSE file for better clarity.
- **Copyright headers**: Improve copyright headers across source files.

### Added
- **CHANGELOG.md**: Add comprehensive changelog for version tracking.
- **Project evolution log**: Add detailed project evolution documentation for standard draft and paper reference.

## [2.0.1] - 2026-04-15

### Added
- **Cultural note**: Add cultural note to LICENSE file.

## [2.0.0] - 2026-04-10

### Added
- **Large-scale refactor**: Unified CLI/WASM core path, reduced dual-end divergence.
- **WASM enhancements**: Reserved input, explicit output release, memory stats, compat/perf-lite dual profile.
- **Three-layer verification**: Structure validation / lossless validation / decoding consistency.

### Changed
- **Dual-end collaboration**: Scenario split — browser side for lightweight preview/quick checks, local CLI for heavy conversion.

## [1.0.0] - 2026-03-01

### Added
- **Initial release**: SPZ to GLB lossless packaging with KHR_gaussian_splatting_compression_spz_2 extension.
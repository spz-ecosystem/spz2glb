# spz2glb v2.0.2 Release Notes

**Release Date**: April 20, 2026  
**Tag**: v2.0.2  
**Type**: Stabilization & Documentation Cleanup

## Overview

This is a stabilization release focused on version consistency, documentation accuracy, and cultural heritage acknowledgment. It addresses version mismatches between build configuration and runtime reporting, corrects outdated verification descriptions, and adds cultural context to source headers.

## Key Changes

### Version Consistency
- **CMakeLists.txt**: Updated project version from 1.0.0 to 2.0.2
- **Runtime API**: Updated `spz2glb_get_version()` to return 2.0.2
- **Documentation**: Updated CHANGELOG.md to reflect current version

### Documentation Corrections
- **Layer 2 Verification**: Corrected descriptions in README.md and README-zh.md
  - Changed from "100% MD5 match" to "byte-identical"
  - Changed from "extracts and decompresses data to compute MD5 hashes" to "performs byte-by-byte comparison"
  - This reflects the actual implementation which performs byte-level comparison, not MD5 hashing

### Cultural Heritage
- **Source Headers**: Added cultural note to all source file headers
  - "Cultural Note: Huangdi Era 4723, Year of the Red Fire Horse (Bingwu, 丙午)"
  - This acknowledges the project's release in the traditional Chinese lunisolar calendar

### Known Limitations

#### Three-Layer Verification Path Constraint
The current implementation has a known limitation in the three-layer verification system:
- **L1 (Structure)**: Correctly checks for outer `KHR_gaussian_splatting` extension declaration
- **L2/L3 (Lossless/Decoding)**: Does not enforce the full nested path constraint:
  ```
  primitive.extensions.KHR_gaussian_splatting.extensions.KHR_gaussian_splatting_compression_spz_2
  ```
- **Impact**: The verification may pass files that don't properly nest the SPZ_2 extension within the outer Gaussian splatting extension
- **Planned Fix**: This will be addressed in v2.0.3

#### Wiki Documentation
Multiple wiki files contain outdated or inconsistent information:
- **GLB-Structure.md** and **Extensions.md**: Examples miss the outer `KHR_gaussian_splatting` extension
- **Verification.md** and **Usage.md**: Still describe Layer 2 as MD5 verification
- **SPZ-Format.md**: Header field descriptions don't match actual `SpzHeader` structure
- **Planned Fix**: Wiki will be rewritten using graphify tool in a future release

## Upgrade Path

### From v2.0.1
This is a drop-in replacement with no breaking changes. The version number update is purely internal and doesn't affect functionality.

### From v1.x
Please refer to the v2.0.0 release notes for the major architectural changes.

## Installation

### Pre-built Binaries
Download from [GitHub Releases](https://github.com/spz-ecosystem/spz2glb/releases):
- Windows: `spz2glb-windows-x64.exe`
- Linux: `spz2glb-linux-x64`
- macOS: `spz2glb-macos-arm64` (Apple Silicon) / `spz2glb-macos-x64` (Intel)

### Building from Source
```bash
git clone https://github.com/spz-ecosystem/spz2glb.git
cd spz2glb/tools/spz_to_glb
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## What's Next

### v2.0.3 (Planned)
**Focus**: Three-Layer Verification Enhancement with Backward Compatibility

#### 1. Three-Layer Verification Enhancement
- **Goal**: Add nested path constraint detection without breaking existing validation
- **Strategy**: "Detect but don't enforce" - report nested structure status without failing validation
- **Implementation**:
  - Add `has_nested_extension` field to `VerifyResult` struct
  - Modify `parseBufferAndView` to detect nested structure: `primitive.extensions.KHR_gaussian_splatting.extensions.KHR_gaussian_splatting_compression_spz_2`
  - Output `[INFO]` or `[WARN]` messages about nested structure status
  - **No change to pass/fail criteria** - existing files continue to pass validation
- **Backward Compatibility**: 100% - no breaking changes to validation logic
- **Testing**: Add test cases for nested structure detection without changing existing tests

#### 2. Documentation Alignment
- **Update all documentation** to match current implementation:
  - Fix wiki files (GLB-Structure.md, Extensions.md) to show correct nested extension structure
  - Update Verification.md to reflect byte-level comparison (not MD5)
  - Align README files with actual validation behavior
- **Add validation mode documentation**:
  - Document current "lenient mode" (draft extensions safely ignored)
  - Prepare for future "strict mode" (when extensions are ratified)

#### 3. Wiki Rewrite with graphify
- **Use graphify tool** to generate knowledge graph of project
- **Generate wiki** using `graphify . --wiki`
- **Integrate generated wiki** into project documentation
- **Ensure consistency** between wiki and codebase

#### 4. Validation Mode Interface (Preparation)
- **Add CLI interface** for future validation modes:
  ```bash
  spz_verify all --strict   # Future: strict validation (when extensions ratified)
  spz_verify all            # Current: lenient validation (default)
  ```
- **No implementation yet** - just prepare the interface for v2.5+

**Key Principle**: v2.0.3 enhances detection capabilities while maintaining 100% backward compatibility. No existing validation behavior changes.

### v2.1.0 (Planned)
- **QR Code URL**: Add QR code generation for demo URLs
- **Enhanced Demo**: Improve web demo functionality

## Contributors

- **Pu Junhan** - Project maintainer and lead developer

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

**Cultural Note**: This project is released in the year of the Red Fire Horse (Bingwu, 丙午), Huangdi Era 4723. It honors the ancient Chinese lunisolar calendar, a testament to humanity's enduring quest to harmonize with the cosmos.
#!/usr/bin/env bash
# WASM pre-check for spz2glb.
# Adapted from spz_gatekeeper wasm-pre-check.sh (Apache 2.0).
# Run locally before pushing WASM-related changes to CI.
# Exit codes:
#   0  pass
#   1  usage/internal error
#   2  P0 environment
#   3  P1 build
#   4  P2 symbol exports
#   5  P3 artifact
#   6  P4 WASM analysis
#   7  P5 workflow lint
#   8  P6 file integrity

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build_wasm"
DIST_DIR="${PROJECT_DIR}/dist"

EMSDK_VERSION="6.0.3"
WASM_MIN_BYTES=$((200 * 1024))        # 200KB (minimal C API WASM)
WASM_MAX_BYTES=$((15 * 1024 * 1024))   # 15MB

# spz2glb-wasm C API exports (from CMakeLists.txt EXPORTED_FUNCTIONS)
REQUIRED_SPZ2GLB_SYMBOLS=(
  "_spz2glb_alloc"
  "_spz2glb_free"
  "_spz2glb_reserve_input"
  "_spz2glb_get_input_ptr"
  "_spz2glb_convert_reserved_input"
  "_spz2glb_release_output"
  "_spz2glb_convert"
  "_spz2glb_validate_header"
  "_spz2glb_validate_glb_header"
  "_spz2glb_validate_spz_header"
  "_spz2glb_get_version"
  "_spz2glb_get_memory_stats"
  "_spz2glb_reset_memory_stats"
  "_spz2glb_sizeof_size_t"
  "_spz2glb_sizeof_memory_stats"
)

# spz_verify-wasm exports — none currently (no EXPORTED_FUNCTIONS set)
# Verification is runtime-only: confirm WASM binary exists and has expected size.

STRICT=false
SKIP_BUILD=false
SKIP_SMOKE=false
AUTO_FIX=false

while [ $# -gt 0 ]; do
  case "$1" in
    --strict) STRICT=true; shift ;;
    --skip-build) SKIP_BUILD=true; shift ;;
    --skip-smoke) SKIP_SMOKE=true; shift ;;
    --build-dir) BUILD_DIR="$2"; shift 2 ;;
    --auto-fix) AUTO_FIX=true; shift ;;
    *) echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

SITE_DIR="${BUILD_DIR}/site"
SPZ2GLB_JS="${DIST_DIR}/spz2glb.js"
SPZ2GLB_WASM="${DIST_DIR}/spz2glb.wasm"
SPZ_VERIFY_JS="${DIST_DIR}/spz_verify.js"
SPZ_VERIFY_WASM="${DIST_DIR}/spz_verify.wasm"

fail() {
  local stage="$1" code="$2" msg="$3" hint="${4:-}"
  python3 -c "import json,sys; print(json.dumps({'ok':False,'stage':sys.argv[1],'exit_code':int(sys.argv[2]),'message':sys.argv[3],'hint':sys.argv[4],'logs':[]}, ensure_ascii=False))" "$stage" "$code" "$msg" "$hint"
  exit "$code"
}

pass() {
  python3 -c "import json; print(json.dumps({'ok':True,'stage':'P5_WORKFLOW','exit_code':0,'message':'WASM pre-check passed','logs':[]}, ensure_ascii=False))"
  exit 0
}

# ---------------------------------------------------------------------------
# P0: Environment
# ---------------------------------------------------------------------------
check_environment() {
  local missing=()
  command -v emcc >/dev/null 2>&1 || missing+=("emcc")
  command -v wasm-objdump >/dev/null 2>&1 || missing+=("wasm-objdump")
  command -v node >/dev/null 2>&1 || missing+=("node")
  command -v python3 >/dev/null 2>&1 || missing+=("python3")

  if [ ${#missing[@]} -gt 0 ]; then
    if [ "${AUTO_FIX}" = "true" ]; then
      local fixed=false
      for tool in "${missing[@]}"; do
        case "$tool" in
          wasm-objdump)
            echo "Auto-fix: installing wabt..." >&2
            if apt-get update -qq >/dev/null 2>&1 && apt-get install -y -qq wabt >/dev/null 2>&1; then
              fixed=true
            fi
            ;;
          emcc)
            echo "Auto-fix: emcc requires emsdk — run: git clone https://github.com/emscripten-core/emsdk && ./emsdk install ${EMSDK_VERSION} && ./emsdk activate ${EMSDK_VERSION}" >&2
            ;;
          node)
            echo "Auto-fix: installing nodejs..." >&2
            if apt-get update -qq >/dev/null 2>&1 && apt-get install -y -qq nodejs >/dev/null 2>&1; then
              fixed=true
            fi
            ;;
        esac
      done
      if [ "${fixed}" = "true" ]; then
        missing=()
        command -v emcc >/dev/null 2>&1 || missing+=("emcc")
        command -v wasm-objdump >/dev/null 2>&1 || missing+=("wasm-objdump")
        command -v node >/dev/null 2>&1 || missing+=("node")
        command -v python3 >/dev/null 2>&1 || missing+=("python3")
      fi
    fi

    if [ ${#missing[@]} -gt 0 ]; then
      local hint=""
      for tool in "${missing[@]}"; do
        case "$tool" in
          emcc)        hint+="emcc: git clone https://github.com/emscripten-core/emsdk && ./emsdk install ${EMSDK_VERSION} && ./emsdk activate ${EMSDK_VERSION}; " ;;
          wasm-objdump) hint+="wasm-objdump: sudo apt-get install -y wabt; " ;;
          node)        hint+="node: https://nodejs.org; " ;;
          python3)     hint+="python3: sudo apt-get install -y python3; " ;;
        esac
      done
      fail "P0_ENV" 2 "Missing tools: ${missing[*]}" "${hint}"
    fi
  fi

  local active_version
  active_version="$(emcc --version | head -n1 | grep -oP '[0-9]+\.[0-9]+\.[0-9]+' | head -n1 || true)"
  if [ "${active_version}" != "${EMSDK_VERSION}" ]; then
    fail "P0_ENV" 2 "emsdk version mismatch: expected ${EMSDK_VERSION}, got ${active_version:-unknown}" \
      "Run: ./emsdk install ${EMSDK_VERSION} && ./emsdk activate ${EMSDK_VERSION}"
  fi
}

# ---------------------------------------------------------------------------
# P1: Build
# ---------------------------------------------------------------------------
check_build() {
  local build_ok=false
  local max_attempts=1
  [ "${AUTO_FIX}" = "true" ] && max_attempts=2

  for attempt in $(seq 1 ${max_attempts}); do
    if [ "${attempt}" -gt 1 ]; then
      echo "  Auto-fix: retrying build (attempt ${attempt}/${max_attempts})..." >&2
      rm -rf "${BUILD_DIR}"
    fi

    if ! emcmake cmake -S "${PROJECT_DIR}" -B "${BUILD_DIR}" \
        -DSPZ2GLB_BUILD_WASM=ON -DSPZ2GLB_WASM_PROFILE=compat >/dev/null 2>&1; then
      if [ "${attempt}" -lt "${max_attempts}" ]; then
        echo "  Auto-fix: cmake configure failed, retrying..." >&2
        continue
      fi
      fail "P1_BUILD" 3 "emcmake configuration failed" "Check CMake output: emcmake cmake -S . -B ${BUILD_DIR} -DSPZ2GLB_BUILD_WASM=ON"
    fi

    if ! emmake cmake --build "${BUILD_DIR}" --target spz2glb-wasm spz_verify-wasm --parallel >/dev/null 2>&1; then
      if [ "${AUTO_FIX}" = "true" ] && [ "${attempt}" -lt "${max_attempts}" ]; then
        echo "  Auto-fix: build failed, retrying with --parallel 1..." >&2
        if emmake cmake --build "${BUILD_DIR}" --target spz2glb-wasm spz_verify-wasm --parallel 1 >/dev/null 2>&1; then
          build_ok=true
          break
        fi
        continue
      fi
      fail "P1_BUILD" 3 "emmake build failed" "Check compiler errors in src/ source files"
    fi
    build_ok=true
    break
  done

  if [ "${build_ok}" != "true" ]; then
    fail "P1_BUILD" 3 "emmake build failed after ${max_attempts} attempts"
  fi

  # Locate and copy artifacts to dist/
  mkdir -p "${DIST_DIR}"
  local wasm_files
  wasm_files=$(find "${BUILD_DIR}" -type f -name '*.wasm' | sort || true)
  local js_files
  js_files=$(find "${BUILD_DIR}" -type f -name '*.js' 2>/dev/null | sort || true)

  for f in ${wasm_files}; do
    cp "$f" "${DIST_DIR}/"
    echo "  Copied WASM: $f -> ${DIST_DIR}/" >&2
  done
  for f in ${js_files}; do
    cp "$f" "${DIST_DIR}/"
    echo "  Copied JS: $f -> ${DIST_DIR}/" >&2
  done
}

# ---------------------------------------------------------------------------
# P2: Symbol exports — spz2glb-wasm C API + spz_verify-wasm runtime check
# ---------------------------------------------------------------------------
check_symbols() {
  local missing=()
  local cmake_file="${PROJECT_DIR}/CMakeLists.txt"

  # === spz2glb-wasm C API exports ===
  # Verify each _spz2glb_* symbol exists in CMakeLists.txt EXPORTED_FUNCTIONS
  for sym in "${REQUIRED_SPZ2GLB_SYMBOLS[@]}"; do
    if ! grep -qE "\b${sym}\b" "${cmake_file}" 2>/dev/null; then
      missing+=("${sym}")
    fi
  done

  # Also verify _malloc and _free
  for sym in "_malloc" "_free"; do
    if ! grep -qE "\b${sym}\b" "${cmake_file}" 2>/dev/null; then
      missing+=("${sym}")
    fi
  done

  if [ ${#missing[@]} -gt 0 ]; then
    fail "P2_SYMBOL" 4 "Missing exported symbol(s) in CMakeLists.txt: ${missing[*]}" \
      "Check EXPORTED_FUNCTIONS in CMakeLists.txt §WASM build"
  fi

  # === spz_verify-wasm: no EXPORTED_FUNCTIONS → just verify WASM binary exists ===
  if [ ! -f "${SPZ_VERIFY_WASM}" ]; then
    fail "P2_SYMBOL" 4 "spz_verify-wasm binary not found at ${SPZ_VERIFY_WASM}" \
      "Ensure 'emmake cmake --build ... --target spz_verify-wasm' succeeds"
  fi

  # Emscripten underscore prefix check: spz2glb symbols should already use _ prefix
  local emsc_issues=0
  if grep -qE 'EXPORTED_FUNCTIONS=[^"]*\bspz2glb_(?!alloc|free|reserve|get|convert|release|validate|sizeof|memory|reset)\b' "${cmake_file}" 2>/dev/null; then
    echo "  WARN: EXPORTED_FUNCTIONS contains spz2glb_* without Emscripten underscore prefix (should be _spz2glb_*)" >&2
    emsc_issues=1
  fi
  if [ "${emsc_issues}" -gt 0 ] && [ "${STRICT}" = "true" ]; then
    fail "P2_EMSCRIPTEN_PREFIX" 4 "EXPORTED_FUNCTIONS missing underscore prefix for spz2glb_ symbols"
  fi
}

# ---------------------------------------------------------------------------
# P3: Artifact — existence, size range, pair consistency
# ---------------------------------------------------------------------------
check_artifact() {
  # spz2glb-wasm
  if [ ! -f "${SPZ2GLB_JS}" ]; then
    fail "P3_ARTIFACT" 5 "spz2glb JS glue not found: ${SPZ2GLB_JS}"
  fi
  if [ ! -f "${SPZ2GLB_WASM}" ]; then
    fail "P3_ARTIFACT" 5 "spz2glb WASM binary not found: ${SPZ2GLB_WASM}"
  fi

  # spz_verify-wasm
  if [ ! -f "${SPZ_VERIFY_JS}" ]; then
    echo "  INFO: spz_verify JS glue not found (spz_verify-wasm may not generate JS glue)" >&2
  fi
  if [ ! -f "${SPZ_VERIFY_WASM}" ]; then
    fail "P3_ARTIFACT" 5 "spz_verify WASM binary not found: ${SPZ_VERIFY_WASM}"
  fi

  # Size check: spz2glb.wasm
  local size_spz2glb
  size_spz2glb="$(stat -c%s "${SPZ2GLB_WASM}" 2>/dev/null || stat -f%z "${SPZ2GLB_WASM}" 2>/dev/null || echo 0)"
  if [ "${size_spz2glb}" -lt "${WASM_MIN_BYTES}" ] || [ "${size_spz2glb}" -gt "${WASM_MAX_BYTES}" ]; then
    fail "P3_ARTIFACT" 5 "spz2glb WASM size ${size_spz2glb} bytes out of range [${WASM_MIN_BYTES}, ${WASM_MAX_BYTES}]"
  fi

  # Size check: spz_verify.wasm
  local size_verify
  size_verify="$(stat -c%s "${SPZ_VERIFY_WASM}" 2>/dev/null || stat -f%z "${SPZ_VERIFY_WASM}" 2>/dev/null || echo 0)"
  if [ "${size_verify}" -lt "${WASM_MIN_BYTES}" ] || [ "${size_verify}" -gt "${WASM_MAX_BYTES}" ]; then
    fail "P3_ARTIFACT" 5 "spz_verify WASM size ${size_verify} bytes out of range [${WASM_MIN_BYTES}, ${WASM_MAX_BYTES}]"
  fi

  # JS glue should be small (< 500KB)
  local js_size
  js_size="$(stat -c%s "${SPZ2GLB_JS}" 2>/dev/null || stat -f%z "${SPZ2GLB_JS}" 2>/dev/null || echo 0)"
  if [ "${js_size}" -gt 524288 ]; then
    echo "  WARN: spz2glb JS glue size ${js_size} bytes > 512KB" >&2
  fi

  # Check for unresolved git conflict markers in all source files
  local conflict_count=0
  local src_files="${PROJECT_DIR}/src/spz2glb_core.cpp ${PROJECT_DIR}/src/spz2glb_wasm_c_api.cpp ${PROJECT_DIR}/src/spz_verifier.cpp ${PROJECT_DIR}/src/spz_verify.cpp ${PROJECT_DIR}/CMakeLists.txt"
  for f in ${src_files}; do
    if [ -f "${f}" ]; then
      local markers
      markers="$(grep -cE '<<<<<<< |=======$|>>>>>>> ' "${f}" 2>/dev/null || true)"
      if [ "${markers}" -gt 0 ]; then
        echo "  ERROR: Found ${markers} unresolved conflict marker(s) in ${f}" >&2
        conflict_count=$((conflict_count + markers))
      fi
    fi
  done
  if [ "${conflict_count}" -gt 0 ]; then
    fail "P3_ARTIFACT" 5 "Found ${conflict_count} unresolved conflict marker(s) in source files"
  fi
}

# ---------------------------------------------------------------------------
# P4: WASM binary analysis — import/export/memory verification
# ---------------------------------------------------------------------------
check_wasm_analysis() {
  local analysis_file="${BUILD_DIR}/wasm_analysis.txt"
  mkdir -p "${BUILD_DIR}"

  if ! wasm-objdump -x "${SPZ2GLB_WASM}" > "${analysis_file}" 2>/dev/null; then
    fail "P4_WASM_ANALYSIS" 6 "wasm-objdump failed on spz2glb.wasm" "Ensure wabt is installed"
  fi

  # Verify exports contain expected C API symbols
  local export_section
  export_section=$(grep -A 50 "Export" "${analysis_file}" | head -50 || true)
  local missing_exports=0
  local sample_symbols=("_spz2glb_alloc" "_spz2glb_convert" "_spz2glb_get_version" "_malloc")
  for sym in "${sample_symbols[@]}"; do
    if ! echo "${export_section}" | grep -qE "\b${sym}\b"; then
      echo "  WARN: Expected export '${sym}' not found in WASM binary export table" >&2
      missing_exports=$((missing_exports + 1))
    fi
  done
  if [ "${missing_exports}" -gt 0 ]; then
    fail "P4_WASM_ANALYSIS" 6 "${missing_exports} expected export(s) missing in spz2glb.wasm"
  fi

  # Verify spz_verify.wasm exports exist
  if ! wasm-objdump -x "${SPZ_VERIFY_WASM}" > /dev/null 2>&1; then
    fail "P4_WASM_ANALYSIS" 6 "wasm-objdump failed on spz_verify.wasm"
  fi
  local verify_exports
  verify_exports=$(wasm-objdump -x "${SPZ_VERIFY_WASM}" 2>/dev/null | grep -A 20 "Export" | head -20 || true)
  if [ -z "${verify_exports}" ]; then
    echo "  INFO: spz_verify.wasm has no export section (no EXPORTED_FUNCTIONS set)" >&2
  fi

  # Check no unexpected imports
  local import_section
  import_section=$(grep -A 20 "Import" "${analysis_file}" | head -20 || true)
  local unexpected_imports=()
  if echo "${import_section}" | grep -qE "emscripten_memcpy|dlopen|pthread"; then
    unexpected_imports+=("emscripten_memcpy|dlopen|pthread")
  fi
  if [ ${#unexpected_imports[@]} -gt 0 ]; then
    echo "  WARN: Unexpected imports: ${unexpected_imports[*]} (may break -sFILESYSTEM=0)" >&2
  fi
}

# ---------------------------------------------------------------------------
# P5: Workflow lint — actionlint + zizmor
# ---------------------------------------------------------------------------
check_workflow() {
  if git -C "${PROJECT_DIR}" diff --quiet -- .github/workflows/ 2>/dev/null && \
     git -C "${PROJECT_DIR}" diff --cached --quiet -- .github/workflows/ 2>/dev/null; then
    if [ "${STRICT}" != "true" ]; then
      return 0
    fi
  fi

  local missing=()
  command -v actionlint >/dev/null 2>&1 || missing+=("actionlint")
  command -v zizmor >/dev/null 2>&1 || missing+=("zizmor")

  if [ ${#missing[@]} -gt 0 ]; then
    if [ "${STRICT}" = "true" ]; then
      fail "P5_WORKFLOW" 7 "Strict mode requires tools: ${missing[*]}"
    fi
    return 0
  fi

  local had_error=false
  for wf in "${PROJECT_DIR}/.github/workflows/"*.yml "${PROJECT_DIR}/.github/workflows/"*.yaml; do
    [ -e "${wf}" ] || continue
    if ! actionlint "${wf}" >/dev/null 2>&1; then
      had_error=true
    fi
    if ! zizmor "${wf}" >/dev/null 2>&1; then
      had_error=true
    fi
  done

  if [ "${had_error}" = "true" ]; then
    fail "P5_WORKFLOW" 7 "actionlint or zizmor reported issues"
  fi
}

# ---------------------------------------------------------------------------
# P6: File integrity (UTF-8 validity)
# ---------------------------------------------------------------------------
check_file_integrity() {
  local failures=0

  # UTF-8 validity for C++ sources
  while IFS= read -r -d '' f; do
    if ! python3 -c "open('$f','rb').read().decode('utf-8')" 2>/dev/null; then
      echo "  INVALID UTF-8: $f" >&2
      failures=$((failures + 1))
    fi
  done < <(find "${PROJECT_DIR}/src" -name '*.cpp' -o -name '*.h' -o -name '*.hpp' 2>/dev/null | tr '\n' '\0')

  # CMake syntax: detect set(VAR::NAME ...) which is invalid — :: is reserved
  # for ALIAS/IMPORTED target namespaces and cannot appear in set() variable names.
  local cmake_file="${PROJECT_DIR}/CMakeLists.txt"
  if [ -f "${cmake_file}" ]; then
    local bad_sets
    bad_sets="$(grep -cPn 'set\([A-Za-z_][A-Za-z0-9_]*::' "${cmake_file}" 2>/dev/null || true)"
    if [ "${bad_sets}" -gt 0 ]; then
      echo "  ERROR: Found ${bad_sets} invalid set(XXX::YYY) pattern(s) in CMakeLists.txt" >&2
      echo "  :: in variable name is reserved for CMake ALIAS/IMPORTED targets" >&2
      grep -Pn 'set\([A-Za-z_][A-Za-z0-9_]*::' "${cmake_file}" >&2
      failures=$((failures + bad_sets))
    fi
  fi

  # Cross-platform #ifdef guard check: detect header files with #ifdef _WIN32
  # that reference POSIX-only members (#else branch) outside conditional guards.
  # This catches MSVC failures where variables declared only in the #else branch
  # are referenced in move constructor initializer lists or assignment operators.
  local guard_issues=0
  while IFS= read -r -d '' f; do
    if grep -q '#ifdef _WIN32' "${f}" 2>/dev/null; then
      local posix_members
      posix_members="$(sed -n '/#else/,/#endif/p' "${f}" | grep -oP '^\s+\w+\s+\w+_\s*;' | grep -oP '\w+_' || true)"
      if [ -n "${posix_members}" ]; then
        for member in ${posix_members}; do
          local total_refs
          total_refs="$(grep -cP "\b${member}\b" "${f}" 2>/dev/null || true)"
          local inside_else_refs
          inside_else_refs="$(sed -n '/#else/,/#endif/p' "${f}" | grep -cP "\b${member}\b" 2>/dev/null || true)"
          if [ "${total_refs}" -gt "${inside_else_refs}" ] 2>/dev/null; then
            echo "  WARNING: '${member}' declared in #else (POSIX) branch of ${f##*/}" >&2
            echo "    has ${total_refs} total refs, ${inside_else_refs} inside #else guard." >&2
            echo "    Exposed refs outside guard will fail MSVC (Windows) compilation." >&2
            echo "    Wrap all references inside #ifdef _WIN32 / #else / #endif." >&2
            guard_issues=$((guard_issues + 1))
          fi
        done
      fi
    fi
  done < <(find "${PROJECT_DIR}/src" -name '*.h' -o -name '*.hpp' 2>/dev/null | tr '\n' '\0')
  failures=$((failures + guard_issues))

  if [ "${failures}" -gt 0 ]; then
    fail "P6_INTEGRITY" 8 "${failures} integrity issue(s) (UTF-8, CMake syntax, cross-platform guard)"
  fi
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
main() {
  cd "${PROJECT_DIR}"
  check_environment
  if [ "${SKIP_BUILD}" != "true" ]; then
    check_build
  fi
  check_symbols
  check_artifact
  check_wasm_analysis
  check_workflow
  check_file_integrity
  pass
}

main "$@"

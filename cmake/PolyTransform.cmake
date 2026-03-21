# =============================================================================
# PolyTransform.cmake - Acquire the poly-transform LLVM pass tool
# =============================================================================
# Acquires poly-transform (in order of preference):
#   1. Pre-installed on PATH
#   2. Built from in-tree source (tools/poly-transform)
#
# The pass constrains instruction selection to a random per-build subset,
# making static instruction-pattern signatures effectively impossible.
# It operates on LLVM IR alongside pic-transform.
#
# Outputs (at most one of these will be set):
#   POLY_TRANSFORM_PLUGIN      - Path to loadable pass plugin (.so/.dylib/.dll)
#   POLY_TRANSFORM_EXECUTABLE  - Path to standalone poly-transform binary
#   POLY_TRANSFORM_TARGET      - CMake target to add as build dependency
#   POLY_TRANSFORM_SEED        - Build-unique seed derived from timestamp

include_guard(GLOBAL)

# Platform-specific artifact names
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(_ply_bin_name    "poly-transform.exe")
    set(_ply_plugin_name "PolyTransform.dll")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(_ply_bin_name    "poly-transform")
    set(_ply_plugin_name "PolyTransform.dylib")
else()
    set(_ply_bin_name    "poly-transform")
    set(_ply_plugin_name "PolyTransform.so")
endif()

pir_log_debug_at("poly-transform" "Host: ${CMAKE_HOST_SYSTEM_NAME}, binary=${_ply_bin_name}, plugin=${_ply_plugin_name}")

# -----------------------------------------------------------------------------
# Helper: set output variables and return
# -----------------------------------------------------------------------------
macro(_ply_found mode path)
    if("${mode}" STREQUAL "plugin")
        set(POLY_TRANSFORM_PLUGIN     "${path}")
        set(POLY_TRANSFORM_EXECUTABLE "")
    else()
        set(POLY_TRANSFORM_PLUGIN     "")
        set(POLY_TRANSFORM_EXECUTABLE "${path}")
    endif()
    set(POLY_TRANSFORM_TARGET "")
    pir_log_at("poly-transform" "${ARGN}")
    pir_log_verbose_at("poly-transform" "Path: ${path}")
    pir_log_verbose_at("poly-transform" "Mode: ${mode}")
    return()
endmacro()

# Helper: find a built artifact in both single-config and multi-config layouts
macro(_ply_find_artifact var name build_dir)
    set(${var} "")
    if(EXISTS "${build_dir}/${name}")
        set(${var} "${build_dir}/${name}")
    elseif(EXISTS "${build_dir}/Release/${name}")
        set(${var} "${build_dir}/Release/${name}")
    endif()
    pir_log_debug_at("poly-transform" "Artifact search: ${name} in ${build_dir} -> ${${var}}")
endmacro()

# -----------------------------------------------------------------------------
# Generate seed from build timestamp (mirrors DJB2 FNV-1a seeding)
# -----------------------------------------------------------------------------
string(TIMESTAMP _ply_timestamp "%Y-%m-%d" UTC)
string(MD5 _ply_date_hash "${_ply_timestamp}")
string(SUBSTRING "${_ply_date_hash}" 0 16 _ply_seed_hex)
math(EXPR POLY_TRANSFORM_SEED "0x${_ply_seed_hex}" OUTPUT_FORMAT HEXADECIMAL)

pir_log_verbose_at("poly-transform" "Build date: ${_ply_timestamp}")
pir_log_verbose_at("poly-transform" "Seed: ${POLY_TRANSFORM_SEED}")

# =============================================================================
# Strategy 1: Pre-installed on PATH
# =============================================================================
pir_log_debug_at("poly-transform" "Strategy 1: Searching PATH...")
find_program(_ply_system_bin poly-transform)

if(_ply_system_bin)
    _ply_found(standalone "${_ply_system_bin}" "Using system binary")
endif()
pir_log_verbose_at("poly-transform" "Not found on PATH, trying in-tree build")

# =============================================================================
# Strategy 2: Build from in-tree source
# =============================================================================
pir_log_debug_at("poly-transform" "Strategy 2: Building from in-tree source...")

set(_ply_source_dir "${PIR_ROOT_DIR}/tools/poly-transform")

if(NOT EXISTS "${_ply_source_dir}/CMakeLists.txt")
    pir_log_at("poly-transform" "Source not found — tool disabled")
    set(POLY_TRANSFORM_PLUGIN "")
    set(POLY_TRANSFORM_EXECUTABLE "")
    set(POLY_TRANSFORM_TARGET "")
    return()
endif()
pir_log_debug_at("poly-transform" "Source found: ${_ply_source_dir}")

# ── Locate LLVM cmake config (reuse the one found for pic-transform) ────
if(NOT DEFINED POLY_TRANSFORM_LLVM_DIR)
    if(DEFINED PIC_TRANSFORM_LLVM_DIR)
        set(POLY_TRANSFORM_LLVM_DIR "${PIC_TRANSFORM_LLVM_DIR}")
        pir_log_debug_at("poly-transform" "Reusing LLVM_DIR from pic-transform: ${POLY_TRANSFORM_LLVM_DIR}")
    else()
        get_filename_component(_ply_compiler_real "${CMAKE_CXX_COMPILER}" REALPATH)
        get_filename_component(_ply_compiler_dir  "${_ply_compiler_real}" DIRECTORY)
        get_filename_component(_ply_llvm_root     "${_ply_compiler_dir}/.." ABSOLUTE)
        foreach(_dir "${_ply_llvm_root}/lib/cmake/llvm"
                     "${_ply_llvm_root}/lib64/cmake/llvm")
            if(EXISTS "${_dir}/LLVMConfig.cmake")
                set(POLY_TRANSFORM_LLVM_DIR "${_dir}")
                break()
            endif()
        endforeach()
    endif()
endif()

if(NOT DEFINED POLY_TRANSFORM_LLVM_DIR OR NOT EXISTS "${POLY_TRANSFORM_LLVM_DIR}/LLVMConfig.cmake")
    pir_log_at("poly-transform" "LLVM dev files not found — tool disabled")
    set(POLY_TRANSFORM_PLUGIN "")
    set(POLY_TRANSFORM_EXECUTABLE "")
    set(POLY_TRANSFORM_TARGET "")
    return()
endif()

pir_log_verbose_at("poly-transform" "LLVM_DIR: ${POLY_TRANSFORM_LLVM_DIR}")

# ── Configure ───────────────────────────────────────────────────────────
set(_ply_build_dir "${CMAKE_BINARY_DIR}/poly-transform-build")

get_filename_component(_ply_compiler_real "${CMAKE_CXX_COMPILER}" REALPATH)
get_filename_component(_ply_compiler_dir  "${_ply_compiler_real}" DIRECTORY)

if(NOT EXISTS "${_ply_build_dir}/CMakeCache.txt")
    pir_log_at("poly-transform" "Configuring (LLVM_DIR=${POLY_TRANSFORM_LLVM_DIR})")
    # Force standalone mode (not plugin) to avoid LTO conflicts.
    # The pass runs in the compile pipeline as a separate tool step:
    #   clang -emit-llvm → pic-transform → poly-transform → clang -c
    execute_process(
        COMMAND ${CMAKE_COMMAND}
            -S "${_ply_source_dir}"
            -B "${_ply_build_dir}"
            -DLLVM_DIR=${POLY_TRANSFORM_LLVM_DIR}
            -DLLVM_ENABLE_PLUGINS=OFF
            -DCMAKE_BUILD_TYPE=Release
            -DCMAKE_C_COMPILER=${_ply_compiler_dir}/clang
            -DCMAKE_CXX_COMPILER=${_ply_compiler_dir}/clang++
            -DSTATIC_LINK=ON
        RESULT_VARIABLE _ply_cfg_rc
        OUTPUT_VARIABLE _ply_cfg_out
        ERROR_VARIABLE  _ply_cfg_err
    )
    if(NOT _ply_cfg_rc EQUAL 0)
        pir_log_at("poly-transform" "Configure failed (rc=${_ply_cfg_rc}) — tool disabled")
        pir_log_debug_at("poly-transform" "${_ply_cfg_err}")
        set(POLY_TRANSFORM_PLUGIN "")
        set(POLY_TRANSFORM_EXECUTABLE "")
        set(POLY_TRANSFORM_TARGET "")
        return()
    endif()
else()
    pir_log_debug_at("poly-transform" "Reusing cached build dir: ${_ply_build_dir}")
endif()

# ── Build ───────────────────────────────────────────────────────────────
pir_log_at("poly-transform" "Building from source...")
string(TIMESTAMP _ply_build_start "%s")
execute_process(
    COMMAND ${CMAKE_COMMAND} --build "${_ply_build_dir}" --config Release
    RESULT_VARIABLE _ply_build_rc
    OUTPUT_VARIABLE _ply_build_out
    ERROR_VARIABLE  _ply_build_err
)
string(TIMESTAMP _ply_build_end "%s")
math(EXPR _ply_build_elapsed "${_ply_build_end} - ${_ply_build_start}")

if(NOT _ply_build_rc EQUAL 0)
    pir_log_at("poly-transform" "Build failed (rc=${_ply_build_rc}, ${_ply_build_elapsed}s) — tool disabled")
    pir_log_debug_at("poly-transform" "${_ply_build_err}")
    set(POLY_TRANSFORM_PLUGIN "")
    set(POLY_TRANSFORM_EXECUTABLE "")
    set(POLY_TRANSFORM_TARGET "")
    return()
endif()
pir_log_at("poly-transform" "Build succeeded (${_ply_build_elapsed}s)")

# ── Locate artifact (standalone only — avoids LTO conflicts) ────────────
_ply_find_artifact(_ply_bin "${_ply_bin_name}" "${_ply_build_dir}")
if(_ply_bin)
    _ply_found(standalone "${_ply_bin}" "Built standalone from source")
endif()

pir_log_at("poly-transform" "Build succeeded but no artifact found — tool disabled")
set(POLY_TRANSFORM_PLUGIN "")
set(POLY_TRANSFORM_EXECUTABLE "")
set(POLY_TRANSFORM_TARGET "")

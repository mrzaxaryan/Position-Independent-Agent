# =============================================================================
# PICTransform.cmake - Acquire the pic-transform LLVM pass tool
# =============================================================================
# Acquires pic-transform (in order of preference):
#   1. Pre-installed on PATH
#   2. Built from in-tree source (tools/pic-transform)
#
# The pass eliminates .rodata/.rdata/.data/.bss sections by transforming global
# constants into stack-local immediate values automatically during compilation.
# It also replaces function pointer references with PC-relative inline assembly
# and lowers x86 FP operations that generate constant-pool entries.
#
# Outputs (at most one of these will be set):
#   PIC_TRANSFORM_PLUGIN      - Path to loadable pass plugin (.so/.dylib/.dll)
#   PIC_TRANSFORM_EXECUTABLE  - Path to standalone pic-transform binary
#   PIC_TRANSFORM_TARGET      - CMake target to add as build dependency

include_guard(GLOBAL)

# Platform-specific artifact names
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(_pt_bin_name    "pic-transform.exe")
    set(_pt_plugin_name "PICTransform.dll")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(_pt_bin_name    "pic-transform")
    set(_pt_plugin_name "PICTransform.dylib")
else()
    set(_pt_bin_name    "pic-transform")
    set(_pt_plugin_name "PICTransform.so")
endif()

pir_log_debug_at("pic-transform" "Host: ${CMAKE_HOST_SYSTEM_NAME}, binary=${_pt_bin_name}, plugin=${_pt_plugin_name}")

# -----------------------------------------------------------------------------
# Helper: set output variables and return
# -----------------------------------------------------------------------------
macro(_pt_found mode path)
    if("${mode}" STREQUAL "plugin")
        set(PIC_TRANSFORM_PLUGIN     "${path}")
        set(PIC_TRANSFORM_EXECUTABLE "")
    else()
        set(PIC_TRANSFORM_PLUGIN     "")
        set(PIC_TRANSFORM_EXECUTABLE "${path}")
    endif()
    set(PIC_TRANSFORM_TARGET "")
    pir_log_at("pic-transform" "${ARGN}")
    pir_log_verbose_at("pic-transform" "Path: ${path}")
    pir_log_verbose_at("pic-transform" "Mode: ${mode}")
    return()
endmacro()

# Helper: find a built artifact in both single-config and multi-config layouts
macro(_pt_find_artifact var name build_dir)
    set(${var} "")
    if(EXISTS "${build_dir}/${name}")
        set(${var} "${build_dir}/${name}")
    elseif(EXISTS "${build_dir}/Release/${name}")
        set(${var} "${build_dir}/Release/${name}")
    endif()
    pir_log_debug_at("pic-transform" "Artifact search: ${name} in ${build_dir} -> ${${var}}")
endmacro()

# =============================================================================
# Strategy 1: Pre-installed on PATH
# =============================================================================
pir_log_debug_at("pic-transform" "Strategy 1: Searching PATH...")
find_program(_pt_system_bin pic-transform)

if(_pt_system_bin)
    _pt_found(standalone "${_pt_system_bin}" "Using system binary")
endif()
pir_log_verbose_at("pic-transform" "Not found on PATH, trying in-tree build")

# =============================================================================
# Strategy 2: Build from in-tree source
# =============================================================================
# Builds tools/pic-transform if LLVM dev files are available.
# Override LLVM location: -DPIC_TRANSFORM_LLVM_DIR=/path/to/lib/cmake/llvm

pir_log_debug_at("pic-transform" "Strategy 2: Building from in-tree source...")

set(_pt_source_dir "${PIR_ROOT_DIR}/tools/pic-transform")

if(NOT EXISTS "${_pt_source_dir}/CMakeLists.txt")
    message(FATAL_ERROR "[pir:pic-transform] Source not found at ${_pt_source_dir}")
endif()
pir_log_debug_at("pic-transform" "Source found: ${_pt_source_dir}")

# ── Locate LLVM cmake config from the compiler installation ─────────────
get_filename_component(_pt_compiler_real "${CMAKE_CXX_COMPILER}" REALPATH)
get_filename_component(_pt_compiler_dir  "${_pt_compiler_real}" DIRECTORY)
get_filename_component(_pt_llvm_root     "${_pt_compiler_dir}/.." ABSOLUTE)

pir_log_debug_at("pic-transform" "Compiler: ${CMAKE_CXX_COMPILER}")
pir_log_debug_at("pic-transform" "Compiler real path: ${_pt_compiler_real}")
pir_log_debug_at("pic-transform" "LLVM root (inferred): ${_pt_llvm_root}")

if(NOT DEFINED PIC_TRANSFORM_LLVM_DIR)
    foreach(_dir "${_pt_llvm_root}/lib/cmake/llvm"
                 "${_pt_llvm_root}/lib64/cmake/llvm")
        pir_log_debug_at("pic-transform" "Probing: ${_dir}/LLVMConfig.cmake")
        if(EXISTS "${_dir}/LLVMConfig.cmake")
            set(PIC_TRANSFORM_LLVM_DIR "${_dir}")
            pir_log_debug_at("pic-transform" "Found LLVMConfig.cmake")
            break()
        endif()
    endforeach()
else()
    pir_log_debug_at("pic-transform" "Using user-provided LLVM_DIR: ${PIC_TRANSFORM_LLVM_DIR}")
endif()

if(NOT DEFINED PIC_TRANSFORM_LLVM_DIR OR NOT EXISTS "${PIC_TRANSFORM_LLVM_DIR}/LLVMConfig.cmake")
    message(FATAL_ERROR
        "[pir:pic-transform] LLVM dev files not found (need LLVMConfig.cmake)\n"
        "Searched: ${_pt_llvm_root}/lib/cmake/llvm, ${_pt_llvm_root}/lib64/cmake/llvm\n"
        "Override with: -DPIC_TRANSFORM_LLVM_DIR=/path/to/lib/cmake/llvm")
endif()

pir_log_verbose_at("pic-transform" "LLVM_DIR: ${PIC_TRANSFORM_LLVM_DIR}")

# ── Configure ───────────────────────────────────────────────────────────
set(_pt_build_dir "${CMAKE_BINARY_DIR}/pic-transform-build")

if(NOT EXISTS "${_pt_build_dir}/CMakeCache.txt")
    pir_log_at("pic-transform" "Configuring (LLVM_DIR=${PIC_TRANSFORM_LLVM_DIR})")
    pir_log_verbose_at("pic-transform" "Source: ${_pt_source_dir}")
    pir_log_verbose_at("pic-transform" "Build dir: ${_pt_build_dir}")
    pir_log_verbose_at("pic-transform" "Compiler: ${_pt_compiler_dir}/clang++")
    pir_log_verbose_at("pic-transform" "Static link: ON")
    string(TIMESTAMP _pt_cfg_start "%s")
    execute_process(
        COMMAND ${CMAKE_COMMAND}
            -S "${_pt_source_dir}"
            -B "${_pt_build_dir}"
            -DLLVM_DIR=${PIC_TRANSFORM_LLVM_DIR}
            -DCMAKE_BUILD_TYPE=Release
            -DCMAKE_C_COMPILER=${_pt_compiler_dir}/clang
            -DCMAKE_CXX_COMPILER=${_pt_compiler_dir}/clang++
            -DSTATIC_LINK=ON
        RESULT_VARIABLE _pt_cfg_rc
        OUTPUT_VARIABLE _pt_cfg_out
        ERROR_VARIABLE  _pt_cfg_err
    )
    string(TIMESTAMP _pt_cfg_end "%s")
    math(EXPR _pt_cfg_elapsed "${_pt_cfg_end} - ${_pt_cfg_start}")
    if(NOT _pt_cfg_rc EQUAL 0)
        message(FATAL_ERROR "[pir:pic-transform] Configure failed (rc=${_pt_cfg_rc}, ${_pt_cfg_elapsed}s):\n${_pt_cfg_err}")
    endif()
    pir_log_debug_at("pic-transform" "Configure succeeded (${_pt_cfg_elapsed}s)")
else()
    pir_log_debug_at("pic-transform" "Reusing cached build dir: ${_pt_build_dir}")
endif()

# ── Build ───────────────────────────────────────────────────────────────
pir_log_at("pic-transform" "Building from source...")
string(TIMESTAMP _pt_build_start "%s")
execute_process(
    COMMAND ${CMAKE_COMMAND} --build "${_pt_build_dir}" --config Release
    RESULT_VARIABLE _pt_build_rc
    OUTPUT_VARIABLE _pt_build_out
    ERROR_VARIABLE  _pt_build_err
)
string(TIMESTAMP _pt_build_end "%s")
math(EXPR _pt_build_elapsed "${_pt_build_end} - ${_pt_build_start}")

if(NOT _pt_build_rc EQUAL 0)
    message(FATAL_ERROR "[pir:pic-transform] Build failed (rc=${_pt_build_rc}, ${_pt_build_elapsed}s):\n${_pt_build_err}")
endif()
pir_log_at("pic-transform" "Build succeeded (${_pt_build_elapsed}s)")

# ── Locate artifact (prefer plugin over standalone) ─────────────────────
_pt_find_artifact(_pt_plugin "${_pt_plugin_name}" "${_pt_build_dir}")
if(_pt_plugin)
    _pt_found(plugin "${_pt_plugin}" "Built plugin from source")
endif()

_pt_find_artifact(_pt_bin "${_pt_bin_name}" "${_pt_build_dir}")
if(_pt_bin)
    _pt_found(standalone "${_pt_bin}" "Built standalone from source")
endif()

message(FATAL_ERROR "[pir:pic-transform] Build succeeded but no artifact found in ${_pt_build_dir}")

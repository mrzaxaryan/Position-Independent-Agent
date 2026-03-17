# =============================================================================
# ExtractBinary.cmake - Extract and analyze binary output (run via cmake -P)
# =============================================================================
# Usage: cmake -DINPUT_FILE=<exe/elf/macho> -DOUTPUT_DIR=<dir> -P ExtractBinary.cmake
#
# Pipeline:
#   1. Detect binary format (Mach-O vs PE/ELF)
#   2. Extract raw .text / __TEXT,__text section to .bin
#   3. Generate disassembly to .txt
#   4. Extract printable strings to .strings.txt
#   5. Report file sizes and extraction ratio
# =============================================================================

cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED INPUT_FILE OR NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR "[pir:extract] INPUT_FILE and OUTPUT_DIR must be defined")
endif()

if(NOT EXISTS "${INPUT_FILE}")
    message(FATAL_ERROR "[pir:extract] Input file does not exist: ${INPUT_FILE}")
endif()

get_filename_component(_name "${INPUT_FILE}" NAME_WE)
get_filename_component(_input_name "${INPUT_FILE}" NAME)
set(_bin     "${OUTPUT_DIR}/${_name}.bin")
set(_txt     "${OUTPUT_DIR}/${_name}.txt")
set(_strings "${OUTPUT_DIR}/${_name}.strings.txt")

message(STATUS "[pir:extract] Processing ${_input_name}")

# =============================================================================
# Step 1: Detect binary format (Mach-O vs PE/ELF)
# =============================================================================
# Mach-O binaries use __TEXT,__text; PE/ELF use .text
set(_section_name ".text")
set(_is_macho FALSE)

get_filename_component(_ext "${INPUT_FILE}" LAST_EXT)
if("${_ext}" STREQUAL "" OR "${INPUT_FILE}" MATCHES "macos")
    message(STATUS "[pir:extract] Probing for Mach-O format...")
    # IMPORTANT: Provide an explicit output file so llvm-objcopy does NOT modify
    # the input binary in-place. Mach-O round-tripping through objcopy can alter
    # load commands, alignment, or code signature data, producing a binary that
    # macOS (especially ARM64 with strict code signing) rejects with SIGKILL.
    set(_objcopy_temp "${INPUT_FILE}.objcopy_temp")
    execute_process(
        COMMAND llvm-objcopy "--dump-section=__TEXT,__text=${_bin}" "${INPUT_FILE}" "${_objcopy_temp}"
        ERROR_VARIABLE _err
        RESULT_VARIABLE _res
    )
    if(_res EQUAL 0)
        file(REMOVE "${_objcopy_temp}")
        set(_is_macho TRUE)
        set(_section_name "__TEXT,__text")
        message(STATUS "[pir:extract] Detected Mach-O binary (section: __TEXT,__text)")
    else()
        message(STATUS "[pir:extract] Not Mach-O, using PE/ELF (section: .text)")
    endif()
else()
    message(STATUS "[pir:extract] PE/ELF binary (extension: ${_ext})")
endif()

# =============================================================================
# Step 2: Extract raw code section
# =============================================================================
if(NOT _is_macho)
    message(STATUS "[pir:extract] Extracting ${_section_name} section...")
    set(_objcopy_temp "${INPUT_FILE}.objcopy_temp")
    set(_dump_arg "--dump-section=${_section_name}=${_bin}")
    execute_process(
        COMMAND llvm-objcopy ${_dump_arg} "${INPUT_FILE}" "${_objcopy_temp}"
        ERROR_VARIABLE _err
        RESULT_VARIABLE _res
    )
    file(REMOVE "${_objcopy_temp}")
    if(NOT _res EQUAL 0)
        message(FATAL_ERROR "[pir:extract] llvm-objcopy failed (rc=${_res}): ${_err}")
    endif()
endif()

if(NOT EXISTS "${_bin}")
    message(FATAL_ERROR "[pir:extract] Section extraction produced no output: ${_bin}")
endif()

# =============================================================================
# Step 3: Generate disassembly
# =============================================================================
if(_is_macho)
    set(_objdump_section "__text")
else()
    set(_objdump_section ".text")
endif()

message(STATUS "[pir:extract] Generating disassembly (section: ${_objdump_section})...")
execute_process(
    COMMAND llvm-objdump -d -s -h -j ${_objdump_section} "${INPUT_FILE}"
    OUTPUT_FILE "${_txt}"
    ERROR_VARIABLE _err
    RESULT_VARIABLE _res
)
if(NOT _res EQUAL 0)
    message(FATAL_ERROR "[pir:extract] llvm-objdump failed (rc=${_res}): ${_err}")
endif()

# =============================================================================
# Step 4: Extract strings
# =============================================================================
message(STATUS "[pir:extract] Extracting printable strings...")
execute_process(
    COMMAND llvm-strings "${INPUT_FILE}"
    OUTPUT_FILE "${_strings}"
    ERROR_VARIABLE _err
    RESULT_VARIABLE _res
)
if(NOT _res EQUAL 0)
    message(FATAL_ERROR "[pir:extract] llvm-strings failed (rc=${_res}): ${_err}")
endif()

# =============================================================================
# Step 5: Report file sizes
# =============================================================================
# Use stat instead of file(SIZE) to avoid 9P filesystem caching issues on WSL
# when the source tree is on a Windows NTFS mount (/mnt/...).
execute_process(
    COMMAND stat -c%s "${INPUT_FILE}"
    OUTPUT_VARIABLE _input_size
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _res
)
if(NOT _res EQUAL 0)
    message(FATAL_ERROR "[pir:extract] Failed to stat ${INPUT_FILE}")
endif()
execute_process(
    COMMAND stat -c%s "${_bin}"
    OUTPUT_VARIABLE _bin_size
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _res
)
if(NOT _res EQUAL 0)
    message(FATAL_ERROR "[pir:extract] Failed to stat ${_bin}")
endif()

# Human-readable size formatting
foreach(_pair "input;${_input_size}" "bin;${_bin_size}")
    list(GET _pair 0 _label)
    list(GET _pair 1 _bytes)
    if(_bytes GREATER_EQUAL 1048576)
        math(EXPR _whole "${_bytes} / 1048576")
        math(EXPR _frac  "(${_bytes} % 1048576) * 10 / 1048576")
        set(_size_${_label} "${_whole}.${_frac} MiB")
    elseif(_bytes GREATER_EQUAL 1024)
        math(EXPR _whole "${_bytes} / 1024")
        math(EXPR _frac  "(${_bytes} % 1024) * 10 / 1024")
        set(_size_${_label} "${_whole}.${_frac} KiB")
    else()
        set(_size_${_label} "${_bytes} B")
    endif()
endforeach()

get_filename_component(_bin_name "${_bin}" NAME)
message(STATUS "[pir:extract] ${_input_name} (${_size_input}) -> ${_bin_name} (${_size_bin})")

# Report extraction ratio
if(_input_size GREATER 0)
    math(EXPR _ratio "${_bin_size} * 100 / ${_input_size}")
    message(STATUS "[pir:extract] Code section is ${_ratio}% of total binary")
endif()

message(STATUS "[pir:extract] Artifacts: ${_bin_name}, ${_name}.txt, ${_name}.strings.txt")

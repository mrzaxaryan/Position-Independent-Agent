# =============================================================================
# PatchElfOSABI.cmake - Patch ELF EI_OSABI byte (run via cmake -P)
# =============================================================================
# Usage: cmake -DELF_FILE=<path> -DOSABI_VALUE=<int> -P PatchElfOSABI.cmake
#
# LLD does not support Solaris-specific ELF emulations (_sol2 variants),
# so the output binary has ELFOSABI_NONE (0). Solaris/illumos kernels
# reject ELFOSABI_NONE with "Exec format error", requiring ELFOSABI_SOLARIS
# (6). This script patches byte 7 (EI_OSABI) of the ELF header in-place.
# =============================================================================

cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED ELF_FILE)
    message(FATAL_ERROR "[pir:osabi] ELF_FILE is required")
endif()
if(NOT DEFINED OSABI_VALUE)
    message(FATAL_ERROR "[pir:osabi] OSABI_VALUE is required")
endif()
if(NOT EXISTS "${ELF_FILE}")
    message(FATAL_ERROR "[pir:osabi] ELF file not found: ${ELF_FILE}")
endif()

# EI_OSABI is byte 7 of the ELF header (offset 7, zero-indexed).
# Patch a single byte in-place without truncating the file.
if(CMAKE_HOST_WIN32)
    # Windows: use PowerShell to seek and write one byte.
    execute_process(
        COMMAND powershell -NoProfile -Command
            "$f=[System.IO.File]::OpenWrite('${ELF_FILE}'); $f.Seek(7,0)|Out-Null; $f.WriteByte(${OSABI_VALUE}); $f.Close()"
        RESULT_VARIABLE _result
    )
else()
    # Unix: use printf + dd to overwrite one byte.
    math(EXPR _d2  "${OSABI_VALUE} / 64")
    math(EXPR _rem "${OSABI_VALUE} % 64")
    math(EXPR _d1  "${_rem} / 8")
    math(EXPR _d0  "${_rem} % 8")
    set(_octal "\\${_d2}${_d1}${_d0}")

    execute_process(
        COMMAND sh -c "printf '${_octal}' | dd of='${ELF_FILE}' bs=1 seek=7 count=1 conv=notrunc 2>/dev/null"
        RESULT_VARIABLE _result
    )
endif()

if(NOT _result EQUAL 0)
    message(FATAL_ERROR "[pir:osabi] Failed to patch ELF OSABI in: ${ELF_FILE}")
endif()

message(STATUS "[pir:osabi] Patched EI_OSABI to ${OSABI_VALUE} in ${ELF_FILE}")

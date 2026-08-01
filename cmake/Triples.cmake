# =============================================================================
# Triples.cmake - Target Triple and Extension Mapping
# =============================================================================

include_guard(GLOBAL)

# Big-endian MIPS32 o32: target mips-unknown-linux-gnu (e.g. Qualcomm Atheros
# QCA953x / TP-Link ath79 big-endian routers) instead of the default
# little-endian mipsel. The o32 ABI/syscall tables are identical for both byte
# orders; only the endianness of emitted code/data differs (driven by the triple).
option(PIR_MIPS_BIGENDIAN "Build big-endian MIPS32 o32 (mips-unknown-linux-gnu) instead of little-endian (mipsel-unknown-linux-gnu)" OFF)

# =============================================================================
# Target Triples
# =============================================================================
set(_triple_windows_i386    "i386-pc-windows-gnu")
set(_triple_windows_x86_64  "x86_64-pc-windows-gnu")
set(_triple_windows_armv7a  "armv7a-pc-windows-gnu")
set(_triple_windows_aarch64 "aarch64-pc-windows-gnu")
set(_triple_linux_i386      "i386-unknown-linux-gnu")
set(_triple_linux_x86_64    "x86_64-unknown-linux-gnu")
set(_triple_linux_armv7a    "armv7a-unknown-linux-gnueabihf")
set(_triple_linux_aarch64   "aarch64-unknown-linux-gnu")
set(_triple_macos_x86_64    "x86_64-apple-macos11")
set(_triple_macos_aarch64   "arm64-apple-macos11")
set(_triple_uefi_x86_64     "x86_64-pc-windows-gnu")
set(_triple_uefi_aarch64    "aarch64-pc-windows-gnu")
set(_triple_solaris_i386    "i386-pc-solaris2.11")
set(_triple_solaris_x86_64  "x86_64-pc-solaris2.11")
set(_triple_solaris_aarch64 "aarch64-pc-solaris2.11")
set(_triple_linux_riscv32    "riscv32-unknown-linux-gnu")
set(_triple_linux_riscv64    "riscv64-unknown-linux-gnu")
set(_triple_linux_mips64    "mips64el-unknown-linux-gnuabi64")
set(_triple_linux_mips      "mipsel-unknown-linux-gnu")
set(_triple_freebsd_i386    "i386-unknown-freebsd14.0")
set(_triple_freebsd_x86_64  "x86_64-unknown-freebsd14.0")
set(_triple_freebsd_aarch64 "aarch64-unknown-freebsd14.0")
set(_triple_freebsd_riscv64 "riscv64-unknown-freebsd14.0")

set(_triple_android_x86_64  "x86_64-linux-android")
set(_triple_android_armv7a  "armv7a-linux-androideabi")
set(_triple_android_aarch64 "aarch64-linux-android")

set(_triple_ios_aarch64     "arm64-apple-ios14.0")


# Platform extensions
set(_ext_windows ".exe")
set(_ext_linux ".elf")
set(_ext_macos "")
set(_ext_uefi ".efi")
set(_ext_solaris ".elf")
set(_ext_freebsd ".elf")
set(_ext_android ".elf")
set(_ext_ios "")


# =============================================================================
# Helper: Get Triple and Extension
# =============================================================================
macro(pir_get_target_info)
    set(PIR_TRIPLE "${_triple_${PIR_PLATFORM}_${PIR_ARCH}}")
    if(PIR_ARCH STREQUAL "mips" AND PIR_MIPS_BIGENDIAN)
        string(REPLACE "mipsel-" "mips-" PIR_TRIPLE "${PIR_TRIPLE}")
        pir_log_verbose("MIPS big-endian override: ${PIR_TRIPLE}")
    endif()
    set(PIR_EXT "${_ext_${PIR_PLATFORM}}")
    if(NOT PIR_TRIPLE)
        message(FATAL_ERROR "[pir] ${PIR_PLATFORM}/${PIR_ARCH} is not a valid combination")
    endif()
    pir_log_verbose("Triple: ${PIR_TRIPLE}")
    pir_log_debug("Extension: ${PIR_EXT}")
endmacro()

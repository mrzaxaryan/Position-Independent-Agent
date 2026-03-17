# =============================================================================
# FreeBSD.cmake - FreeBSD Platform Configuration
# =============================================================================

include_guard(GLOBAL)

# Validate: FreeBSD supports i386, x86_64, aarch64, and riscv64
if(NOT PIR_ARCH MATCHES "^(i386|x86_64|aarch64|riscv64)$")
    message(FATAL_ERROR "[pir:freebsd] Unsupported architecture '${PIR_ARCH}'. Valid: i386, x86_64, aarch64, riscv64")
endif()

pir_get_target_info()
pir_filter_sources(windows linux macos uefi solaris android ios)

list(APPEND PIR_INCLUDE_PATHS
    "${PIR_ROOT_DIR}/src/platform/kernel/freebsd")

# Architecture-specific compiler flags
if(PIR_ARCH STREQUAL "x86_64")
    # Disable the red zone for x86_64 (same rationale as Linux/macOS/Solaris)
    list(APPEND PIR_BASE_FLAGS -mno-red-zone)
    pir_log_debug_at("freebsd" "x86_64: -mno-red-zone")
endif()

# Force hidden visibility in all build types (release already gets this from
# CompilerFlags.cmake; debug builds need it explicitly). Hidden visibility
# prevents the linker from generating GOT/PLT entries for interposable symbols,
# which is essential since the PIC binary has no dynamic linker to populate them.
list(APPEND PIR_BASE_FLAGS -fvisibility=hidden)

# Disable stack protector — Clang enables -fstack-protector by default for
# FreeBSD targets (>= 13.0). The canary read (%fs:0x28 on x86_64) requires
# CRT-initialized TLS, which is unavailable in this freestanding binary.
list(APPEND PIR_BASE_FLAGS -fno-stack-protector)

# Linker configuration (ELF via LLD through clang driver)
pir_add_link_flags(
    -e,entry_point
    --no-dynamic-linker
    --symbol-ordering-file=${PIR_ROOT_DIR}/cmake/data/function.order.freebsd
    --build-id=none
    -Map,${PIR_MAP_FILE}
)

# x86 requires --pie: the x86 backend emits absolute addressing (movl
# $0xNNNNNN, %reg) instead of RIP-relative (leaq offset(%rip), %reg) for
# references to data embedded in .text unless the output is a PIE. These
# absolute addresses are resolved at the link-time VMA and break when the PIC
# binary is loaded at a different address. --pie produces a PIE (ET_DYN)
# executable, which forces position-independent code generation. AArch64 and
# RISC-V are unaffected because those ISAs always use PC-relative addressing.
#
# Release builds use -flto=full, so the LTO backend sees --pie and selects the
# PIE relocation model at link time. Debug builds do not use LTO, so the object
# files must be compiled with -fpie to generate PIC-compatible relocations;
# without it, R_X86_64_32 relocations against function symbols (from
# pic-transform's inline asm) are incompatible with --pie linking.
if(PIR_ARCH MATCHES "^(i386|x86_64)$")
    pir_add_link_flags(--pie)
    if(PIR_BUILD_TYPE STREQUAL "debug")
        list(APPEND PIR_BASE_FLAGS -fpie)
        pir_log_debug_at("freebsd" "x86 debug: -fpie + --pie for PIC-compatible objects")
    else()
        pir_log_debug_at("freebsd" "x86 release: --pie (LTO handles PIC codegen)")
    endif()
endif()

if(PIR_BUILD_TYPE STREQUAL "release")
    pir_add_link_flags(--strip-all --gc-sections)
endif()

# RISC-V 64: merge .rodata (LTO constant pools) into .text so the
# PIC binary contains the constant-pool data that auipc+ld references.
# Uses a FreeBSD-specific linker script with a non-zero base address
# (0x200000) because FreeBSD disallows mapping at VA 0 by default
# (security.bsd.map_at_zero=0).
# Disable linker relaxation: the relaxation pass can convert auipc+ld
# sequences into GP-relative loads (e.g. c.ld via gp). PIR has no CRT
# to initialise the gp register, so GP-relative accesses fault at runtime.
if(PIR_ARCH STREQUAL "riscv64")
    list(APPEND PIR_BASE_FLAGS -mno-relax)
    pir_add_link_flags(-T,${PIR_ROOT_DIR}/cmake/data/linker.freebsd.riscv64.ld --no-relax)
    pir_log_debug_at("freebsd" "riscv64: custom linker script + no-relax")
endif()

list(APPEND PIR_BASE_LINK_FLAGS -fuse-ld=lld)

# ELFOSABI patch — LLD produces ELFOSABI_NONE; FreeBSD requires
# ELFOSABI_FREEBSD (9). PostBuild.cmake patches this after linking.
set(PIR_ELF_OSABI 9)

pir_log_verbose_at("freebsd" "ELFOSABI patch: ${PIR_ELF_OSABI} (ELFOSABI_FREEBSD)")

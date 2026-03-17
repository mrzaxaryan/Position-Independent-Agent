# =============================================================================
# macOS.cmake - macOS Platform Configuration
# =============================================================================

include_guard(GLOBAL)

# Validate: macOS only supports x86_64 and aarch64
if(NOT PIR_ARCH MATCHES "^(x86_64|aarch64)$")
    message(FATAL_ERROR "[pir:macos] Unsupported architecture '${PIR_ARCH}'. Valid: x86_64, aarch64")
endif()

pir_get_target_info()
pir_filter_sources(windows linux uefi solaris freebsd android ios)

list(APPEND PIR_INCLUDE_PATHS
    "${PIR_ROOT_DIR}/src/platform/kernel/macos")

# macOS-specific compiler flags
list(APPEND PIR_BASE_FLAGS -fno-stack-protector)

# Force hidden visibility in all build types. Without hidden visibility the
# linker treats weak symbols (e.g. template explicit instantiations like
# SHABase<SHA256Traits>) as interposable and generates __TEXT,__stubs +
# __DATA,__la_symbol_ptr sections. The stubs load pointers from __DATA, which
# is not mapped by the PIC loader, causing a crash on any call through a stub.
# Hidden visibility tells both the compiler and linker that symbols cannot be
# interposed, eliminating stubs. This is especially critical on aarch64 where
# -static is not used (ARM64 macOS requires dyld).
# (Release builds already get this from CompilerFlags.cmake; debug builds need
# it explicitly here.)
list(APPEND PIR_BASE_FLAGS -fvisibility=hidden)

if(PIR_ARCH STREQUAL "x86_64")
    # Disable the red zone for x86_64. The System V ABI allows leaf functions to
    # use 128 bytes below RSP without adjusting RSP (the "red zone"). PIR is
    # designed as position-independent shellcode that executes syscalls via inline
    # asm. When injected into arbitrary contexts the red zone may not be available
    # (signal handlers, exception contexts, foreign stacks). At -O1+ with LTO the
    # optimizer may inline System::Call into what becomes a leaf function, placing
    # syscall buffers in the red zone. This flag is already used for UEFI x86_64
    # (where interrupts clobber the red zone) and is standard practice for
    # freestanding / position-independent code that makes direct syscalls.
    list(APPEND PIR_BASE_FLAGS -mno-red-zone)
    pir_log_debug_at("macos" "x86_64: -mno-red-zone, static linking")
elseif(PIR_ARCH STREQUAL "aarch64")
    list(APPEND PIR_BASE_FLAGS -mstack-probe-size=0)
    pir_log_debug_at("macos" "aarch64: -mstack-probe-size=0, dynamic (dyld required)")
endif()

# On non-macOS hosts (e.g. Linux/WSL cross-compilation), use LLD explicitly.
# LLD's ld.lld supports Mach-O output and is available in the LLVM Linux release
# tarballs. On macOS, Apple's ld is used by default — Homebrew's LLVM 22 removed
# ld64.lld so -fuse-ld=lld is invalid there.
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    list(APPEND PIR_BASE_LINK_FLAGS -fuse-ld=lld)
    pir_log_verbose_at("macos" "Cross-compiling: using LLD")
else()
    pir_log_verbose_at("macos" "Native build: using Apple ld")
endif()

# Linker configuration (Mach-O)
# Note: -platform_version is derived from the target triple (arm64-apple-macos11
# or x86_64-apple-macos11). Do NOT pass it explicitly — the triple already
# provides the min deployment target, and passing both causes a linker warning
# ("passed two min versions") and potential load command conflicts.
pir_add_link_flags(
    -e,_entry_point
    -no_compact_unwind
    -order_file,${PIR_ROOT_DIR}/cmake/data/function.order.macos
    -map,${PIR_MAP_FILE}
)

# Use -static only on x86_64. macOS ARM64 (Apple Silicon) does not support
# static executables — the kernel requires all binaries to have
# LC_LOAD_DYLINKER and go through dyld. A static binary (without dyld) is
# killed with SIGKILL (Killed: 9) immediately on execution. On x86_64 the
# kernel has legacy support for running static Mach-O binaries directly.
# Without -static on ARM64, the linker adds LC_LOAD_DYLINKER but since
# -nostdlib prevents any library linkage, the binary has zero dynamic
# dependencies — dyld starts, sees nothing to load, and jumps to _entry_point.
if(PIR_ARCH STREQUAL "x86_64")
    pir_add_link_flags(-static)
endif()

if(PIR_BUILD_TYPE STREQUAL "release")
    pir_add_link_flags(-dead_strip)
endif()

# Force entry_point.cc to compile without LTO on macOS release builds.
#
# At -Os/-Oz with -flto=full, the LTO backend emits constant-pool data
# (from __const/__literal4/__literal8/... sections renamed into __TEXT,__text)
# and unnamed helper sequences BEFORE _entry_point in the merged section.
# These unnamed regions do not appear in the linker map's "# Symbols:" table,
# so VerifyPICMode cannot catch them, yet they land at offset 0 of output.bin.
# The PIC loader always jumps to offset 0, hitting unnamed data and crashing.
#
# Apple's ld places non-LTO input sections BEFORE all LTO-generated sections in
# the final output.  Compiling entry_point.cc without LTO produces a regular
# object whose __TEXT,__text (containing only _entry_point) is inserted FIRST,
# before any LTO-generated constant pools or code, guaranteeing _entry_point
# lands at offset 0 of the extracted PIC binary at every optimization level.
#
# The -fno-lto flag is appended after -flto=full in the per-file compile
# command, so it overrides the LTO mode for this single translation unit.
# entry_point() calls start() (in the LTO object) via a PC-relative CALL/BL,
# which is inherently position-independent.
if(PIR_BUILD_TYPE STREQUAL "release")
    set_source_files_properties(
        "${PIR_ROOT_DIR}/src/entry_point.cc"
        PROPERTIES
        COMPILE_FLAGS "-fno-lto"
    )
    pir_log_debug_at("macos" "entry_point.cc: -fno-lto (entry-point ordering fix)")
endif()

# On ARM64, the linker adds dyld_stub_binder to the initial undefined symbols
# list for dynamic executables. Normally libSystem provides it, but -nostdlib
# prevents linking libSystem. Apple's system linker treats initial-undefines
# as a special category that cannot be satisfied by input objects (even with
# visibility("default")) or suppressed with -U. Use -undefined dynamic_lookup
# to tell the linker to allow all undefined symbols — they'll be dynamically
# resolved by dyld at runtime. The dyld_stub_binder symbol is never actually
# called because -fvisibility=hidden eliminates all lazy-binding stubs.
if(PIR_ARCH STREQUAL "aarch64")
    pir_add_link_flags(-undefined,dynamic_lookup)

    # Force __text to start on a 4KB page boundary within the __TEXT segment.
    #
    # ARM64 uses ADRP+ADD to compute symbol addresses. ADRP works at 4KB page
    # granularity: it adds a 21-bit signed page delta to the current PC's page.
    # The linker computes these page deltas relative to the final VMAs in the
    # Mach-O. Without this flag, __text starts at a non-page-aligned VMA
    # (e.g. 0x100000b7c — after the Mach-O headers), so the page deltas bake in
    # a 0xb7c page offset. When the PIC loader extracts __text into output.bin
    # and loads it via mmap at a page-aligned address (offset 0x0), the page
    # boundaries within the code shift, and every ADRP instruction computes the
    # wrong page — producing corrupted function pointers (SIGSEGV / SIGILL).
    #
    # By aligning __text to 0x1000 the section VMA is always page-aligned, so
    # ADRP page deltas are correct at any page-aligned load address.
    pir_add_link_flags(-sectalign,__TEXT,__text,1000)
endif()

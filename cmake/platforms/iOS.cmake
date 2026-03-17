# =============================================================================
# iOS.cmake - iOS Platform Configuration
# =============================================================================

include_guard(GLOBAL)

# Validate: iOS only supports aarch64
if(NOT PIR_ARCH STREQUAL "aarch64")
    message(FATAL_ERROR "[pir:ios] Unsupported architecture '${PIR_ARCH}'. Valid: aarch64")
endif()

pir_get_target_info()
pir_filter_sources(windows linux macos uefi solaris freebsd android)

# iOS reuses macOS XNU syscalls — re-add the macOS common include path
# that pir_filter_sources(macos) removed.
list(APPEND PIR_INCLUDE_PATHS
    "${PIR_ROOT_DIR}/src/platform/kernel/macos"
    "${PIR_ROOT_DIR}/src/platform/kernel/ios")

# iOS-specific compiler flags
list(APPEND PIR_BASE_FLAGS -fno-stack-protector)

# Force hidden visibility to eliminate lazy-binding stubs and __DATA sections.
list(APPEND PIR_BASE_FLAGS -fvisibility=hidden)

# Disable stack probing on AArch64
list(APPEND PIR_BASE_FLAGS -mstack-probe-size=0)

# On non-Darwin hosts (e.g. Linux cross-compilation), use LLD explicitly.
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    list(APPEND PIR_BASE_LINK_FLAGS -fuse-ld=lld)
    pir_log_verbose_at("ios" "Cross-compiling: using LLD")
else()
    pir_log_verbose_at("ios" "Native build: using Apple ld")
endif()

# Linker configuration (Mach-O)
pir_add_link_flags(
    -e,_entry_point
    -no_compact_unwind
    -order_file,${PIR_ROOT_DIR}/cmake/data/function.order.ios
    -map,${PIR_MAP_FILE}
)

if(PIR_BUILD_TYPE STREQUAL "release")
    pir_add_link_flags(-dead_strip)
endif()

# iOS ARM64 release builds: compile entry_point.cc without LTO to guarantee
# _entry_point lands at offset 0 of the extracted PIC binary. Same rationale
# as macOS ARM64 — Apple's ld places non-LTO input sections before LTO ones.
if(PIR_BUILD_TYPE STREQUAL "release")
    set_source_files_properties(
        "${PIR_ROOT_DIR}/src/entry_point.cc"
        PROPERTIES
        COMPILE_FLAGS "-fno-lto"
    )
    pir_log_debug_at("ios" "entry_point.cc: -fno-lto (entry-point ordering fix)")
endif()

# iOS ARM64 requires dyld (same as macOS ARM64). Use -undefined dynamic_lookup
# to satisfy the dyld_stub_binder symbol without linking libSystem.
pir_add_link_flags(-undefined,dynamic_lookup)

# Force __text to start on a 4KB page boundary for correct ADRP page deltas.
pir_add_link_flags(-sectalign,__TEXT,__text,1000)

# =============================================================================
# Windows.cmake - Windows Platform Configuration
# =============================================================================

include_guard(GLOBAL)

pir_get_target_info()
pir_filter_sources(linux macos uefi posix solaris freebsd android ios)

list(APPEND PIR_INCLUDE_PATHS
    "${PIR_ROOT_DIR}/src/platform/kernel/windows")

# Debug-specific
if(PIR_BUILD_TYPE STREQUAL "debug")
    list(APPEND PIR_BASE_FLAGS -gcodeview)
    pir_log_debug_at("windows" "Debug: -gcodeview (PDB)")

    # Remap WSL paths in PDB to Windows paths so cppvsdbg can match source files.
    # /mnt/d/Foo -> D:/Foo
    if(CMAKE_SOURCE_DIR MATCHES "^/mnt/([a-zA-Z])/")
        string(TOUPPER "${CMAKE_MATCH_1}" _drive)
        list(APPEND PIR_BASE_FLAGS
            "-fdebug-prefix-map=/mnt/${CMAKE_MATCH_1}/=${_drive}:/")
        pir_log_debug_at("windows" "WSL path remap: /mnt/${CMAKE_MATCH_1}/ -> ${_drive}:/")
    endif()
endif()

# Architecture-specific compiler flags
if(PIR_ARCH STREQUAL "x86_64")
    list(APPEND PIR_BASE_FLAGS -mno-red-zone)
elseif(PIR_ARCH MATCHES "^(armv7a|aarch64)$")
    list(APPEND PIR_BASE_FLAGS -mstack-probe-size=0)
endif()

# Linker configuration (PE/COFF)
pir_add_link_flags(
    /Entry:entry_point
    /SUBSYSTEM:CONSOLE
    /ORDER:@${PIR_ROOT_DIR}/cmake/data/function.order.windows
    /MAP:${PIR_MAP_FILE}
    /FILEALIGN:0x200
)

if(PIR_ARCH STREQUAL "i386")
    pir_add_link_flags(/BASE:0x400000 /SAFESEH:NO)
endif()

if(PIR_BUILD_TYPE STREQUAL "debug")
    pir_add_link_flags(/DEBUG)
else()
    pir_add_link_flags(--strip-all /OPT:REF /OPT:ICF /RELEASE)
endif()

list(APPEND PIR_BASE_LINK_FLAGS -fuse-ld=lld)

# =============================================================================
# UEFI.cmake - UEFI Platform Configuration
# =============================================================================

include_guard(GLOBAL)

# UEFI architecture validation
if(NOT PIR_ARCH MATCHES "^(x86_64|aarch64)$")
    message(FATAL_ERROR "[pir:uefi] Unsupported architecture '${PIR_ARCH}'. Valid: x86_64, aarch64")
endif()

pir_get_target_info()
pir_filter_sources(windows linux macos posix solaris freebsd android ios)

list(APPEND PIR_INCLUDE_PATHS
    "${PIR_ROOT_DIR}/src/platform/kernel/uefi")

# Architecture-specific compiler flags
if(PIR_ARCH STREQUAL "x86_64")
    list(APPEND PIR_BASE_FLAGS -mno-red-zone)
    pir_log_debug_at("uefi" "x86_64: -mno-red-zone")
elseif(PIR_ARCH STREQUAL "aarch64")
    list(APPEND PIR_BASE_FLAGS -mstack-probe-size=0)
    pir_log_debug_at("uefi" "aarch64: -mstack-probe-size=0")
endif()

# Linker configuration (PE/COFF)
pir_add_link_flags(
    /Entry:entry_point
    /SUBSYSTEM:EFI_APPLICATION
    /NODEFAULTLIB
    /ORDER:@${PIR_ROOT_DIR}/cmake/data/function.order.uefi
    /MAP:${PIR_MAP_FILE}
)

if(PIR_BUILD_TYPE STREQUAL "debug")
    pir_add_link_flags(/DEBUG)
else()
    pir_add_link_flags(--strip-all /OPT:REF /OPT:ICF /RELEASE)
endif()

list(APPEND PIR_BASE_LINK_FLAGS -fuse-ld=lld)

# =============================================================================
# UEFI Boot Structure
# =============================================================================
function(pir_add_uefi_boot target_name)
    set(_boot_dir "${PIR_OUTPUT_DIR}/EFI/BOOT")
    if(PIR_ARCH STREQUAL "x86_64")
        set(_boot_name "BOOTX64.EFI")
    else()
        set(_boot_name "BOOTAA64.EFI")
    endif()
    pir_log_verbose_at("uefi" "Boot image: ${_boot_dir}/${_boot_name}")

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_boot_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy "${PIR_OUTPUT_DIR}/output${PIR_EXT}" "${_boot_dir}/${_boot_name}"
        COMMENT "[pir:uefi] Creating boot image..."
        VERBATIM
    )
endfunction()

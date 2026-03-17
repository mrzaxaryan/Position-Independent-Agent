# =============================================================================
# Android.cmake - Android Platform Configuration
# =============================================================================

include_guard(GLOBAL)

pir_get_target_info()
pir_filter_sources(windows linux macos uefi solaris freebsd ios)

# Android reuses Linux kernel syscalls — re-add the Linux common include path
# that pir_filter_sources(linux) removed.
list(APPEND PIR_INCLUDE_PATHS
    "${PIR_ROOT_DIR}/src/platform/kernel/linux")

# Architecture-specific compiler flags
if(PIR_ARCH STREQUAL "armv7a")
    # The androideabi triple defaults to soft-float, generating __aeabi_d*
    # calls for all double-precision operations. All Android ARMv7-A devices
    # have VFPv3-D16 hardware at minimum. Use -mfloat-abi=softfp to compute
    # float ops in hardware (eliminating __aeabi_d* dependencies) while keeping
    # the soft-float calling convention required by the androideabi ABI.
    list(APPEND PIR_BASE_FLAGS -mfpu=vfpv3-d16 -mfloat-abi=softfp)
    pir_log_debug_at("android" "armv7a: -mfpu=vfpv3-d16 -mfloat-abi=softfp (hardware FP)")
endif()

# Force hidden visibility in all build types. Hidden visibility prevents the
# linker from generating GOT/PLT entries for interposable symbols, which is
# essential since the PIC binary has no dynamic linker to populate them.
list(APPEND PIR_BASE_FLAGS -fvisibility=hidden)

# Linker configuration (ELF)
pir_add_link_flags(
    -e,entry_point
    --no-dynamic-linker
    --symbol-ordering-file=${PIR_ROOT_DIR}/cmake/data/function.order.linux
    --build-id=none
    -Map,${PIR_MAP_FILE}
)

if(PIR_BUILD_TYPE STREQUAL "release")
    pir_add_link_flags(--strip-all --gc-sections)
endif()

list(APPEND PIR_BASE_LINK_FLAGS -fuse-ld=lld)

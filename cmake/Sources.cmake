# =============================================================================
# Sources.cmake - Source Collection and Platform Filtering
# =============================================================================

include_guard(GLOBAL)

# =============================================================================
# Source Collection
# =============================================================================
# Auto-discover include paths (all subdirs containing headers)
file(GLOB_RECURSE _all_headers CONFIGURE_DEPENDS
    "${PIR_ROOT_DIR}/src/core/*.h"
    "${PIR_ROOT_DIR}/src/platform/*.h"
    "${PIR_ROOT_DIR}/src/lib/*.h"
)
set(PIR_INCLUDE_PATHS "${PIR_ROOT_DIR}/src")
foreach(_hdr ${_all_headers})
    get_filename_component(_dir "${_hdr}" DIRECTORY)
    list(APPEND PIR_INCLUDE_PATHS "${_dir}")
endforeach()

file(GLOB PIR_SOURCES_ROOT CONFIGURE_DEPENDS
    "${PIR_ROOT_DIR}/src/*.cc"
)
file(GLOB_RECURSE PIR_SOURCES CONFIGURE_DEPENDS
    "${PIR_ROOT_DIR}/src/core/*.cc"
    "${PIR_ROOT_DIR}/src/platform/*.cc"
    "${PIR_ROOT_DIR}/src/lib/*.cc"
)
list(APPEND PIR_SOURCES ${PIR_SOURCES_ROOT})
file(GLOB_RECURSE PIR_HEADERS CONFIGURE_DEPENDS
    "${PIR_ROOT_DIR}/src/core/*.h"
    "${PIR_ROOT_DIR}/src/platform/*.h"
    "${PIR_ROOT_DIR}/src/lib/*.h"
)

# Append application layer sources (beacon or tests)
list(APPEND PIR_INCLUDE_PATHS "${PIR_APP_DIR}")
file(GLOB_RECURSE _app_sources CONFIGURE_DEPENDS "${PIR_APP_DIR}/*.cc")
file(GLOB_RECURSE _app_headers CONFIGURE_DEPENDS "${PIR_APP_DIR}/*.h")
list(LENGTH _app_sources _app_src_count)
list(LENGTH _app_headers _app_hdr_count)
pir_log_debug("App layer ${PIR_APP_DIR}: ${_app_src_count} sources, ${_app_hdr_count} headers")
list(APPEND PIR_SOURCES ${_app_sources})
list(APPEND PIR_HEADERS ${_app_headers})

list(REMOVE_DUPLICATES PIR_INCLUDE_PATHS)

# Log source collection results
list(LENGTH PIR_SOURCES _src_count)
list(LENGTH PIR_HEADERS _hdr_count)
list(LENGTH PIR_INCLUDE_PATHS _inc_count)
pir_log_verbose("Collected ${_src_count} sources, ${_hdr_count} headers, ${_inc_count} include paths")

# =============================================================================
# Helper: Filter Platform Sources
# =============================================================================
# Excludes sources and include paths for the listed platforms.
# Usage: pir_filter_sources(linux macos uefi)  -- excludes listed platforms
macro(pir_filter_sources)
    pir_log_debug("Filtering platforms: ${ARGN}")
    foreach(_exclude ${ARGN})
        list(FILTER PIR_SOURCES EXCLUDE REGEX ".*/${_exclude}/.*")
        list(FILTER PIR_INCLUDE_PATHS EXCLUDE REGEX ".*/platform/(common|memory|io|fs|network|system)/${_exclude}(/.*)?$")
    endforeach()
endmacro()

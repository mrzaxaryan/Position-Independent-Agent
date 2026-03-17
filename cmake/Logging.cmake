# =============================================================================
# Logging.cmake - Unified Build System Logging
# =============================================================================
# Provides consistent, prefixed logging across the entire build system.
# All messages use the [pir] prefix (or [pir:<component>] for subsystems).
#
# Log levels (controlled by PIR_LOG_LEVEL cache variable):
#   QUIET   - Errors and warnings only (no status messages)
#   STATUS  - Default: summary and key milestones (default)
#   VERBOSE - Detailed: flag lists, source counts, paths
#   DEBUG   - Everything: per-file details, internal state
#
# Functions:
#   pir_log_warn(msg)            - WARNING (always shown)
#   pir_log_warn_at(c msg)      - WARNING with component tag
#   pir_log(msg)                - STATUS-level message
#   pir_log_at(component msg)   - STATUS-level with component tag
#   pir_log_verbose(msg)        - VERBOSE-level message
#   pir_log_verbose_at(c msg)   - VERBOSE-level with component tag
#   pir_log_debug(msg)          - DEBUG-level message
#   pir_log_debug_at(c msg)     - DEBUG-level with component tag
#   pir_log_header(title)       - Section header
#   pir_log_kv(key value)       - Key-value with dot-leader alignment
#   pir_log_footer()            - Section footer
#   pir_log_list(label list)    - Print a list compactly

include_guard(GLOBAL)

# ─── Log level ──────────────────────────────────────────────────────────────

set(PIR_LOG_LEVEL "STATUS" CACHE STRING "Log verbosity: QUIET, STATUS, VERBOSE, DEBUG")
string(TOUPPER "${PIR_LOG_LEVEL}" _pir_log_level)

# Map level names to numeric ranks for comparison
set(_PIR_LOG_RANK_QUIET   0)
set(_PIR_LOG_RANK_STATUS  1)
set(_PIR_LOG_RANK_VERBOSE 2)
set(_PIR_LOG_RANK_DEBUG   3)

if(NOT DEFINED _PIR_LOG_RANK_${_pir_log_level})
    message(WARNING "[pir] Unknown PIR_LOG_LEVEL '${PIR_LOG_LEVEL}', defaulting to STATUS")
    set(_pir_log_level "STATUS")
endif()

set(_PIR_LOG_RANK ${_PIR_LOG_RANK_${_pir_log_level}})

# ─── Warnings (always shown, regardless of log level) ────────────────────────

function(pir_log_warn msg)
    message(WARNING "[pir] ${msg}")
endfunction()

function(pir_log_warn_at component msg)
    message(WARNING "[pir:${component}] ${msg}")
endfunction()

# ─── Status messages (shown at STATUS level and above) ──────────────────────

function(pir_log msg)
    if(_PIR_LOG_RANK GREATER_EQUAL ${_PIR_LOG_RANK_STATUS})
        message(STATUS "[pir] ${msg}")
    endif()
endfunction()

function(pir_log_at component msg)
    if(_PIR_LOG_RANK GREATER_EQUAL ${_PIR_LOG_RANK_STATUS})
        message(STATUS "[pir:${component}] ${msg}")
    endif()
endfunction()

# ─── Verbose messages (shown at VERBOSE level and above) ────────────────────

function(pir_log_verbose msg)
    if(_PIR_LOG_RANK GREATER_EQUAL ${_PIR_LOG_RANK_VERBOSE})
        message(STATUS "[pir]   ${msg}")
    endif()
endfunction()

function(pir_log_verbose_at component msg)
    if(_PIR_LOG_RANK GREATER_EQUAL ${_PIR_LOG_RANK_VERBOSE})
        message(STATUS "[pir:${component}]   ${msg}")
    endif()
endfunction()

# ─── Debug messages (shown at DEBUG level only) ─────────────────────────────

function(pir_log_debug msg)
    if(_PIR_LOG_RANK GREATER_EQUAL ${_PIR_LOG_RANK_DEBUG})
        message(STATUS "[pir] (debug) ${msg}")
    endif()
endfunction()

function(pir_log_debug_at component msg)
    if(_PIR_LOG_RANK GREATER_EQUAL ${_PIR_LOG_RANK_DEBUG})
        message(STATUS "[pir:${component}] (debug) ${msg}")
    endif()
endfunction()

# ─── Build summary helpers ──────────────────────────────────────────────────

function(pir_log_header title)
    if(_PIR_LOG_RANK GREATER_EQUAL ${_PIR_LOG_RANK_STATUS})
        message(STATUS "")
        message(STATUS "[pir] ──────────────────────────────────────────")
        message(STATUS "[pir]  ${title}")
        message(STATUS "[pir] ──────────────────────────────────────────")
    endif()
endfunction()

function(pir_log_kv key value)
    if(_PIR_LOG_RANK GREATER_EQUAL ${_PIR_LOG_RANK_STATUS})
        if("${value}" STREQUAL "")
            set(value "(none)")
        endif()
        set(_col 18)
        string(LENGTH "${key}" _klen)
        math(EXPR _pad "${_col} - ${_klen}")
        if(_pad LESS 2)
            set(_pad 2)
        endif()
        string(REPEAT "." ${_pad} _dots)
        message(STATUS "[pir]   ${key} ${_dots} ${value}")
    endif()
endfunction()

function(pir_log_footer)
    if(_PIR_LOG_RANK GREATER_EQUAL ${_PIR_LOG_RANK_STATUS})
        message(STATUS "[pir] ──────────────────────────────────────────")
        message(STATUS "")
    endif()
endfunction()

# ─── List printer (verbose level) ──────────────────────────────────────────

function(pir_log_list label items)
    if(_PIR_LOG_RANK GREATER_EQUAL ${_PIR_LOG_RANK_VERBOSE})
        list(LENGTH items _count)
        message(STATUS "[pir]   ${label} (${_count}):")
        foreach(_item IN LISTS items)
            message(STATUS "[pir]     - ${_item}")
        endforeach()
    endif()
endfunction()

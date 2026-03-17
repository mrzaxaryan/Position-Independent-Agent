# =============================================================================
# Base64Encode.cmake - Cross-platform base64 encoding (run via cmake -P)
# =============================================================================
# Usage: cmake -DPIC_FILE=<input> -DBASE64_FILE=<output> -P Base64Encode.cmake
# =============================================================================

cmake_minimum_required(VERSION 3.20)

foreach(_var PIC_FILE BASE64_FILE)
    if(NOT DEFINED ${_var})
        message(FATAL_ERROR "[pir:base64] ${_var} is required")
    endif()
endforeach()

if(NOT EXISTS "${PIC_FILE}")
    message(FATAL_ERROR "[pir:base64] Input file not found: ${PIC_FILE}")
endif()

if(WIN32)
    execute_process(
        COMMAND certutil -encodehex -f "${PIC_FILE}" "${BASE64_FILE}" 0x40000001
        RESULT_VARIABLE _result
    )
else()
    # GNU base64 uses -w, BSD base64 does not
    execute_process(
        COMMAND base64 -w 0 "${PIC_FILE}"
        OUTPUT_FILE "${BASE64_FILE}"
        ERROR_QUIET
        RESULT_VARIABLE _result
    )
    if(NOT _result EQUAL 0)
        execute_process(
            COMMAND base64 -i "${PIC_FILE}"
            OUTPUT_VARIABLE _b64
            RESULT_VARIABLE _result
        )
        string(REPLACE "\n" "" _b64 "${_b64}")
        file(WRITE "${BASE64_FILE}" "${_b64}")
    endif()
    file(APPEND "${BASE64_FILE}" "\n")
endif()

if(NOT _result EQUAL 0)
    message(WARNING "[pir:base64] Encoding failed with code: ${_result}")
endif()

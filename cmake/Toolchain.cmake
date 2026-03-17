# =============================================================================
# Toolchain.cmake - Freestanding Toolchain Setup (must precede project())
# =============================================================================

include_guard(GLOBAL)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Compiler detection
if(DEFINED ENV{CC})
    set(CMAKE_C_COMPILER "$ENV{CC}" CACHE STRING "" FORCE)
else()
    set(CMAKE_C_COMPILER clang CACHE STRING "" FORCE)
endif()
if(DEFINED ENV{CXX})
    set(CMAKE_CXX_COMPILER "$ENV{CXX}" CACHE STRING "" FORCE)
else()
    set(CMAKE_CXX_COMPILER clang++ CACHE STRING "" FORCE)
endif()

# Skip compiler tests (freestanding environment)
foreach(_lang C CXX)
    set(CMAKE_${_lang}_COMPILER_FORCED TRUE)
    set(CMAKE_${_lang}_COMPILER_WORKS TRUE)
    set(CMAKE_${_lang}_STANDARD_LIBRARIES "")
endforeach()

set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <LINK_FLAGS> <OBJECTS> -o <TARGET>")

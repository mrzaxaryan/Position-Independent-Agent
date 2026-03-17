# =============================================================================
# VerifyPICMode.cmake - Verify PIC requirements (run via cmake -P)
# =============================================================================
# Usage: cmake -DMAP_FILE=<path> -P VerifyPICMode.cmake
# =============================================================================

cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED MAP_FILE)
    message(FATAL_ERROR "[pir:verify] MAP_FILE is required")
endif()

if(NOT EXISTS "${MAP_FILE}")
    message(STATUS "[pir:verify] Map file not found (expected for debug builds): ${MAP_FILE}")
    return()
endif()

file(READ "${MAP_FILE}" _content)

# =============================================================================
# Verify entry_point is first function in .text section
# =============================================================================
set(entry_point_first FALSE)

# Windows/UEFI (PE format): Check .text$entry_point is at offset 0
# Format: "0001:00000000 <size>H .text$entry_point"
if(_content MATCHES "0001:00000000[ \t]+[0-9a-fA-F]+H[ \t]+\\.text\\$entry_point")
    set(entry_point_first TRUE)
endif()

# Linux (ELF format): Check entry_point is first symbol in .text section
# Format: "VMA LMA Size Align Out In Symbol" with entry_point immediately after .text section
if(_content MATCHES "\\.text\n[^\n]+\\(\\.text\\.entry_point\\)")
    set(entry_point_first TRUE)
endif()

# macOS (Mach-O format): Check _entry_point is the first symbol listed
# ld64.lld map format: "# Symbols:" header, then "# Address..." header line, then symbols
# The first actual symbol should be _entry_point
if(_content MATCHES "# Symbols:\n#[^\n]*\n[^\n]*_entry_point")
    set(entry_point_first TRUE)
endif()

if(NOT entry_point_first)
    message(FATAL_ERROR
        "[pir:verify] entry_point is not the first function in .text!\n\n"
        "The entry point must be at the beginning of the code section.\n"
        "Check that cmake/function.order contains 'entry_point' and is being used.\n\n"
        "Map file: ${MAP_FILE}"
    )
endif()

# macOS only: verify the first named symbol's VMA equals the __TEXT,__text
# section start VMA.  The named-symbol check above confirms _entry_point is
# the first *named* symbol, but with -flto=full at -Os/-Oz the LTO backend can
# place unnamed constant-pool data (from __const/__literal* sections renamed
# into __text) BEFORE _entry_point without creating any map entry for it.
# If such unnamed data occupies the first bytes of __text, the PIC loader
# jumps to data instead of code and crashes (SIGSEGV) even though the
# named-symbol order looks correct.
#
# The address comparison catches this: if anything unnamed precedes
# _entry_point, the first symbol's VMA will be greater than the section start.
if(_content MATCHES "# Symbols:")
    # Extract __TEXT,__text section start VMA from the "# Sections:" table.
    string(REGEX MATCH
        "(0x[0-9a-fA-F]+)[ \t]+0x[0-9a-fA-F]+[ \t]+__TEXT[ \t]+__text"
        _m "${_content}")
    set(_text_start "${CMAKE_MATCH_1}")

    # Extract the VMA of the first entry in the "# Symbols:" table.
    # The format is: "# Symbols:\n# Address\t...\n0x<addr>\t..."
    string(REGEX MATCH
        "# Symbols:[ \t]*\n#[^\n]*\n(0x[0-9a-fA-F]+)"
        _m "${_content}")
    set(_first_sym_addr "${CMAKE_MATCH_1}")

    if(_text_start AND _first_sym_addr AND NOT _text_start STREQUAL _first_sym_addr)
        message(FATAL_ERROR
            "[pir:verify] _entry_point is not at offset 0 of __TEXT,__text!\n\n"
            "__text section start: ${_text_start}\n"
            "First symbol address: ${_first_sym_addr}\n\n"
            "Unnamed constant-pool data or an LTO-generated helper was placed\n"
            "before _entry_point in the merged __text section. Ensure that\n"
            "entry_point.cc is compiled without LTO (-fno-lto) so ld64.lld\n"
            "places the non-LTO object's __text before all LTO-generated content.\n\n"
            "Map file: ${MAP_FILE}"
        )
    endif()
endif()

# =============================================================================
# Verify no forbidden data sections
# =============================================================================
set(_sections rdata rodata data bss got plt)
set(_found)

foreach(_sec ${_sections})
    # Windows/UEFI (PE format): "0001:offset sizeH .section"
    if(_content MATCHES "[0-9a-fA-F]+:[0-9a-fA-F]+[ \t]+[0-9a-fA-F]+H[ \t]+\\.${_sec}[ \t\n]")
        list(APPEND _found ".${_sec}")
    # Linux (ELF format): "VMA LMA Size Align .section"
    # The size field requires a non-zero first hex digit ([1-9a-fA-F]) to skip
    # empty sections. lld creates a zero-size .got section header for x86_64
    # ELF targets even when no GOT entries exist; this is harmless and must not
    # trigger a false positive.
    elseif(_content MATCHES "[ \t]+[0-9a-fA-F]+[ \t]+[0-9a-fA-F]+[ \t]+[1-9a-fA-F][0-9a-fA-F]*[ \t]+[0-9]+[ \t]+\\.${_sec}[ \t\n]")
        list(APPEND _found ".${_sec}")
    endif()
endforeach()

# macOS (Mach-O format): Check for __DATA segment sections
# ld64.lld map uses tab-separated "Segment\tSection" format
if(_content MATCHES "__DATA[ \t]+__data")
    list(APPEND _found "__DATA,__data")
endif()
if(_content MATCHES "__DATA[ \t]+__bss")
    list(APPEND _found "__DATA,__bss")
endif()
if(_content MATCHES "__DATA_CONST[ \t]+__const")
    list(APPEND _found "__DATA_CONST,__const")
endif()
# GOT section: The linker synthesizes __DATA_CONST,__got when the compiler
# emits GOT-relative relocations (@GOTPCREL on x86_64, ADRP+LDR via GOT page
# on aarch64). This section cannot be merged into __TEXT,__text via
# -rename_section. The -fdirect-access-external-data compiler flag prevents
# GOT generation; this check is a safety net to catch regressions.
if(_content MATCHES "__DATA_CONST[ \t]+__got")
    list(APPEND _found "__DATA_CONST,__got")
endif()
# Lazy symbol pointers: without hidden visibility, the linker may generate
# __TEXT,__stubs + __DATA,__la_symbol_ptr for weak/interposable symbols (e.g.
# template explicit instantiations). Stubs load pointers from __DATA which is
# not mapped by the PIC loader. The -fvisibility=hidden compiler flag prevents
# this by marking all symbols as non-interposable.
if(_content MATCHES "__DATA[ \t]+__la_symbol_ptr")
    list(APPEND _found "__DATA,__la_symbol_ptr")
endif()

# macOS: Catch ANY __TEXT subsection that is NOT __text.
# Only __TEXT,__text is extracted into output.bin by the PIC loader. Any other
# __TEXT subsection (constant pools, literal pools, string constants, stubs,
# unwind info, etc.) would be missing from the PIC binary, causing a crash on
# any PC-relative reference from code to data in that section.
# The -rename_section linker flags in macOS.cmake merge known constant sections
# into __text. This check is a comprehensive safety net that catches:
#   - Known sections where the rename flag was accidentally removed
#   - New/unexpected sections the compiler or LTO optimizer may create
# The regex matches any line with "__TEXT" followed by a section name that is
# NOT "__text" (the only allowed __TEXT subsection).
string(REGEX MATCHALL "__TEXT[ \t]+__[a-z0-9_]+" _text_sections "${_content}")
foreach(_entry ${_text_sections})
    if(NOT _entry MATCHES "__TEXT[ \t]+__text$")
        # Extract the section name for the error message
        string(REGEX MATCH "__TEXT[ \t]+(__[a-z0-9_]+)" _match "${_entry}")
        list(APPEND _found "__TEXT,${CMAKE_MATCH_1}")
    endif()
endforeach()

if(_found)
    list(JOIN _found ", " _list)
    message(FATAL_ERROR
        "[pir:verify] Forbidden data sections break position-independence!\n"
        "Found: ${_list}\n\n"
        "Common causes:\n"
        "  - Static/global variables (use stack-local variables)\n"
        "  - String literals (use auto str = \"...\" on the stack)\n"
        "  - Floating-point constants (pic-transform should handle these automatically)\n"
        "  - Array literals (use stack-local arrays)\n\n"
        "Map file: ${MAP_FILE}"
    )
endif()

message(STATUS "[pir:verify] Position-independent verification passed")

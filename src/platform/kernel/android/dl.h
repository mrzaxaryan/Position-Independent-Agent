/**
 * @file dl.h
 * @brief Position-independent dynamic symbol resolution for Android
 *
 * @details Locates loaded shared libraries by parsing /proc/self/maps and
 * resolves exported symbols by parsing their ELF dynamic symbol tables.
 * This is the Android equivalent of:
 *   - Windows: PEB walking + PE export parsing (peb.cc, pe.cc)
 *   - macOS: dyld Mach-O parsing + dlopen/dlsym resolution (dyld.cc)
 *
 * Used to find libart.so in memory and resolve JNI_GetCreatedJavaVMs,
 * enabling the PIC agent to attach to the ART Java VM.
 *
 * No libc dependency — uses direct syscalls to read /proc/self/maps
 * and parses ELF structures in-memory.
 *
 * @ingroup kernel_android
 */

#pragma once

#include "core/types/primitives.h"

/**
 * @brief Find a loaded shared library's base address by name substring
 *
 * @details Parses /proc/self/maps to find the first executable mapping
 * whose path contains the given substring (e.g. "libart.so").
 *
 * @param nameSubstr Substring to match in the mapped file path
 * @return Base address of the library, or nullptr if not found
 */
PVOID FindLoadedLibrary(const CHAR *nameSubstr);

/**
 * @brief Resolve an exported symbol from a loaded ELF shared library
 *
 * @details Parses the ELF header at the given base address, locates the
 * dynamic symbol table (.dynsym) and string table (.dynstr) via PT_DYNAMIC
 * program header, then searches for the named symbol.
 *
 * Uses SysV hash table (DT_HASH) for symbol count when available,
 * otherwise falls back to linear scan with a bounded limit.
 *
 * @param base Base address of the loaded ELF (ELF header)
 * @param symbolName Exact name of the symbol to resolve
 * @return Symbol address on success, nullptr if not found or parse error
 */
PVOID ResolveElfSymbol(PVOID base, const CHAR *symbolName);

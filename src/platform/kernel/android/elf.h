/**
 * @file elf.h
 * @brief ELF structures for parsing loaded shared libraries on Android
 *
 * @details Minimal ELF64/ELF32 definitions needed to parse loaded .so files
 * in /proc/self/maps and resolve dynamic symbol exports. This is the Android
 * equivalent of pe.h (Windows PE export parsing) and dyld.h (macOS Mach-O).
 *
 * Used to locate libart.so in memory and resolve JNI_GetCreatedJavaVMs,
 * enabling the PIC agent to attach to the ART Java VM and call framework
 * APIs (e.g. MediaProjection for screen capture) via JNI.
 *
 * @ingroup kernel_android
 */

#pragma once

#include "core/types/primitives.h"

// =============================================================================
// ELF identification
// =============================================================================

constexpr UINT8 ELF_MAGIC_0 = 0x7F;
constexpr UINT8 ELF_MAGIC_1 = 'E';
constexpr UINT8 ELF_MAGIC_2 = 'L';
constexpr UINT8 ELF_MAGIC_3 = 'F';

constexpr UINT8 ELFCLASS32 = 1;
constexpr UINT8 ELFCLASS64 = 2;

// =============================================================================
// ELF program header types
// =============================================================================

constexpr UINT32 PT_NULL    = 0;
constexpr UINT32 PT_LOAD    = 1;
constexpr UINT32 PT_DYNAMIC = 2;

// =============================================================================
// ELF dynamic tags
// =============================================================================

constexpr USIZE DT_NULL     = 0;
constexpr USIZE DT_STRTAB   = 5;   ///< Address of dynamic string table
constexpr USIZE DT_SYMTAB   = 6;   ///< Address of dynamic symbol table
constexpr USIZE DT_STRSZ    = 10;  ///< Size of string table in bytes
constexpr USIZE DT_SYMENT   = 11;  ///< Size of one symbol table entry
constexpr USIZE DT_HASH     = 4;   ///< Address of ELF hash table (SysV)
constexpr USIZE DT_GNU_HASH = 0x6FFFFEF5; ///< Address of GNU hash table

// =============================================================================
// ELF symbol binding/type
// =============================================================================

constexpr UINT8 STB_GLOBAL = 1;
constexpr UINT8 STB_WEAK   = 2;
constexpr UINT16 SHN_UNDEF = 0;

// =============================================================================
// ELF64 structures
// =============================================================================

#pragma pack(push, 1)

/// @brief ELF64 file header (at base of loaded .so)
struct Elf64Ehdr
{
	UINT8  Ident[16];  ///< Magic + class + endian + version + OS/ABI + padding
	UINT16 Type;       ///< Object file type (ET_DYN = 3 for .so)
	UINT16 Machine;    ///< Architecture
	UINT32 Version;    ///< ELF version
	UINT64 Entry;      ///< Entry point virtual address
	UINT64 PhOff;      ///< Program header table offset
	UINT64 ShOff;      ///< Section header table offset
	UINT32 Flags;      ///< Processor-specific flags
	UINT16 EhSize;     ///< ELF header size
	UINT16 PhEntSize;  ///< Program header entry size
	UINT16 PhNum;      ///< Number of program headers
	UINT16 ShEntSize;  ///< Section header entry size
	UINT16 ShNum;      ///< Number of section headers
	UINT16 ShStrNdx;   ///< Section name string table index
};

/// @brief ELF64 program header
struct Elf64Phdr
{
	UINT32 Type;    ///< Segment type (PT_LOAD, PT_DYNAMIC, etc.)
	UINT32 Flags;   ///< Segment flags (PF_R, PF_W, PF_X)
	UINT64 Offset;  ///< Offset in file
	UINT64 VAddr;   ///< Virtual address in memory
	UINT64 PAddr;   ///< Physical address (unused)
	UINT64 FileSz;  ///< Size in file
	UINT64 MemSz;   ///< Size in memory
	UINT64 Align;   ///< Alignment
};

/// @brief ELF64 dynamic section entry
struct Elf64Dyn
{
	INT64  Tag;   ///< Dynamic tag (DT_STRTAB, DT_SYMTAB, etc.)
	UINT64 Val;   ///< Value (address or integer depending on tag)
};

/// @brief ELF64 symbol table entry
struct Elf64Sym
{
	UINT32 Name;   ///< Offset into string table
	UINT8  Info;   ///< Symbol type and binding (STB_GLOBAL, etc.)
	UINT8  Other;  ///< Symbol visibility
	UINT16 ShNdx;  ///< Section header index (SHN_UNDEF = undefined)
	UINT64 Value;  ///< Symbol value (virtual address)
	UINT64 Size;   ///< Symbol size
};

/// @brief ELF SysV hash table header
struct ElfHashTable
{
	UINT32 NBucket; ///< Number of hash buckets
	UINT32 NChain;  ///< Number of chain entries (= total symbol count)
	// Followed by: UINT32 bucket[NBucket], UINT32 chain[NChain]
};

/// @brief ELF GNU hash table header
struct ElfGnuHashTable
{
	UINT32 NBuckets;   ///< Number of hash buckets
	UINT32 SymOffset;  ///< Index of first symbol in .dynsym accessible via hash
	UINT32 BloomSize;  ///< Number of Bloom filter words
	UINT32 BloomShift; ///< Bloom filter shift count
	// Followed by: USIZE bloom[BloomSize], UINT32 buckets[NBuckets], UINT32 chains[]
};

#pragma pack(pop)

// =============================================================================
// ELF32 structures (for armv7a)
// =============================================================================

#pragma pack(push, 1)

/// @brief ELF32 file header
struct Elf32Ehdr
{
	UINT8  Ident[16];
	UINT16 Type;
	UINT16 Machine;
	UINT32 Version;
	UINT32 Entry;
	UINT32 PhOff;
	UINT32 ShOff;
	UINT32 Flags;
	UINT16 EhSize;
	UINT16 PhEntSize;
	UINT16 PhNum;
	UINT16 ShEntSize;
	UINT16 ShNum;
	UINT16 ShStrNdx;
};

/// @brief ELF32 program header
struct Elf32Phdr
{
	UINT32 Type;
	UINT32 Offset;
	UINT32 VAddr;
	UINT32 PAddr;
	UINT32 FileSz;
	UINT32 MemSz;
	UINT32 Flags;
	UINT32 Align;
};

/// @brief ELF32 dynamic section entry
struct Elf32Dyn
{
	INT32  Tag;
	UINT32 Val;
};

/// @brief ELF32 symbol table entry
struct Elf32Sym
{
	UINT32 Name;
	UINT32 Value;
	UINT32 Size;
	UINT8  Info;
	UINT8  Other;
	UINT16 ShNdx;
};

#pragma pack(pop)

// =============================================================================
// Architecture-independent type aliases
// =============================================================================

#if defined(ARCHITECTURE_AARCH64) || defined(ARCHITECTURE_X86_64) || \
    defined(ARCHITECTURE_RISCV64) || defined(ARCHITECTURE_MIPS64)
using ElfEhdr = Elf64Ehdr;
using ElfPhdr = Elf64Phdr;
using ElfDyn  = Elf64Dyn;
using ElfSym  = Elf64Sym;
#elif defined(ARCHITECTURE_ARMV7A) || defined(ARCHITECTURE_I386) || \
      defined(ARCHITECTURE_RISCV32)
using ElfEhdr = Elf32Ehdr;
using ElfPhdr = Elf32Phdr;
using ElfDyn  = Elf32Dyn;
using ElfSym  = Elf32Sym;
#endif

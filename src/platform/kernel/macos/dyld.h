/**
 * @file dyld.h
 * @brief Dynamic library loader for macOS via dyld Mach-O parsing
 *
 * @details Provides position-independent access to macOS framework functions
 * (e.g. CoreGraphics) by locating dyld's base address via Mach IPC
 * (task_info with TASK_DYLD_INFO), parsing dyld's Mach-O symbol table
 * to resolve dlopen/dlsym, and then using those to load frameworks.
 *
 * This is the macOS equivalent of the Windows PEB-based module resolution
 * in peb.cc — it enables calling framework functions without linking
 * against libSystem or any shared library.
 *
 * @note Only available on macOS (not iOS — iOS requires UIKit/ObjC runtime
 * for most display operations and has stricter sandboxing).
 *
 * @see Apple Mach-O Reference
 *      https://developer.apple.com/library/archive/documentation/DeveloperTools/Conceptual/MachORuntime/
 */
#pragma once

#include "core/types/primitives.h"

// =============================================================================
// Mach-O header types (from <mach-o/loader.h>)
// =============================================================================

/// @brief Mach-O 64-bit magic number
constexpr UINT32 MH_MAGIC_64 = 0xFEEDFACF;

/// @brief Load command: 64-bit segment
constexpr UINT32 LC_SEGMENT_64 = 0x19;

/// @brief Load command: symbol table
constexpr UINT32 LC_SYMTAB = 0x02;

/// @brief 64-bit Mach-O file header
struct MachHeader64
{
	UINT32 Magic;      ///< MH_MAGIC_64 (0xFEEDFACF)
	UINT32 CpuType;    ///< CPU type identifier
	UINT32 CpuSubtype; ///< CPU subtype identifier
	UINT32 FileType;   ///< Type of file (MH_EXECUTE, MH_DYLINKER, etc.)
	UINT32 NCmds;      ///< Number of load commands
	UINT32 SizeOfCmds; ///< Total size of all load commands in bytes
	UINT32 Flags;      ///< Flags
	UINT32 Reserved;   ///< Reserved (64-bit padding)
};

/// @brief Generic load command header
struct LoadCommand
{
	UINT32 Cmd;     ///< Load command type (LC_SEGMENT_64, LC_SYMTAB, etc.)
	UINT32 CmdSize; ///< Total size of this load command in bytes
};

/// @brief 64-bit segment load command
struct SegmentCommand64
{
	UINT32 Cmd;        ///< LC_SEGMENT_64
	UINT32 CmdSize;    ///< sizeof(SegmentCommand64) + section headers
	CHAR SegName[16];  ///< Segment name (e.g. "__TEXT", "__LINKEDIT")
	UINT64 VmAddr;     ///< Virtual address of this segment
	UINT64 VmSize;     ///< Size in virtual memory
	UINT64 FileOff;    ///< Offset in the file
	UINT64 FileSize;   ///< Size in the file
	INT32 MaxProt;     ///< Maximum VM protection
	INT32 InitProt;    ///< Initial VM protection
	UINT32 NSects;     ///< Number of sections
	UINT32 Flags;      ///< Segment flags
};

/// @brief Symbol table load command
struct SymtabCommand
{
	UINT32 Cmd;     ///< LC_SYMTAB
	UINT32 CmdSize; ///< sizeof(SymtabCommand) = 24
	UINT32 SymOff;  ///< File offset to symbol table (array of Nlist64)
	UINT32 NSyms;   ///< Number of symbols
	UINT32 StrOff;  ///< File offset to string table
	UINT32 StrSize; ///< Size of string table in bytes
};

/// @brief 64-bit symbol table entry
struct Nlist64
{
	UINT32 StrIndex; ///< Index into string table
	UINT8 Type;      ///< Symbol type and flags
	UINT8 Sect;      ///< Section ordinal (1-based, 0 = NO_SECT)
	UINT16 Desc;     ///< Symbol descriptor
	UINT64 Value;    ///< Symbol value (address)
};

// =============================================================================
// dyld structures (from <mach-o/dyld_images.h>)
// =============================================================================

/// @brief Per-image info in the dyld image list
struct DyldImageInfo
{
	const MachHeader64 *ImageLoadAddress; ///< Mach-O header address in memory
	const CHAR *ImageFilePath;            ///< Full path to the image file
	USIZE ImageFileModDate;               ///< File modification date
};

/// @brief Master dyld info structure (version >= 2 required for dyldImageLoadAddress)
struct DyldAllImageInfos
{
	UINT32 Version;                         ///< Structure version (>= 2 required)
	UINT32 InfoArrayCount;                  ///< Number of entries in InfoArray
	const DyldImageInfo *InfoArray;         ///< Array of loaded image info
	PVOID Notification;                     ///< Notification callback function
	UINT8 ProcessDetachedFromSharedRegion;  ///< Process left shared region
	UINT8 LibSystemInitialized;             ///< libSystem is initialized
	UINT8 Padding[6];                       ///< Padding to align next pointer
	const MachHeader64 *DyldImageLoadAddress; ///< Mach-O header of dyld itself
};

// =============================================================================
// Framework resolver API
// =============================================================================

/// @brief Resolve a function from a macOS framework by loading it via dyld
///
/// @details Locates dyld in the process address space via Mach IPC (task_info),
/// parses dyld's Mach-O symbol table to find dlopen/dlsym, then uses those
/// to load the specified framework and resolve the named function.
///
/// Results are cached — repeated calls for the same framework reuse the
/// previously loaded handle.
///
/// @param frameworkPath Full path to the framework binary
///        (e.g. "/System/Library/Frameworks/CoreGraphics.framework/CoreGraphics")
/// @param functionName Name of the function to resolve (e.g. "CGMainDisplayID")
/// @return Function pointer on success, nullptr on failure
PVOID ResolveFrameworkFunction(const CHAR *frameworkPath, const CHAR *functionName);

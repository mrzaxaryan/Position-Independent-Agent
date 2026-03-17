/**
 * @file pe.h
 * @brief Windows Portable Executable (PE) Format Structures
 *
 * @details Defines the structures that describe the Windows Portable Executable
 * (PE) file format, including the DOS header, COFF file header, optional header,
 * NT headers, and export directory. These structures are used for runtime parsing
 * of loaded PE images to resolve exported function addresses from DLLs (e.g.,
 * kernel32.dll, ntdll.dll) without relying on the Windows loader or import tables.
 *
 * Both 32-bit (PE32) and 64-bit (PE32+) optional header variants are provided,
 * with IMAGE_NT_HEADERS aliased to the correct variant based on the target
 * architecture.
 *
 * @see Microsoft PE Format Specification
 *      https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
 */

#pragma once

#include "core/types/primitives.h"

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

/**
 * @brief Describes the export directory of a PE image.
 *
 * @details Located via the first entry in the data directory array
 * (IMAGE_DIRECTORY_ENTRY_EXPORT). Contains RVAs to the export address table,
 * name pointer table, and ordinal table, enabling runtime resolution of
 * exported function addresses by name or ordinal.
 *
 * @see Microsoft PE Format -- Export Directory Table
 *      https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#export-directory-table
 */
typedef struct _IMAGE_EXPORT_DIRECTORY
{
	UINT32 Characteristics;       ///< Reserved, must be 0
	UINT32 TimeDateStamp;         ///< Time and date the export data was created
	UINT16 MajorVersion;          ///< Major version number (user-defined)
	UINT16 MinorVersion;          ///< Minor version number (user-defined)
	UINT32 Name;                  ///< RVA to the ASCII name of the DLL
	UINT32 Base;                  ///< Starting ordinal number for exports
	UINT32 NumberOfFunctions;     ///< Number of entries in the export address table
	UINT32 NumberOfNames;         ///< Number of entries in the name pointer table
	UINT32 AddressOfFunctions;    ///< RVA to the export address table
	UINT32 AddressOfNames;        ///< RVA to the export name pointer table
	UINT32 AddressOfNameOrdinals; ///< RVA to the ordinal table
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;

/**
 * @brief COFF file header describing the target machine and section layout.
 *
 * @details Immediately follows the PE signature in the NT headers. Identifies
 * the target architecture, number of sections, and size of the optional header
 * that follows.
 *
 * @see Microsoft PE Format -- COFF File Header
 *      https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#coff-file-header-object-and-image
 */
typedef struct _IMAGE_FILE_HEADER
{
	UINT16 Machine;              ///< Target architecture (e.g., 0x8664 for x86_64, 0xAA64 for ARM64)
	UINT16 NumberOfSections;     ///< Number of section table entries
	UINT32 TimeDateStamp;        ///< Time and date the file was created (seconds since epoch)
	UINT32 PointerToSymbolTable; ///< File offset to the COFF symbol table (0 for images)
	UINT32 NumberOfSymbols;      ///< Number of symbol table entries (0 for images)
	UINT16 SizeOfOptionalHeader; ///< Size of the optional header in bytes
	UINT16 Characteristics;      ///< Flags indicating attributes of the file (e.g., executable, DLL)
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

/**
 * @brief Describes a single data directory entry in the optional header.
 *
 * @details Each entry provides the RVA and size of a specific data structure
 * within the PE image (e.g., export table, import table, resource table).
 * The optional header contains an array of IMAGE_NUMBEROF_DIRECTORY_ENTRIES
 * (16) of these entries.
 *
 * @see Microsoft PE Format -- Optional Header Data Directories
 *      https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#optional-header-data-directories-image-only
 */
typedef struct _IMAGE_DATA_DIRECTORY
{
	UINT32 VirtualAddress; ///< RVA of the data structure
	UINT32 Size;           ///< Size of the data structure in bytes
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

/**
 * @brief PE32+ optional header for 64-bit executable images.
 *
 * @details Contains the linker version, entry point address, image base,
 * section alignment, and the data directory array. The Magic field is 0x20B
 * for PE32+ images. This header is required for executable images and provides
 * information needed by the loader.
 *
 * @see Microsoft PE Format -- Optional Header (PE32+)
 *      https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#optional-header-image-only
 */
typedef struct _IMAGE_OPTIONAL_HEADER64
{
	UINT16 Magic;                        ///< PE format magic number (0x20B for PE32+)
	UINT8 MajorLinkerVersion;            ///< Major version of the linker that produced the image
	UINT8 MinorLinkerVersion;            ///< Minor version of the linker that produced the image
	UINT32 SizeOfCode;                   ///< Total size of all code sections in bytes
	UINT32 SizeOfInitializedData;        ///< Total size of all initialized data sections in bytes
	UINT32 SizeOfUninitializedData;      ///< Total size of all uninitialized (BSS) data sections in bytes
	UINT32 AddressOfEntryPoint;          ///< RVA of the entry point function
	UINT32 BaseOfCode;                   ///< RVA of the beginning of the code section
	UINT64 ImageBase;                    ///< Preferred base address of the image when loaded
	UINT32 SectionAlignment;             ///< Alignment of sections when loaded into memory (bytes)
	UINT32 FileAlignment;               ///< Alignment of raw data of sections in the file (bytes)
	UINT16 MajorOperatingSystemVersion;  ///< Major version of the required operating system
	UINT16 MinorOperatingSystemVersion;  ///< Minor version of the required operating system
	UINT16 MajorImageVersion;            ///< Major version number of the image
	UINT16 MinorImageVersion;            ///< Minor version number of the image
	UINT16 MajorSubsystemVersion;        ///< Major version of the required subsystem
	UINT16 MinorSubsystemVersion;        ///< Minor version of the required subsystem
	UINT32 Win32VersionValue;            ///< Reserved, must be 0
	UINT32 SizeOfImage;                  ///< Total size of the image in memory (bytes), aligned to SectionAlignment
	UINT32 SizeOfHeaders;               ///< Combined size of all headers, aligned to FileAlignment
	UINT32 CheckSum;                     ///< Image file checksum
	UINT16 Subsystem;                    ///< Subsystem required to run the image (e.g., GUI, console)
	UINT16 DllCharacteristics;           ///< DLL characteristics flags (e.g., ASLR, DEP, CFG)
	UINT64 SizeOfStackReserve;           ///< Size of stack to reserve (bytes)
	UINT64 SizeOfStackCommit;            ///< Size of stack to commit initially (bytes)
	UINT64 SizeOfHeapReserve;            ///< Size of local heap to reserve (bytes)
	UINT64 SizeOfHeapCommit;             ///< Size of local heap to commit initially (bytes)
	UINT32 LoaderFlags;                  ///< Reserved, must be 0
	UINT32 NumberOfRvaAndSizes;          ///< Number of valid entries in the DataDirectory array
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES]; ///< Array of data directory entries
} IMAGE_OPTIONAL_HEADER64, *PIMAGE_OPTIONAL_HEADER64;

/**
 * @brief PE32 optional header for 32-bit executable images.
 *
 * @details Contains the linker version, entry point address, image base,
 * section alignment, and the data directory array. The Magic field is 0x10B
 * for PE32 images. Differs from the 64-bit variant by including BaseOfData
 * and using 32-bit fields for ImageBase and stack/heap sizes.
 *
 * @see Microsoft PE Format -- Optional Header (PE32)
 *      https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#optional-header-image-only
 */
typedef struct _IMAGE_OPTIONAL_HEADER32
{
	UINT16 Magic;                        ///< PE format magic number (0x10B for PE32)
	UINT8 MajorLinkerVersion;            ///< Major version of the linker that produced the image
	UINT8 MinorLinkerVersion;            ///< Minor version of the linker that produced the image
	UINT32 SizeOfCode;                   ///< Total size of all code sections in bytes
	UINT32 SizeOfInitializedData;        ///< Total size of all initialized data sections in bytes
	UINT32 SizeOfUninitializedData;      ///< Total size of all uninitialized (BSS) data sections in bytes
	UINT32 AddressOfEntryPoint;          ///< RVA of the entry point function
	UINT32 BaseOfCode;                   ///< RVA of the beginning of the code section
	UINT32 BaseOfData;                   ///< RVA of the beginning of the data section (PE32 only)
	UINT32 ImageBase;                    ///< Preferred base address of the image when loaded
	UINT32 SectionAlignment;             ///< Alignment of sections when loaded into memory (bytes)
	UINT32 FileAlignment;               ///< Alignment of raw data of sections in the file (bytes)
	UINT16 MajorOperatingSystemVersion;  ///< Major version of the required operating system
	UINT16 MinorOperatingSystemVersion;  ///< Minor version of the required operating system
	UINT16 MajorImageVersion;            ///< Major version number of the image
	UINT16 MinorImageVersion;            ///< Minor version number of the image
	UINT16 MajorSubsystemVersion;        ///< Major version of the required subsystem
	UINT16 MinorSubsystemVersion;        ///< Minor version of the required subsystem
	UINT32 Win32VersionValue;            ///< Reserved, must be 0
	UINT32 SizeOfImage;                  ///< Total size of the image in memory (bytes), aligned to SectionAlignment
	UINT32 SizeOfHeaders;               ///< Combined size of all headers, aligned to FileAlignment
	UINT32 CheckSum;                     ///< Image file checksum
	UINT16 Subsystem;                    ///< Subsystem required to run the image (e.g., GUI, console)
	UINT16 DllCharacteristics;           ///< DLL characteristics flags (e.g., ASLR, DEP, CFG)
	UINT32 SizeOfStackReserve;           ///< Size of stack to reserve (bytes)
	UINT32 SizeOfStackCommit;            ///< Size of stack to commit initially (bytes)
	UINT32 SizeOfHeapReserve;            ///< Size of local heap to reserve (bytes)
	UINT32 SizeOfHeapCommit;             ///< Size of local heap to commit initially (bytes)
	UINT32 LoaderFlags;                  ///< Reserved, must be 0
	UINT32 NumberOfRvaAndSizes;          ///< Number of valid entries in the DataDirectory array
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES]; ///< Array of data directory entries
} IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;

/**
 * @brief NT headers for a 64-bit (PE32+) executable image.
 *
 * @details The top-level PE header structure for 64-bit images, located at the
 * file offset specified by IMAGE_DOS_HEADER::e_lfanew. Contains the PE
 * signature ("PE\0\0"), COFF file header, and PE32+ optional header.
 *
 * @see Microsoft PE Format -- PE Signature and Headers
 *      https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#signature-image-only
 */
typedef struct _IMAGE_NT_HEADERS64
{
	UINT32 Signature;                   ///< PE signature, must be IMAGE_NT_SIGNATURE (0x00004550)
	IMAGE_FILE_HEADER FileHeader;       ///< COFF file header
	IMAGE_OPTIONAL_HEADER64 OptionalHeader; ///< PE32+ optional header
} IMAGE_NT_HEADERS64, *PIMAGE_NT_HEADERS64;

/**
 * @brief NT headers for a 32-bit (PE32) executable image.
 *
 * @details The top-level PE header structure for 32-bit images, located at the
 * file offset specified by IMAGE_DOS_HEADER::e_lfanew. Contains the PE
 * signature ("PE\0\0"), COFF file header, and PE32 optional header.
 *
 * @see Microsoft PE Format -- PE Signature and Headers
 *      https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#signature-image-only
 */
typedef struct _IMAGE_NT_HEADERS32
{
	UINT32 Signature;                   ///< PE signature, must be IMAGE_NT_SIGNATURE (0x00004550)
	IMAGE_FILE_HEADER FileHeader;       ///< COFF file header
	IMAGE_OPTIONAL_HEADER32 OptionalHeader; ///< PE32 optional header
} IMAGE_NT_HEADERS32, *PIMAGE_NT_HEADERS32;

/// @brief Architecture-appropriate IMAGE_NT_HEADERS alias.
/// @details Aliases IMAGE_NT_HEADERS64 on 64-bit platforms (x86_64, ARM64) and
/// IMAGE_NT_HEADERS32 on 32-bit platforms (i386, ARMV7A).
#if defined(PLATFORM_WINDOWS_X86_64) || defined(PLATFORM_WINDOWS_AARCH64)

typedef IMAGE_NT_HEADERS64 IMAGE_NT_HEADERS;
typedef PIMAGE_NT_HEADERS64 PIMAGE_NT_HEADERS;

#else

typedef IMAGE_NT_HEADERS32 IMAGE_NT_HEADERS;
typedef PIMAGE_NT_HEADERS32 PIMAGE_NT_HEADERS;

#endif

/**
 * @brief MS-DOS executable header at the beginning of every PE file.
 *
 * @details The legacy DOS header is preserved at offset 0 of every PE image for
 * backward compatibility. The only fields relevant for PE parsing are e_magic
 * (which must be IMAGE_DOS_SIGNATURE, "MZ") and e_lfanew (which provides the
 * file offset to the IMAGE_NT_HEADERS structure).
 *
 * @see Microsoft PE Format -- MS-DOS Stub
 *      https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#ms-dos-stub-image-only
 */
typedef struct _IMAGE_DOS_HEADER
{
	UINT16 e_magic;      ///< Magic number, must be IMAGE_DOS_SIGNATURE (0x5A4D, "MZ")
	UINT16 e_cblp;       ///< Bytes on the last page of the file
	UINT16 e_cp;         ///< Number of pages in the file
	UINT16 e_crlc;       ///< Number of relocations
	UINT16 e_cparhdr;    ///< Size of header in paragraphs
	UINT16 e_minalloc;   ///< Minimum extra paragraphs needed
	UINT16 e_maxalloc;   ///< Maximum extra paragraphs needed
	UINT16 e_ss;         ///< Initial (relative) SS value
	UINT16 e_sp;         ///< Initial SP value
	UINT16 e_csum;       ///< Checksum
	UINT16 e_ip;         ///< Initial IP value
	UINT16 e_cs;         ///< Initial (relative) CS value
	UINT16 e_lfarlc;     ///< File offset to the relocation table
	UINT16 e_ovno;       ///< Overlay number
	UINT16 e_res[4];     ///< Reserved words
	UINT16 e_oemid;      ///< OEM identifier
	UINT16 e_oeminfo;    ///< OEM-specific information
	UINT16 e_res2[10];   ///< Reserved words
	INT32 e_lfanew;      ///< File offset to the IMAGE_NT_HEADERS structure
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

#define IMAGE_DOS_SIGNATURE 0x5A4D     ///< "MZ" DOS executable signature
#define IMAGE_NT_SIGNATURE 0x00004550  ///< "PE\0\0" NT signature
#define IMAGE_DIRECTORY_ENTRY_EXPORT 0 ///< Index of the export directory in the data directory array

/**
 * @brief Resolves an exported function address from a loaded PE module by name hash.
 *
 * @details Walks the export directory of the specified PE module to find the
 * exported function whose name matches the given DJB2 hash. Parses the DOS
 * header, NT headers, and export directory at runtime to locate the export
 * address table, name pointer table, and ordinal table, then performs a
 * hash comparison against each exported name.
 *
 * This is the core mechanism for position-independent function resolution on
 * Windows, eliminating the need for import tables or the Windows loader.
 *
 * @param hModule Base address of the loaded PE module (e.g., kernel32.dll base from PEB).
 * @param functionNameHash DJB2 hash of the exported function name to resolve.
 *
 * @return Pointer to the resolved function, or nullptr if the export was not found.
 *
 * @see Microsoft PE Format -- Export Directory Table
 *      https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#export-directory-table
 */
PVOID GetExportAddress(PVOID hModule, UINT64 functionNameHash);

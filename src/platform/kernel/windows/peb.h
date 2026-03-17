/**
 * @file peb.h
 * @brief Process Environment Block (PEB) Structures and Module Resolution
 *
 * @details Provides position-independent access to the Windows Process
 * Environment Block (PEB) and its associated loader data structures. The PEB
 * is a user-mode structure maintained by the NT loader that contains process
 * startup information, loaded module lists, and heap/environment pointers.
 *
 * This header defines the minimal PEB structure subset needed for dynamic
 * module and export resolution without relying on Win32 API imports. Module
 * lookup is performed by walking the PEB's InLoadOrderModuleList and matching
 * DJB2 hashes of module names, eliminating the need for static import tables.
 *
 * The PEB pointer is obtained directly from the Thread Environment Block (TEB)
 * via architecture-specific register reads (GS:[0x60] on x86_64, FS:[0x30] on
 * i386, TEB->ProcessEnvironmentBlock on ARM64).
 *
 * @see PEB structure
 *      https://learn.microsoft.com/en-us/windows/win32/api/winternl/ns-winternl-peb
 * @see PEB_LDR_DATA structure
 *      https://learn.microsoft.com/en-us/windows/win32/api/winternl/ns-winternl-peb_ldr_data
 */

#pragma once

#if defined(PLATFORM_WINDOWS)

#include "platform/kernel/windows/windows_types.h"

/**
 * @brief Retrieves a pointer to a structure from a pointer to one of its fields.
 *
 * @details Given a pointer to a field within a structure, computes the base
 * address of the containing structure by subtracting the field's offset.
 * Used extensively when walking LIST_ENTRY chains in PEB loader data.
 *
 * @param address Pointer to the field within the structure.
 * @param type The type of the containing structure.
 * @param field The name of the field within the structure.
 *
 * @see CONTAINING_RECORD macro
 *      https://learn.microsoft.com/en-us/windows/win32/api/ntdef/nf-ntdef-containing_record
 */
#define CONTAINING_RECORD(address, type, field) ((type *)((PCHAR)(address) - (USIZE)(&((type *)0)->field)))

/**
 * @brief Doubly-linked list entry used throughout the NT kernel and loader.
 *
 * @details Forms the basis of circular doubly-linked lists used by the NT
 * loader to track loaded modules. Each LIST_ENTRY points to the next and
 * previous entries in the list. The head entry's Flink/Blink point to the
 * first/last data entries respectively; an empty list points to itself.
 *
 * @see LIST_ENTRY structure
 *      https://learn.microsoft.com/en-us/windows/win32/api/ntdef/ns-ntdef-list_entry
 */
typedef struct _LIST_ENTRY
{
	struct _LIST_ENTRY *Flink; ///< Pointer to the next entry in the list
	struct _LIST_ENTRY *Blink; ///< Pointer to the previous entry in the list
} LIST_ENTRY, *PLIST_ENTRY;

/**
 * @brief Loader data table entry describing a single loaded module.
 *
 * @details Each loaded DLL or executable in the process is represented by an
 * LDR_DATA_TABLE_ENTRY. These entries are linked into three parallel circular
 * lists (load order, memory order, initialization order) rooted in the
 * PEB_LDR_DATA structure. Used by PEB-walking code to locate module base
 * addresses and export tables without Win32 API calls.
 *
 * @see LDR_DATA_TABLE_ENTRY structure
 *      https://learn.microsoft.com/en-us/windows/win32/api/winternl/ns-winternl-peb_ldr_data
 */
typedef struct _LDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY InLoadOrderModuleList;           ///< Links to previous/next module in load order
	LIST_ENTRY InMemoryOrderModuleList;         ///< Links to previous/next module in memory order
	LIST_ENTRY InInitializationOrderModuleList; ///< Links to previous/next module in initialization order
	PVOID DllBase;                              ///< Base address where the module is loaded in memory
	PVOID EntryPoint;                           ///< Address of the module's entry point (DllMain)
	UINT32 SizeOfImage;                         ///< Size of the module image in bytes
	UNICODE_STRING FullDllName;                 ///< Full path of the module (e.g., "C:\Windows\System32\ntdll.dll")
	UNICODE_STRING BaseDllName;                 ///< Base name of the module (e.g., "ntdll.dll")
	UINT32 Flags;                               ///< Loader flags (LDRP_* constants)
	INT16 LoadCount;                            ///< Reference count for the module
	INT16 TlsIndex;                             ///< Thread Local Storage index, or -1 if none
	LIST_ENTRY HashTableEntry;                  ///< Entry in the loader's hash table for fast lookup
	UINT32 TimeDateStamp;                       ///< PE timestamp from the module's file header
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

/**
 * @brief Contains the heads of the three module lists maintained by the loader.
 *
 * @details The PEB_LDR_DATA structure is pointed to by PEB.LoaderData and
 * contains the head entries for the three circular doubly-linked lists of
 * LDR_DATA_TABLE_ENTRY structures. Walking these lists provides access to
 * all loaded modules in the process.
 *
 * @see PEB_LDR_DATA structure
 *      https://learn.microsoft.com/en-us/windows/win32/api/winternl/ns-winternl-peb_ldr_data
 */
typedef struct _PEB_LDR_DATA
{
	UINT32 Length;                                  ///< Size of this structure in bytes
	UINT32 Initialized;                             ///< TRUE after the loader has finished initialization
	PVOID SsHandle;                                 ///< Handle to the loader's heap (internal use)
	LIST_ENTRY InLoadOrderModuleList;               ///< Head of the load-order module list
	LIST_ENTRY InMemoryOrderModuleList;             ///< Head of the memory-order module list
	LIST_ENTRY InInitializationOrderModuleList;     ///< Head of the initialization-order module list
} PEB_LDR_DATA, *PPEB_LDR_DATA;

/**
 * @brief Contains process startup parameters set by the parent process.
 *
 * @details The RTL_USER_PROCESS_PARAMETERS structure is created by
 * RtlCreateProcessParametersEx and stored in PEB.ProcessParameters. It
 * contains the command line, environment, standard I/O handles, and other
 * startup information for the process.
 *
 * @note This is a minimal subset of the full structure, containing only the
 * fields needed by this runtime.
 *
 * @see RTL_USER_PROCESS_PARAMETERS structure
 *      https://learn.microsoft.com/en-us/windows/win32/api/winternl/ns-winternl-rtl_user_process_parameters
 */
typedef struct _RTL_USER_PROCESS_PARAMETERS
{
	UINT32 MaximumLength;  ///< Maximum size of this structure in bytes
	UINT32 Length;         ///< Actual size of this structure in bytes
	UINT32 Flags;          ///< Parameter flags (e.g., RTL_USER_PROC_PARAMS_NORMALIZED)
	UINT32 DebugFlags;     ///< Debug-related flags
	PVOID ConsoleHandle;   ///< Handle to the process's console window
	UINT32 ConsoleFlags;   ///< Console creation flags
	PVOID StandardInput;   ///< Handle to the standard input device
	PVOID StandardOutput;  ///< Handle to the standard output device
	PVOID StandardError;   ///< Handle to the standard error device
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

/**
 * @brief The Process Environment Block, the top-level per-process user-mode structure.
 *
 * @details The PEB is allocated by the kernel during process creation and is
 * accessible from user mode via the Thread Environment Block (TEB). It
 * contains pointers to the loader data (loaded module lists), process
 * parameters (command line, environment, standard handles), the process
 * heap, and the image base address.
 *
 * @note This is a minimal subset of the full PEB structure, containing only
 * the fields needed by this runtime.
 *
 * @see PEB structure
 *      https://learn.microsoft.com/en-us/windows/win32/api/winternl/ns-winternl-peb
 */
typedef struct _PEB
{
	UINT8 InheritedAddressSpace;                    ///< TRUE if the address space was inherited from the parent
	UINT8 ReadImageFileExecOptions;                 ///< TRUE if image file execution options should be read
	UINT8 BeingDebugged;                            ///< TRUE if the process is being debugged
	UINT8 Spare;                                    ///< Reserved (BitField on newer Windows versions)
	PVOID Mutant;                                   ///< Handle to a mutant object (typically -1)
	PVOID ImageBase;                                ///< Base address of the process's executable image
	PPEB_LDR_DATA LoaderData;                       ///< Pointer to PEB_LDR_DATA containing loaded module lists
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters; ///< Pointer to process startup parameters
	PVOID SubSystemData;                            ///< Subsystem-specific data (e.g., Win32 subsystem)
	PVOID ProcessHeap;                              ///< Handle to the default process heap
} PEB, *PPEB;

/**
 * @brief Retrieves a pointer to the current process's PEB.
 *
 * @details Reads the PEB pointer directly from the Thread Environment Block
 * (TEB) using architecture-specific mechanisms:
 * - x86_64: reads GS:[0x60]
 * - i386: reads FS:[0x30]
 * - ARM64: reads the TEB pointer from the dedicated register and offsets
 *   to the ProcessEnvironmentBlock field
 *
 * @return Pointer to the current process's PEB structure.
 *
 * @see PEB structure
 *      https://learn.microsoft.com/en-us/windows/win32/api/winternl/ns-winternl-peb
 */
PPEB GetCurrentPEB(VOID);

/**
 * @brief Resolves a loaded module's base address by DJB2 hash of its name.
 *
 * @details Walks the PEB's InMemoryOrderModuleList comparing each module's
 * BaseDllName (converted to lowercase) against the provided DJB2 hash.
 * Returns the DllBase of the first matching module, or nullptr if no
 * module matches.
 *
 * @param moduleNameHash DJB2 hash of the target module name (case-insensitive).
 *
 * @return Base address (HMODULE) of the matching module, or nullptr if not found.
 *
 * @see GetModuleHandleW (documented Win32 equivalent)
 *      https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulehandlew
 */
PVOID GetModuleHandleFromPEB(UINT64 moduleNameHash);

/**
 * @brief Resolves an exported function address from a module identified by DJB2 hash.
 *
 * @details First locates the module via GetModuleHandleFromPEB, then parses
 * the module's PE export directory to find the function whose name matches
 * the provided DJB2 hash. Supports both named exports and forwarded exports.
 *
 * @param moduleNameHash DJB2 hash of the target module name (case-insensitive).
 * @param functionNameHash DJB2 hash of the target function name (case-sensitive).
 *
 * @return Address of the exported function, or nullptr if not found.
 *
 * @see GetProcAddress (documented Win32 equivalent)
 *      https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getprocaddress
 */
PVOID ResolveExportAddressFromPebModule(UINT64 moduleNameHash, UINT64 functionNameHash);

/**
 * @brief Resolves an exported function, loading the module via LdrLoadDll if needed.
 *
 * @details First attempts to find the module in the PEB (fast path). If the
 * module is not loaded, falls back to NTDLL::LdrLoadDll to load it, then
 * resolves the export. The module name hash is computed internally from
 * the provided wide string name.
 *
 * @param moduleName Wide string name of the module (e.g., L"user32.dll").
 * @param functionNameHash DJB2 hash of the target function name (case-sensitive).
 *
 * @return Address of the exported function, or nullptr if not found.
 */
PVOID ResolveExportAddress(const WCHAR *moduleName, UINT64 functionNameHash);

#else
#error Unsupported platform
#endif
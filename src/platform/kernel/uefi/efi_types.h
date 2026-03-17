/**
 * @file efi_types.h
 * @brief Core UEFI type definitions and calling conventions.
 *
 * @details Defines the fundamental UEFI data types (EFI_STATUS, EFI_HANDLE, EFI_GUID, CHAR16),
 *          memory type enumerations, status code macros, task priority levels, and the
 *          EFI_TABLE_HEADER common to all UEFI system tables. The EFIAPI calling convention
 *          macro is set per-architecture: Microsoft x64 ABI on x86_64, standard AAPCS64 on AArch64.
 *
 * @see UEFI Specification 2.10 — Section 2.3, Calling Conventions
 * @see UEFI Specification 2.10 — Section 2.3.1, Data Types
 * @see UEFI Specification 2.10 — Section 7.2, Memory Allocation Services
 * @see UEFI Specification 2.10 — Appendix D, Status Codes
 */

#pragma once

#include "core/types/primitives.h"

// =============================================================================
// UEFI Calling Convention
// =============================================================================

#if defined(ARCHITECTURE_X86_64)
// x86_64 UEFI uses Microsoft x64 ABI
#define EFIAPI __attribute__((ms_abi))
#elif defined(ARCHITECTURE_AARCH64)
// AArch64 UEFI uses standard AAPCS64
#define EFIAPI
#else
#error "Unsupported architecture for UEFI"
#endif

// =============================================================================
// Fundamental UEFI Types
// =============================================================================

typedef USIZE EFI_STATUS;
typedef PVOID EFI_HANDLE;
typedef PVOID EFI_EVENT;
typedef UINT64 EFI_PHYSICAL_ADDRESS;
typedef UINT64 EFI_VIRTUAL_ADDRESS;
typedef USIZE EFI_TPL;

// CHAR16 is defined in primitives.h as a fixed-width 16-bit type (unsigned short).
// On UEFI targets, WCHAR is also 2 bytes, so CHAR16 and WCHAR are interchangeable.

// =============================================================================
// GUID Structure
// =============================================================================

typedef struct
{
	UINT32 Data1;
	UINT16 Data2;
	UINT16 Data3;
	UINT8 Data4[8];
} EFI_GUID;

// =============================================================================
// Memory Types
// =============================================================================

typedef enum
{
	EfiReservedMemoryType,
	EfiLoaderCode,
	EfiLoaderData,
	EfiBootServicesCode,
	EfiBootServicesData,
	EfiRuntimeServicesCode,
	EfiRuntimeServicesData,
	EfiConventionalMemory,
	EfiUnusableMemory,
	EfiACPIReclaimMemory,
	EfiACPIMemoryNVS,
	EfiMemoryMappedIO,
	EfiMemoryMappedIOPortSpace,
	EfiPalCode,
	EfiPersistentMemory,
	EfiMaxMemoryType
} EFI_MEMORY_TYPE;

typedef enum
{
	AllocateAnyPages,
	AllocateMaxAddress,
	AllocateAddress,
	MaxAllocateType
} EFI_ALLOCATE_TYPE;

// =============================================================================
// Status Codes
// =============================================================================

#define EFI_SUCCESS ((EFI_STATUS)0)

// Error codes (high bit set — architecture-width aware)
#define EFI_ERROR_MASK ((EFI_STATUS)1 << (sizeof(EFI_STATUS) * 8 - 1))

#define EFI_ERROR(code) (EFI_ERROR_MASK | (code))

#define EFI_LOAD_ERROR EFI_ERROR(1)
#define EFI_INVALID_PARAMETER EFI_ERROR(2)
#define EFI_UNSUPPORTED EFI_ERROR(3)
#define EFI_BAD_BUFFER_SIZE EFI_ERROR(4)
#define EFI_BUFFER_TOO_SMALL EFI_ERROR(5)
#define EFI_NOT_READY EFI_ERROR(6)
#define EFI_DEVICE_ERROR EFI_ERROR(7)
#define EFI_WRITE_PROTECTED EFI_ERROR(8)
#define EFI_OUT_OF_RESOURCES EFI_ERROR(9)
#define EFI_NOT_FOUND EFI_ERROR(14)
#define EFI_ACCESS_DENIED EFI_ERROR(15)
#define EFI_TIMEOUT EFI_ERROR(18)
#define EFI_ALREADY_STARTED EFI_ERROR(20)
#define EFI_ABORTED EFI_ERROR(21)

// Warning codes (high bit clear, but non-zero)
#define EFI_WARN_UNKNOWN_GLYPH 1
#define EFI_WARN_DELETE_FAILURE 2

// =============================================================================
// Task Priority Levels
// =============================================================================

#define TPL_APPLICATION 4
#define TPL_CALLBACK 8
#define TPL_NOTIFY 16
#define TPL_HIGH_LEVEL 31

// =============================================================================
// Helper Macros
// =============================================================================

#define EFI_ERROR_CHECK(status) (((SSIZE)(status)) < 0)

// Table header structure (common to all UEFI tables)
typedef struct
{
	UINT64 Signature;
	UINT32 Revision;
	UINT32 HeaderSize;
	UINT32 CRC32;
	UINT32 Reserved;
} EFI_TABLE_HEADER;

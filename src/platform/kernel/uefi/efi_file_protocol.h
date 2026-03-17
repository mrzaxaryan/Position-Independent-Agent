/**
 * @file efi_file_protocol.h
 * @brief EFI File Protocol and Simple File System Protocol definitions.
 *
 * @details Defines EFI_FILE_PROTOCOL for file I/O operations (open, close, read, write,
 *          get/set position, get/set info, flush) and EFI_SIMPLE_FILE_SYSTEM_PROTOCOL for
 *          opening the root directory of a file system volume. Includes EFI_FILE_INFO for
 *          querying file metadata, file attribute constants, and open mode flags.
 *
 * @see UEFI Specification 2.10 — Section 13.4, Simple File System Protocol
 * @see UEFI Specification 2.10 — Section 13.5, EFI File Protocol
 */

#pragma once

#include "platform/kernel/uefi/efi_types.h"
#include "platform/kernel/uefi/efi_runtime_services.h"

// =============================================================================
// Forward Declarations
// =============================================================================

struct EFI_FILE_PROTOCOL;
struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

// =============================================================================
// File Information Structure
// =============================================================================

typedef struct
{
	UINT64 Size;           // Size of the EFI_FILE_INFO structure including FileName
	UINT64 FileSize;       // Size of the file in bytes
	UINT64 PhysicalSize;   // Physical space consumed on storage media
	EFI_TIME CreateTime;   // Time file was created
	EFI_TIME LastAccessTime; // Time file was last accessed
	EFI_TIME ModificationTime; // Time file was last modified
	UINT64 Attribute;      // File attributes
	CHAR16 FileName[1];    // Null-terminated name (variable length)
} EFI_FILE_INFO;

// File attributes
#define EFI_FILE_READ_ONLY   0x0000000000000001ULL
#define EFI_FILE_HIDDEN      0x0000000000000002ULL
#define EFI_FILE_SYSTEM      0x0000000000000004ULL
#define EFI_FILE_RESERVED    0x0000000000000008ULL
#define EFI_FILE_DIRECTORY   0x0000000000000010ULL
#define EFI_FILE_ARCHIVE     0x0000000000000020ULL
#define EFI_FILE_VALID_ATTR  0x0000000000000037ULL

// =============================================================================
// File Protocol Function Types
// =============================================================================

// Open modes
#define EFI_FILE_MODE_READ   0x0000000000000001ULL
#define EFI_FILE_MODE_WRITE  0x0000000000000002ULL
#define EFI_FILE_MODE_CREATE 0x8000000000000000ULL

typedef EFI_STATUS (EFIAPI *EFI_FILE_OPEN)(
	struct EFI_FILE_PROTOCOL *This,
	struct EFI_FILE_PROTOCOL **NewHandle,
	CHAR16 *FileName,
	UINT64 OpenMode,
	UINT64 Attributes);

typedef EFI_STATUS (EFIAPI *EFI_FILE_CLOSE)(
	struct EFI_FILE_PROTOCOL *This);

typedef EFI_STATUS (EFIAPI *EFI_FILE_DELETE)(
	struct EFI_FILE_PROTOCOL *This);

typedef EFI_STATUS (EFIAPI *EFI_FILE_READ)(
	struct EFI_FILE_PROTOCOL *This,
	USIZE *BufferSize,
	PVOID Buffer);

typedef EFI_STATUS (EFIAPI *EFI_FILE_WRITE)(
	struct EFI_FILE_PROTOCOL *This,
	USIZE *BufferSize,
	PVOID Buffer);

typedef EFI_STATUS (EFIAPI *EFI_FILE_GET_POSITION)(
	struct EFI_FILE_PROTOCOL *This,
	UINT64 *Position);

typedef EFI_STATUS (EFIAPI *EFI_FILE_SET_POSITION)(
	struct EFI_FILE_PROTOCOL *This,
	UINT64 Position);

typedef EFI_STATUS (EFIAPI *EFI_FILE_GET_INFO)(
	struct EFI_FILE_PROTOCOL *This,
	EFI_GUID *InformationType,
	USIZE *BufferSize,
	PVOID Buffer);

typedef EFI_STATUS (EFIAPI *EFI_FILE_SET_INFO)(
	struct EFI_FILE_PROTOCOL *This,
	EFI_GUID *InformationType,
	USIZE BufferSize,
	PVOID Buffer);

typedef EFI_STATUS (EFIAPI *EFI_FILE_FLUSH)(
	struct EFI_FILE_PROTOCOL *This);

// =============================================================================
// EFI File Protocol Structure
// =============================================================================

#define EFI_FILE_PROTOCOL_REVISION  0x00010000
#define EFI_FILE_PROTOCOL_REVISION2 0x00020000

typedef struct EFI_FILE_PROTOCOL {
	UINT64 Revision;
	EFI_FILE_OPEN Open;
	EFI_FILE_CLOSE Close;
	EFI_FILE_DELETE Delete;
	EFI_FILE_READ Read;
	EFI_FILE_WRITE Write;
	EFI_FILE_GET_POSITION GetPosition;
	EFI_FILE_SET_POSITION SetPosition;
	EFI_FILE_GET_INFO GetInfo;
	EFI_FILE_SET_INFO SetInfo;
	EFI_FILE_FLUSH Flush;
	// EFI_FILE_PROTOCOL_REVISION2 additions
	PVOID OpenEx;   // EFI_FILE_OPEN_EX
	PVOID ReadEx;   // EFI_FILE_READ_EX
	PVOID WriteEx;  // EFI_FILE_WRITE_EX
	PVOID FlushEx;  // EFI_FILE_FLUSH_EX
} EFI_FILE_PROTOCOL;

// =============================================================================
// Simple File System Protocol Function Types
// =============================================================================

typedef EFI_STATUS (EFIAPI *EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME)(
	struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
	struct EFI_FILE_PROTOCOL **Root);

// =============================================================================
// EFI Simple File System Protocol Structure
// =============================================================================

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION 0x00010000

typedef struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
	UINT64 Revision;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME OpenVolume;
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

// =============================================================================
// Search Type for LocateHandle
// =============================================================================

#ifndef ByProtocol
#define ByProtocol 2
#endif

// =============================================================================
// Event Types for CreateEvent
// =============================================================================

#ifndef EVT_NOTIFY_SIGNAL
#define EVT_NOTIFY_SIGNAL 0x00000200
#endif

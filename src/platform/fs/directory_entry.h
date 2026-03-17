/**
 * @file directory_entry.h
 * @brief Directory entry structure for filesystem iteration
 * @details Defines the DirectoryEntry struct used by DirectoryIterator to
 * represent files and directories. Packed layout with no padding.
 * Part of the PLATFORM layer of the Position-Independent Runtime (PIR).
 */

#pragma once

#include "core/types/primitives.h"

#pragma pack(push, 1)
/// Directory entry structure returned by DirectoryIterator.
struct DirectoryEntry
{
	WCHAR Name[256];         ///< File or directory name (null-terminated)
	UINT64 CreationTime;     ///< Creation timestamp in platform filetime format
	UINT64 LastModifiedTime; ///< Last modification timestamp in platform filetime format
	UINT64 Size;             ///< File size in bytes
	UINT32 Type;             ///< Drive type when IsDrive is set (2=Removable, 3=Fixed, etc.)
	BOOL IsDirectory;        ///< TRUE if the entry is a directory
	BOOL IsDrive;            ///< TRUE if the entry represents a drive root (e.g., C:\)
	BOOL IsHidden;           ///< TRUE if the file has the hidden attribute
	BOOL IsSystem;           ///< TRUE if the file has the system attribute
	BOOL IsReadOnly;         ///< TRUE if the file has the read-only attribute
};

#pragma pack(pop)

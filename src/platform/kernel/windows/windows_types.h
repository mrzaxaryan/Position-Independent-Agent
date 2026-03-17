/**
 * @file windows_types.h
 * @brief Windows NT Fundamental Type Definitions and Constants
 *
 * @details Defines the core Windows NT types, structures, and constants needed
 * by the position-independent runtime. These definitions replace the standard
 * Windows SDK headers (winnt.h, winternl.h, ntdef.h) to avoid any dependency
 * on the Windows SDK or CRT.
 *
 * Includes NT object management types (OBJECT_ATTRIBUTES, UNICODE_STRING),
 * I/O types (IO_STATUS_BLOCK, LARGE_INTEGER), file system constants
 * (FILE_*, GENERIC_*, MEM_*), and the InitializeObjectAttributes helper macro.
 *
 * @see Windows Data Types
 *      https://learn.microsoft.com/en-us/windows/win32/winprog/windows-data-types
 * @see OBJECT_ATTRIBUTES structure
 *      https://learn.microsoft.com/en-us/windows/win32/api/ntdef/ns-ntdef-_object_attributes
 */

#pragma once

#include "core/types/primitives.h"

/** @name File Create Disposition Constants
 *  @brief Specifies the action to take on files that exist or do not exist.
 *  @see ZwCreateFile — CreateDisposition parameter
 *       https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwcreatefile
 *  @{
 */
#define FILE_OPEN 0x00000001
#define FILE_CREATE 0x00000002
#define FILE_OPEN_IF 0x00000003
#define FILE_OVERWRITE 0x00000004
#define FILE_OVERWRITE_IF 0x00000005
/** @} */

/** @name File Create Options Constants
 *  @brief Flags controlling file object behavior during creation.
 *  @see ZwCreateFile — CreateOptions parameter
 *       https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwcreatefile
 *  @{
 */
#define FILE_DIRECTORY_FILE 0x00000001
#define FILE_SYNCHRONOUS_IO_NONALERT 0x00000020
#define FILE_NON_DIRECTORY_FILE 0x00000040
#define FILE_DELETE_ON_CLOSE 0x00001000
/** @} */

/** @name Access Rights Constants
 *  @brief Standard and generic access rights for kernel objects.
 *  @see Access Rights
 *       https://learn.microsoft.com/en-us/windows/win32/secauthz/access-rights-and-access-masks
 *  @{
 */
#define SYNCHRONIZE (0x00100000L)
#define DELETE (0x00010000L)
#define FILE_READ_ATTRIBUTES (0x0080)
#define FILE_LIST_DIRECTORY 0x00000001
#define FILE_APPEND_DATA 4
#define GENERIC_READ (0x80000000L)
#define GENERIC_WRITE (0x40000000L)
/** @} */

/** @name File Attribute Constants
 *  @brief Bitmask values for file and directory attributes.
 *  @see File Attribute Constants
 *       https://learn.microsoft.com/en-us/windows/win32/fileio/file-attribute-constants
 *  @{
 */
#define FILE_ATTRIBUTE_READONLY  0x00000001
#define FILE_ATTRIBUTE_HIDDEN    0x00000002
#define FILE_ATTRIBUTE_SYSTEM    0x00000004
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define FILE_ATTRIBUTE_NORMAL 0x00000080
/** @} */

/** @name File Sharing Constants
 *  @brief Flags controlling concurrent access to a file object.
 *  @see ZwCreateFile — ShareAccess parameter
 *       https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwcreatefile
 *  @{
 */
#define FILE_SHARE_READ 0x00000001
#define FILE_SHARE_WRITE 0x00000002
#define FILE_SHARE_DELETE 0x00000004
/** @} */

/** @name File Flag Constants
 *  @brief Flags controlling file I/O behavior.
 *  @{
 */
#define FILE_FLAG_OVERLAPPED 0x40000000
#define FILE_FLAG_WRITE_THROUGH 0x80000000
#define FILE_WRITE_THROUGH 0x80000000
/** @} */

/** @name Device Type Constants
 *  @brief Identifies the type of file system device.
 *  @see FILE_FS_DEVICE_INFORMATION
 *       https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ns-wdm-_file_fs_device_information
 *  @{
 */
#define FILE_DEVICE_CD_ROM_FILE_SYSTEM 0x00000003
#define FILE_DEVICE_DISK_FILE_SYSTEM 0x00000008
#define FILE_DEVICE_NETWORK_FILE_SYSTEM 0x00000014
#define FILE_DEVICE_VIRTUAL_DISK 0x00000024
/** @} */

/** @name Device Characteristic Constants
 *  @brief Bitmask values for device characteristics.
 *  @{
 */
#define FILE_REMOTE_DEVICE 0x00000010
#define FILE_REMOVABLE_MEDIA 0x00000002
/** @} */

/** @name Drive Type Constants
 *  @brief Identifies the type of a drive letter mapping.
 *  @see GetDriveTypeW
 *       https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getdrivetypew
 *  @{
 */
#define DRIVE_UNKNOWN 0
#define DRIVE_REMOVABLE 2
#define DRIVE_FIXED 3
#define DRIVE_REMOTE 4
#define DRIVE_CDROM 5
#define DRIVE_RAMDISK 6
/** @} */

/** @name Virtual Memory Constants
 *  @brief Allocation type and protection flags for virtual memory operations.
 *  @see ZwAllocateVirtualMemory
 *       https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-zwallocatevirtualmemory
 *  @{
 */
#define MEM_COMMIT 0x00001000
#define MEM_RESERVE 0x00002000
#define MEM_RELEASE 0x00008000
#define PAGE_READWRITE 0x04
/** @} */

#define INVALID_HANDLE_VALUE ((PVOID)(SSIZE)(-1))  ///< Sentinel value representing an invalid handle

/** @brief Process information class for querying the device map. */
#define ProcessDeviceMap 23

/**
 * @brief Initializes an OBJECT_ATTRIBUTES structure for use with NT Native API functions.
 *
 * @details Sets up the OBJECT_ATTRIBUTES fields required by ZwCreateFile,
 * ZwOpenFile, and other NT functions that accept object attributes. The
 * SecurityQualityOfService field is always set to nullptr.
 *
 * @param p Pointer to the OBJECT_ATTRIBUTES structure to initialize.
 * @param n Pointer to a UNICODE_STRING containing the object name (NT path).
 * @param a Attribute flags (e.g., OBJ_CASE_INSENSITIVE).
 * @param r Optional root directory handle for relative paths.
 * @param s Optional security descriptor.
 *
 * @see InitializeObjectAttributes macro
 *      https://learn.microsoft.com/en-us/windows/win32/api/ntdef/nf-ntdef-initializeobjectattributes
 */
#define InitializeObjectAttributes(p, n, a, r, s) \
	do                                            \
	{                                             \
		(p)->Length = sizeof(OBJECT_ATTRIBUTES);  \
		(p)->RootDirectory = r;                   \
		(p)->Attributes = a;                      \
		(p)->ObjectName = n;                      \
		(p)->SecurityDescriptor = s;              \
		(p)->SecurityQualityOfService = nullptr;  \
	} while (0)

/**
 * @brief Counted Unicode (UTF-16LE) string used throughout the NT Native API.
 *
 * @details Stores a length-prefixed wide character string. Unlike C-style
 * null-terminated strings, UNICODE_STRING tracks both the current length and
 * the buffer capacity, and may or may not be null-terminated.
 *
 * @see UNICODE_STRING structure
 *      https://learn.microsoft.com/en-us/windows/win32/api/ntdef/ns-ntdef-_unicode_string
 */
typedef struct _UNICODE_STRING
{
	UINT16 Length;        ///< Length of the string in bytes (not including any null terminator)
	UINT16 MaximumLength; ///< Total size of the Buffer in bytes
	PWCHAR Buffer;        ///< Pointer to the wide character string data
} UNICODE_STRING, *PUNICODE_STRING;

/** @brief NT status code type. Negative values indicate errors, zero/positive indicate success. */
typedef INT32 NTSTATUS;

/**
 * @brief Contains the process device map, mapping drive letters to device objects.
 *
 * @details Used with ZwQueryInformationProcess (ProcessDeviceMap class) to
 * retrieve the current drive letter mappings for the process. The Query
 * variant returns a bitmask of active drive letters and an array of drive
 * types.
 *
 * @see ZwQueryInformationProcess
 *      https://learn.microsoft.com/en-us/windows/win32/procthread/zwqueryinformationprocess
 */
typedef struct _PROCESS_DEVICEMAP_INFORMATION
{
	union
	{
		struct
		{
			PVOID DirectoryHandle; ///< Handle to a directory object to set as the device map (requires DIRECTORY_TRAVERSE access)
		} Set;
		struct
		{
			UINT32 DriveMap;     ///< Bitmask of active drive letters (bit 0 = A:, bit 1 = B:, ..., bit 25 = Z:)
			UCHAR DriveType[32]; ///< Drive type for each letter (DRIVE_FIXED, DRIVE_REMOTE, etc.)
		} Query;
	};
} PROCESS_DEVICEMAP_INFORMATION, *PPROCESS_DEVICEMAP_INFORMATION;

/**
 * @brief Contains the completion status and information for an I/O operation.
 *
 * @details Passed to all NT I/O functions (ZwReadFile, ZwWriteFile,
 * ZwDeviceIoControlFile, etc.) to receive the final NTSTATUS and the
 * number of bytes transferred. The Status and Pointer fields occupy the
 * same memory (union), with Pointer used internally by the I/O manager.
 *
 * @see IO_STATUS_BLOCK structure
 *      https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ns-wdm-_io_status_block
 */
typedef struct _IO_STATUS_BLOCK
{
	union
	{
		NTSTATUS Status; ///< Final NTSTATUS code for the I/O operation
		PVOID Pointer;   ///< Internal pointer used by the I/O manager (overlaps Status)
	};
	USIZE Information;   ///< Number of bytes transferred, or operation-specific information
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

/**
 * @brief Specifies the attributes and name of an object for NT Native API functions.
 *
 * @details Required by most NT object creation and open functions (ZwCreateFile,
 * ZwOpenFile, ZwCreateEvent, etc.) to specify the object's NT namespace path,
 * attribute flags, root directory, and security descriptor.
 *
 * @see OBJECT_ATTRIBUTES structure
 *      https://learn.microsoft.com/en-us/windows/win32/api/ntdef/ns-ntdef-_object_attributes
 */
typedef struct _OBJECT_ATTRIBUTES
{
	UINT32 Length;                  ///< Size of this structure in bytes (must be sizeof(OBJECT_ATTRIBUTES))
	PVOID RootDirectory;           ///< Optional handle to the root directory for relative ObjectName paths
	PUNICODE_STRING ObjectName;    ///< Pointer to the UNICODE_STRING containing the object's NT path
	UINT32 Attributes;             ///< Attribute flags (e.g., OBJ_CASE_INSENSITIVE, OBJ_INHERIT)
	PVOID SecurityDescriptor;      ///< Optional security descriptor for the object
	PVOID SecurityQualityOfService; ///< Optional security quality of service (for impersonation)
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

/**
 * @brief Represents a 64-bit signed integer as both a quad-part and a high/low pair.
 *
 * @details Used throughout the NT Native API for file sizes, byte offsets,
 * timestamps, and timeout values. Timeout values are expressed in 100-nanosecond
 * intervals; negative values indicate relative time from the current moment.
 *
 * @see LARGE_INTEGER union
 *      https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-large_integer-r1
 */
typedef union _LARGE_INTEGER
{
	struct
	{
		UINT32 LowPart;  ///< Low-order 32 bits of the 64-bit value
		INT32 HighPart;   ///< High-order 32 bits of the 64-bit value (signed)
	};
	struct
	{
		UINT32 LowPart;  ///< Low-order 32 bits (named member variant)
		INT32 HighPart;   ///< High-order 32 bits (named member variant)
	} u;                  ///< Named struct variant for compatibility
	INT64 QuadPart;       ///< Full 64-bit signed integer value
} LARGE_INTEGER, *PLARGE_INTEGER;

/** @brief System information class for querying firmware tables. */
#define SystemFirmwareTableInformation 76

/** @brief Firmware table provider signature for raw SMBIOS data. */
#define RSMB_PROVIDER_SIGNATURE 0x52534D42 // 'RSMB'

/**
 * @brief Action codes for SYSTEM_FIRMWARE_TABLE_INFORMATION.
 * @{
 */
#define SystemFirmwareTable_Enumerate 0
#define SystemFirmwareTable_Get 1
/** @} */

/**
 * @brief Input/output structure for SystemFirmwareTableInformation queries.
 *
 * @details Used with ZwQuerySystemInformation (class 76) to enumerate or
 * retrieve firmware tables. For SMBIOS, set ProviderSignature to 'RSMB',
 * Action to SystemFirmwareTable_Get, and TableID to 0.
 *
 * @see NtQuerySystemInformation
 *      https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntquerysysteminformation
 */
typedef struct _SYSTEM_FIRMWARE_TABLE_INFORMATION
{
	UINT32 ProviderSignature;  ///< Firmware table provider ('RSMB' for SMBIOS, 'FIRM' for firmware, 'ACPI' for ACPI)
	UINT32 Action;             ///< SystemFirmwareTable_Enumerate or SystemFirmwareTable_Get
	UINT32 TableID;            ///< Table identifier (0 for SMBIOS raw table)
	UINT32 TableBufferLength;  ///< Size of TableBuffer in bytes
	UINT8  TableBuffer[1];     ///< Variable-length buffer containing the firmware table data
} SYSTEM_FIRMWARE_TABLE_INFORMATION, *PSYSTEM_FIRMWARE_TABLE_INFORMATION;

/**
 * @brief Raw SMBIOS firmware table data header.
 *
 * @details Returned in SYSTEM_FIRMWARE_TABLE_INFORMATION::TableBuffer when
 * querying the 'RSMB' provider. The SMBIOSTableData array contains the
 * concatenated SMBIOS structures.
 *
 * @see GetSystemFirmwareTable
 *      https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getsystemfirmwaretable
 */
typedef struct _RAW_SMBIOS_DATA
{
	UINT8  Used20CallingMethod; ///< Whether the 2.0 calling method was used
	UINT8  SMBIOSMajorVersion;  ///< SMBIOS major version
	UINT8  SMBIOSMinorVersion;  ///< SMBIOS minor version
	UINT8  DmiRevision;         ///< DMI revision
	UINT32 Length;              ///< Total length of SMBIOSTableData in bytes
	UINT8  SMBIOSTableData[1];  ///< Variable-length SMBIOS structure data
} RAW_SMBIOS_DATA, *PRAW_SMBIOS_DATA;

/**
 * @brief SMBIOS structure header common to all structure types.
 *
 * @details Each SMBIOS structure begins with this 4-byte header, followed
 * by the formatted area (type-specific fields), then a string table
 * terminated by a double null (0x00, 0x00).
 *
 * @see SMBIOS Reference Specification (DSP0134)
 *      https://www.dmtf.org/standards/smbios
 */
typedef struct _SMBIOS_HEADER
{
	UINT8  Type;   ///< Structure type (0=BIOS, 1=System, 2=Baseboard, etc.)
	UINT8  Length; ///< Length of the formatted area (including this header)
	UINT16 Handle; ///< Unique handle for this structure instance
} SMBIOS_HEADER, *PSMBIOS_HEADER;

/**
 * @brief SMBIOS Type 1 — System Information.
 *
 * @details Contains the system manufacturer, product name, version, serial
 * number, and a 16-byte UUID assigned by the OEM. The UUID is valid for
 * SMBIOS version 2.1+ (Length >= 0x19).
 *
 * @note The UUID bytes are stored in mixed-endian format per SMBIOS spec:
 * the first three fields (TimeLow, TimeMid, TimeHiAndVersion) are
 * little-endian, while the remaining 8 bytes are big-endian.
 *
 * @see SMBIOS Reference Specification — Type 1: System Information
 *      https://www.dmtf.org/standards/smbios
 */
typedef struct _SMBIOS_TYPE1_SYSTEM_INFORMATION
{
	SMBIOS_HEADER Header;      ///< Type=1, Length>=0x19 for UUID-bearing entries
	UINT8  Manufacturer;       ///< String index for manufacturer name
	UINT8  ProductName;        ///< String index for product name
	UINT8  Version;            ///< String index for version string
	UINT8  SerialNumber;       ///< String index for serial number
	UINT8  UUID[16];           ///< 16-byte system UUID (valid when Header.Length >= 0x19)
} SMBIOS_TYPE1_SYSTEM_INFORMATION, *PSMBIOS_TYPE1_SYSTEM_INFORMATION;

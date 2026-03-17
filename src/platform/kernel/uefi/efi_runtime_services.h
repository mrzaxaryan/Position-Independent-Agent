/**
 * @file efi_runtime_services.h
 * @brief EFI Runtime Services function table.
 *
 * @details Defines the EFI_RUNTIME_SERVICES table and all associated function pointer typedefs
 *          and data structures. Runtime Services persist after ExitBootServices() and provide
 *          time services (GetTime, SetTime, GetWakeupTime, SetWakeupTime), variable services
 *          (GetVariable, SetVariable, GetNextVariableName), virtual memory mapping
 *          (SetVirtualAddressMap, ConvertPointer), system reset, and capsule update services.
 *          Also defines EFI_TIME, EFI_TIME_CAPABILITIES, EFI_RESET_TYPE, and EFI_CAPSULE_HEADER.
 *
 * @see UEFI Specification 2.10 — Section 4.5, EFI Runtime Services Table
 * @see UEFI Specification 2.10 — Section 8.1, Time Services
 * @see UEFI Specification 2.10 — Section 8.2, Variable Services
 * @see UEFI Specification 2.10 — Section 8.3, Virtual Memory Services
 * @see UEFI Specification 2.10 — Section 8.5, Miscellaneous Runtime Services
 */

#pragma once

#include "platform/kernel/uefi/efi_types.h"

// =============================================================================
// Time Structures
// =============================================================================

typedef struct
{
	UINT16 Year;  // 1900 - 9999
	UINT8 Month;  // 1 - 12
	UINT8 Day;	  // 1 - 31
	UINT8 Hour;	  // 0 - 23
	UINT8 Minute; // 0 - 59
	UINT8 Second; // 0 - 59
	UINT8 Pad1;
	UINT32 Nanosecond; // 0 - 999,999,999
	INT16 TimeZone;	   // -1440 to 1440 or EFI_UNSPECIFIED_TIMEZONE
	UINT8 Daylight;
	UINT8 Pad2;
} EFI_TIME;

typedef struct
{
	UINT32 Resolution; // 1e-6 parts per million
	UINT32 Accuracy;   // Error rate in 1e-6 parts per million
	BOOL SetsToZero;   // true if time is lost at reset
} EFI_TIME_CAPABILITIES;

#define EFI_UNSPECIFIED_TIMEZONE 0x07FF

// Daylight Saving flags
#define EFI_TIME_ADJUST_DAYLIGHT 0x01
#define EFI_TIME_IN_DAYLIGHT 0x02

// =============================================================================
// Variable Attributes
// =============================================================================

#define EFI_VARIABLE_NON_VOLATILE 0x00000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS 0x00000002
#define EFI_VARIABLE_RUNTIME_ACCESS 0x00000004
#define EFI_VARIABLE_HARDWARE_ERROR_RECORD 0x00000008
#define EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS 0x00000010

// =============================================================================
// Reset Types
// =============================================================================

typedef enum
{
	EfiResetCold,
	EfiResetWarm,
	EfiResetShutdown,
	EfiResetPlatformSpecific
} EFI_RESET_TYPE;

// =============================================================================
// Capsule Structures (minimal)
// =============================================================================

typedef struct
{
	EFI_GUID CapsuleGuid;
	UINT32 HeaderSize;
	UINT32 Flags;
	UINT32 CapsuleImageSize;
} EFI_CAPSULE_HEADER;

// =============================================================================
// Runtime Services Function Types
// =============================================================================

// Time Services
typedef EFI_STATUS(EFIAPI *EFI_GET_TIME)(
	EFI_TIME *Time,
	EFI_TIME_CAPABILITIES *Capabilities);

typedef EFI_STATUS(EFIAPI *EFI_SET_TIME)(EFI_TIME *Time);

typedef EFI_STATUS(EFIAPI *EFI_GET_WAKEUP_TIME)(
	BOOL *Enabled,
	BOOL *Pending,
	EFI_TIME *Time);

typedef EFI_STATUS(EFIAPI *EFI_SET_WAKEUP_TIME)(
	BOOL Enable,
	EFI_TIME *Time);

// Virtual Memory Services
typedef EFI_STATUS(EFIAPI *EFI_SET_VIRTUAL_ADDRESS_MAP)(
	USIZE MemoryMapSize,
	USIZE DescriptorSize,
	UINT32 DescriptorVersion,
	PVOID VirtualMap);

typedef EFI_STATUS(EFIAPI *EFI_CONVERT_POINTER)(
	USIZE DebugDisposition,
	PVOID *Address);

// Variable Services
typedef EFI_STATUS(EFIAPI *EFI_GET_VARIABLE)(
	CHAR16 *VariableName,
	EFI_GUID *VendorGuid,
	UINT32 *Attributes,
	USIZE *DataSize,
	PVOID Data);

typedef EFI_STATUS(EFIAPI *EFI_GET_NEXT_VARIABLE_NAME)(
	USIZE *VariableNameSize,
	CHAR16 *VariableName,
	EFI_GUID *VendorGuid);

typedef EFI_STATUS(EFIAPI *EFI_SET_VARIABLE)(
	CHAR16 *VariableName,
	EFI_GUID *VendorGuid,
	UINT32 Attributes,
	USIZE DataSize,
	PVOID Data);

// Miscellaneous Services
typedef EFI_STATUS(EFIAPI *EFI_GET_NEXT_HIGH_MONO_COUNT)(UINT32 *HighCount);

typedef VOID(EFIAPI *EFI_RESET_SYSTEM)(
	EFI_RESET_TYPE ResetType,
	EFI_STATUS ResetStatus,
	USIZE DataSize,
	PVOID ResetData);

// Capsule Services (UEFI 2.0+)
typedef EFI_STATUS(EFIAPI *EFI_UPDATE_CAPSULE)(
	EFI_CAPSULE_HEADER **CapsuleHeaderArray,
	USIZE CapsuleCount,
	EFI_PHYSICAL_ADDRESS ScatterGatherList);

typedef EFI_STATUS(EFIAPI *EFI_QUERY_CAPSULE_CAPABILITIES)(
	EFI_CAPSULE_HEADER **CapsuleHeaderArray,
	USIZE CapsuleCount,
	UINT64 *MaximumCapsuleSize,
	EFI_RESET_TYPE *ResetType);

// Variable Info (UEFI 2.0+)
typedef EFI_STATUS(EFIAPI *EFI_QUERY_VARIABLE_INFO)(
	UINT32 Attributes,
	UINT64 *MaximumVariableStorageSize,
	UINT64 *RemainingVariableStorageSize,
	UINT64 *MaximumVariableSize);

// =============================================================================
// Runtime Services Table
// =============================================================================

typedef struct EFI_RUNTIME_SERVICES
{
	EFI_TABLE_HEADER Hdr;

	// Time Services
	EFI_GET_TIME GetTime;
	EFI_SET_TIME SetTime;
	EFI_GET_WAKEUP_TIME GetWakeupTime;
	EFI_SET_WAKEUP_TIME SetWakeupTime;

	// Virtual Memory Services
	EFI_SET_VIRTUAL_ADDRESS_MAP SetVirtualAddressMap;
	EFI_CONVERT_POINTER ConvertPointer;

	// Variable Services
	EFI_GET_VARIABLE GetVariable;
	EFI_GET_NEXT_VARIABLE_NAME GetNextVariableName;
	EFI_SET_VARIABLE SetVariable;

	// Miscellaneous Services
	EFI_GET_NEXT_HIGH_MONO_COUNT GetNextHighMonotonicCount;
	EFI_RESET_SYSTEM ResetSystem;

	// Capsule Services (UEFI 2.0+)
	EFI_UPDATE_CAPSULE UpdateCapsule;
	EFI_QUERY_CAPSULE_CAPABILITIES QueryCapsuleCapabilities;

	// Variable Info (UEFI 2.0+)
	EFI_QUERY_VARIABLE_INFO QueryVariableInfo;
} EFI_RUNTIME_SERVICES;

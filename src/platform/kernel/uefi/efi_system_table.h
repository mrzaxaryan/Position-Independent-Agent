/**
 * @file efi_system_table.h
 * @brief EFI System Table and configuration table structures.
 *
 * @details Defines the EFI_SYSTEM_TABLE, which is the primary entry point structure passed
 *          to every UEFI application. It provides access to console I/O protocols, Boot Services,
 *          Runtime Services, and firmware configuration tables (ACPI, SMBIOS). Also defines
 *          EFI_CONFIGURATION_TABLE and well-known configuration table GUIDs.
 *
 * @see UEFI Specification 2.10 — Section 4.3, EFI System Table
 * @see UEFI Specification 2.10 — Section 4.6, EFI Configuration Table & Properties Table
 */

#pragma once

#include "platform/kernel/uefi/efi_types.h"
#include "platform/kernel/uefi/efi_protocols.h"
#include "platform/kernel/uefi/efi_boot_services.h"
#include "platform/kernel/uefi/efi_runtime_services.h"

// =============================================================================
// Configuration Table
// =============================================================================

typedef struct
{
	EFI_GUID VendorGuid;
	PVOID VendorTable;
} EFI_CONFIGURATION_TABLE;

// Well-known configuration table GUIDs
#define EFI_ACPI_20_TABLE_GUID                                                 \
	{                                                                          \
		0x8868e871, 0xe4f1, 0x11d3,                                             \
		{                                                                      \
			0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81                      \
		}                                                                      \
	}

#define EFI_ACPI_TABLE_GUID                                                    \
	{                                                                          \
		0xeb9d2d30, 0x2d88, 0x11d3,                                             \
		{                                                                      \
			0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d                      \
		}                                                                      \
	}

#define SMBIOS_TABLE_GUID                                                      \
	{                                                                          \
		0xeb9d2d31, 0x2d88, 0x11d3,                                             \
		{                                                                      \
			0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d                      \
		}                                                                      \
	}

#define SMBIOS3_TABLE_GUID                                                     \
	{                                                                          \
		0xf2fd1544, 0x9794, 0x4a2c,                                             \
		{                                                                      \
			0x99, 0x2e, 0xe5, 0xbb, 0xcf, 0x20, 0xe3, 0x94                      \
		}                                                                      \
	}

// =============================================================================
// System Table Signature
// =============================================================================

#define EFI_SYSTEM_TABLE_SIGNATURE 0x5453595320494249ULL // "IBI SYST"

// =============================================================================
// EFI System Table
// =============================================================================

typedef struct EFI_SYSTEM_TABLE {
	EFI_TABLE_HEADER Hdr;

	// Firmware vendor string (null-terminated)
	CHAR16 *FirmwareVendor;
	UINT32 FirmwareRevision;

	// Console handles and protocols
	EFI_HANDLE ConsoleInHandle;
	EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn;
	EFI_HANDLE ConsoleOutHandle;
	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
	EFI_HANDLE StandardErrorHandle;
	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;

	// Service tables
	EFI_RUNTIME_SERVICES *RuntimeServices;
	EFI_BOOT_SERVICES *BootServices;

	// Configuration tables (ACPI, SMBIOS, etc.)
	USIZE NumberOfTableEntries;
	EFI_CONFIGURATION_TABLE *ConfigurationTable;
} EFI_SYSTEM_TABLE;

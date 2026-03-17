/**
 * @file uefi_fs_helpers.h
 * @brief UEFI filesystem helper functions
 *
 * @details Provides utilities for locating and opening files through
 * the EFI_SIMPLE_FILE_SYSTEM_PROTOCOL. All GUID construction is done
 * on the stack to avoid .rdata dependencies.
 */
#pragma once

#include "platform/kernel/uefi/efi_context.h"
#include "platform/kernel/uefi/efi_file_protocol.h"
#include "core/types/primitives.h"
#include "core/types/span.h"

/**
 * @brief Build EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID on the stack (no .rdata)
 * @return EFI_GUID {964E5B22-6459-11D2-8E39-00A0C969723B}
 */
NOINLINE EFI_GUID MakeFsProtocolGuid();

/**
 * @brief Get the root directory handle from the first working filesystem
 * @return Root directory file protocol handle, or nullptr on failure
 */
EFI_FILE_PROTOCOL *GetRootDirectory();

/**
 * @brief Open a file by path from a root directory handle
 * @param root Root directory protocol handle
 * @param path Wide-character file path (normalized to backslashes for UEFI)
 * @param mode EFI file mode flags (EFI_FILE_MODE_READ, etc.)
 * @param attributes EFI file attributes
 * @return File protocol handle, or nullptr on failure
 */
EFI_FILE_PROTOCOL *OpenFileFromRoot(EFI_FILE_PROTOCOL *root, PCWCHAR path, UINT64 mode, UINT64 attributes);

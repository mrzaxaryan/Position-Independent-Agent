/**
 * @file system_info.h
 * @brief System information retrieval
 *
 * @details Provides a cross-platform function to retrieve basic system
 * information including machine UUID, hostname, CPU architecture, and
 * OS platform.
 *
 * Platform implementations:
 * - Windows: UUID from SMBIOS, hostname from COMPUTERNAME environment variable
 * - Linux/macOS/FreeBSD/Solaris/Android: UUID from /etc/machine-id,
 *   hostname from HOSTNAME environment variable or /etc/hostname
 * - UEFI: UUID unavailable, hostname unavailable (returns defaults)
 *
 * Architecture and platform strings are determined at compile time from
 * the ARCHITECTURE_* and PLATFORM_* preprocessor defines.
 *
 * @ingroup platform
 */

#pragma once

#include "core/core.h"
#include "core/types/uuid.h"

/**
 * @brief System information structure
 *
 * @details Contains identifying information about the host system.
 * Hostname, Architecture, and Platform are null-terminated narrow strings.
 */
#pragma pack(push, 1)
struct SystemInfo
{
    UUID MachineUUID;        ///< Machine-unique identifier (hardware/OS level)
    CHAR Hostname[256];      ///< Machine hostname / computer name
    CHAR Architecture[32];   ///< CPU architecture (e.g. "x86_64", "aarch64")
    CHAR Platform[32];       ///< OS platform (e.g. "windows", "linux")
};
#pragma pack(pop)

/**
 * @brief Retrieves system information for the current host.
 *
 * @details Populates a SystemInfo structure with:
 * - MachineUUID: Hardware/OS-level unique identifier (platform-specific)
 * - Hostname: Retrieved from OS environment (platform-specific)
 * - Architecture: Compile-time string from ARCHITECTURE_* define
 * - Platform: Compile-time string from PLATFORM_* define
 *
 * @param[out] info Pointer to SystemInfo structure to populate.
 *                  The structure is zeroed before populating.
 */
VOID GetSystemInfo(SystemInfo *info);

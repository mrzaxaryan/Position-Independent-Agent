/**
 * @file machine_id.h
 * @brief Machine-unique identifier retrieval
 *
 * @details Provides a cross-platform function to retrieve a stable,
 * machine-unique UUID from the operating system. The identifier is
 * constant across reboots and process restarts.
 *
 * Platform implementations:
 * - Windows: SMBIOS Type 1 UUID via NtQuerySystemInformation (hardware-level,
 *   survives OS reinstalls)
 * - Linux/FreeBSD: /etc/machine-id (systemd, generated at install),
 *   falls back to /proc/sys/kernel/random/boot_id (changes per reboot)
 * - Android: /etc/machine-id if available, otherwise
 *   /proc/sys/kernel/random/boot_id (changes per reboot)
 * - macOS/iOS: sysctl kern.uuid (IOPlatformUUID, hardware-level)
 *
 * @ingroup platform
 */

#pragma once

#include "core/types/uuid.h"
#include "core/types/error.h"
#include "core/types/result.h"

/**
 * @brief Retrieves a machine-unique UUID from the operating system.
 *
 * @details Platform-specific implementations:
 * - Windows: Extracts the SMBIOS Type 1 (System Information) UUID from
 *   firmware tables via NtQuerySystemInformation. This is a hardware-level
 *   UUID assigned by the OEM, constant across reboots and OS reinstalls.
 * - Linux/FreeBSD: Reads /etc/machine-id (systemd 128-bit identifier,
 *   generated at install time, constant across reboots). Falls back to
 *   /proc/sys/kernel/random/boot_id (Linux only, changes per reboot).
 * - Android: Reads /etc/machine-id if available (e.g. Termux on rooted
 *   devices), falls back to /proc/sys/kernel/random/boot_id.
 * - macOS/iOS: Queries kern.uuid via sysctl (IOPlatformUUID, hardware-level,
 *   constant across reboots and OS reinstalls).
 *
 * @return Ok(UUID) on success, Err on failure (firmware table unavailable,
 *         SMBIOS Type 1 not found, file not readable, etc.)
 */
[[nodiscard]] Result<UUID, Error> GetMachineUUID();

/**
 * @file efi_context.h
 * @brief UEFI runtime context storage and access.
 *
 * @details Defines the EFI_CONTEXT structure that holds the EFI_HANDLE (ImageHandle),
 *          EFI_SYSTEM_TABLE pointer, and network initialization state flags. The context
 *          is populated at UEFI application entry and stored in a CPU register for fast
 *          access throughout the platform layer. Architecture-specific context register
 *          accessors (SetEfiContextRegister, GetEfiContext) are included via conditional
 *          compilation for x86_64 (GS base MSR) and AArch64 (TPIDR_EL0).
 *
 * @see UEFI Specification 2.10 — Section 4.1, UEFI Image Entry Point
 * @see UEFI Specification 2.10 — Section 4.3, EFI System Table
 */

#pragma once

#include "platform/kernel/uefi/efi_system_table.h"

// =============================================================================
// EFI Context Structure
// =============================================================================

struct EFI_CONTEXT
{
	EFI_HANDLE ImageHandle;
	EFI_SYSTEM_TABLE *SystemTable;
	BOOL NetworkInitialized;
	BOOL DhcpConfigured;
	BOOL TcpStackReady;
};

// =============================================================================
// Context Register Access — architecture-specific implementations
// =============================================================================

#if defined(ARCHITECTURE_X86_64)
#include "platform/kernel/uefi/efi_context.x86_64.h"
#elif defined(ARCHITECTURE_AARCH64)
#include "platform/kernel/uefi/efi_context.aarch64.h"
#else
#error "Unsupported architecture"
#endif

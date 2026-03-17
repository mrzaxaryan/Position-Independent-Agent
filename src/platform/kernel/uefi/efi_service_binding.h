/**
 * @file efi_service_binding.h
 * @brief EFI Service Binding Protocol for child handle management.
 *
 * @details Defines the EFI_SERVICE_BINDING_PROTOCOL, a generic protocol interface used by
 *          network protocol drivers (TCP4, TCP6, UDP4, UDP6, etc.) to create and destroy
 *          child handles. CreateChild produces a new child handle with the associated protocol
 *          instance, and DestroyChild tears it down. This protocol is the foundation of the
 *          UEFI network stack's per-connection handle model.
 *
 * @see UEFI Specification 2.10 — Section 10.6, EFI Service Binding Protocol
 */

#pragma once

#include "platform/kernel/uefi/efi_types.h"

// =============================================================================
// Service Binding Protocol
// =============================================================================

typedef struct EFI_SERVICE_BINDING_PROTOCOL EFI_SERVICE_BINDING_PROTOCOL;

typedef EFI_STATUS(EFIAPI *EFI_SERVICE_BINDING_CREATE_CHILD)(
	EFI_SERVICE_BINDING_PROTOCOL *This,
	EFI_HANDLE *ChildHandle);

typedef EFI_STATUS(EFIAPI *EFI_SERVICE_BINDING_DESTROY_CHILD)(
	EFI_SERVICE_BINDING_PROTOCOL *This,
	EFI_HANDLE ChildHandle);

struct EFI_SERVICE_BINDING_PROTOCOL
{
	EFI_SERVICE_BINDING_CREATE_CHILD CreateChild;
	EFI_SERVICE_BINDING_DESTROY_CHILD DestroyChild;
};

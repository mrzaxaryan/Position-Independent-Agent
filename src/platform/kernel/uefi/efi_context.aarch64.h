/**
 * @file efi_context.aarch64.h
 * @brief AArch64 UEFI context register access via TPIDR_EL0.
 *
 * @details Provides SetEfiContextRegister() and GetEfiContext() for storing and retrieving
 *          the EFI_CONTEXT pointer in the TPIDR_EL0 (thread pointer) system register on
 *          AArch64. This register is freely available in UEFI environments and provides a
 *          fast, per-CPU storage location for the runtime context without requiring memory
 *          indirection.
 */
#pragma once

#include "core/types/primitives.h"

struct EFI_CONTEXT;

/**
 * SetEfiContextRegister - Store context pointer in CPU register
 *
 * Uses TPIDR_EL0 (thread pointer register) on AArch64.
 *
 * @param ctx Reference to the EFI context to store
 */
inline VOID SetEfiContextRegister(EFI_CONTEXT &ctx)
{
	__asm__ volatile("msr tpidr_el0, %0" : : "r"(&ctx) : "memory");
}

/**
 * GetEfiContext - Get the EFI context from CPU register
 *
 * Retrieves the context pointer from TPIDR_EL0 (thread pointer register).
 *
 * @return Pointer to the EFI context
 */
inline EFI_CONTEXT *GetEfiContext()
{
	EFI_CONTEXT *ctx;
	__asm__ volatile("mrs %0, tpidr_el0" : "=r"(ctx));
	return ctx;
}

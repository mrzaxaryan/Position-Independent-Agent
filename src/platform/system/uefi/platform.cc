/**
 * platform.cc - UEFI Platform Abstraction Layer Core
 *
 * Provides ExitProcess implementation for UEFI.
 */

#include "platform/platform.h"
#include "platform/kernel/uefi/efi_context.h"

/**
 * ExitProcess - Shutdown the system
 *
 * Uses EFI Runtime Services ResetSystem() to power off the machine.
 *
 * @param code - Exit code (0 = success, non-zero = error)
 */
NO_RETURN VOID ExitProcess(USIZE code)
{
	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_RUNTIME_SERVICES *rs = ctx->SystemTable->RuntimeServices;

	rs->ResetSystem(EfiResetShutdown, (EFI_STATUS)code, 0, nullptr);

	__builtin_unreachable();
}

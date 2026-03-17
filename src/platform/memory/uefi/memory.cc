/**
 * @file memory.cc
 * @brief UEFI memory allocation implementation.
 * @details Provides Allocator::AllocateMemory and ReleaseMemory using
 * EFI Boot Services AllocatePool/FreePool.
 */

#include "platform/memory/allocator.h"
#include "platform/kernel/uefi/efi_context.h"

/**
 * @brief Allocates memory from the UEFI pool.
 * @details Uses EFI_BOOT_SERVICES->AllocatePool with EfiLoaderData type.
 * @param size Number of bytes to allocate.
 * @return Pointer to allocated memory, or nullptr on failure.
 */
PVOID Allocator::AllocateMemory(USIZE size)
{
	if (size == 0)
		return nullptr;

	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_BOOT_SERVICES *bs = ctx->SystemTable->BootServices;

	PVOID buffer = nullptr;
	EFI_STATUS status = bs->AllocatePool(EfiLoaderData, size, &buffer);

	if (status != EFI_SUCCESS)
		return nullptr;

	return buffer;
}

/**
 * @brief Releases memory back to the UEFI pool.
 * @details Uses EFI_BOOT_SERVICES->FreePool. UEFI FreePool does not require
 * the size parameter (unlike munmap on POSIX).
 * @param ptr Pointer to memory to free.
 */
VOID Allocator::ReleaseMemory(PVOID ptr, USIZE)
{
	if (ptr == nullptr)
		return;

	EFI_CONTEXT *ctx = GetEfiContext();
	EFI_BOOT_SERVICES *bs = ctx->SystemTable->BootServices;

	bs->FreePool(ptr);
}

/**
 * @file allocator.h
 * @brief Heap memory allocation
 *
 * @details Provides platform-specific virtual memory allocation and release via
 * direct syscalls. On Windows this uses NtAllocateVirtualMemory/NtFreeVirtualMemory,
 * on Linux/macOS/Solaris it uses mmap/munmap, and on UEFI it uses AllocatePool/
 * FreePool. For memory operations (copy, set, compare), use core/memory.h instead.
 */

#pragma once

#include "core/types/primitives.h"

class Allocator
{
public:
	/**
	 * @brief Allocates a block of virtual memory of the requested size.
	 * @details Uses platform-specific syscalls: NtAllocateVirtualMemory on Windows,
	 * mmap on Linux/macOS/Solaris, and AllocatePool on UEFI.
	 * @param size Number of bytes to allocate.
	 * @return Pointer to the allocated memory, or nullptr on failure.
	 */
	static PVOID AllocateMemory(USIZE size);

	/**
	 * @brief Releases a previously allocated block of virtual memory.
	 * @details Uses platform-specific syscalls: NtFreeVirtualMemory on Windows,
	 * munmap on Linux/macOS/Solaris, and FreePool on UEFI.
	 * @param ptr Pointer to the memory block to release.
	 * @param size Size of the memory block in bytes (required by munmap on POSIX).
	 */
	static VOID ReleaseMemory(PVOID ptr, USIZE size);
};

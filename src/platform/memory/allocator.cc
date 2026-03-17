/**
 * @file allocator.cc
 * @brief Global operator new/delete overloads.
 * @details Routes all heap allocations through the platform-specific Allocator,
 * eliminating CRT dependencies.
 */

#include "platform/memory/allocator.h"

PVOID operator new(USIZE size)
{
	return Allocator::AllocateMemory(size);
}

PVOID operator new[](USIZE size)
{
	return Allocator::AllocateMemory(size);
}

VOID operator delete(PVOID p) noexcept
{
	Allocator::ReleaseMemory(p, 0);
}

VOID operator delete[](PVOID p) noexcept
{
	Allocator::ReleaseMemory(p, 0);
}

VOID operator delete(PVOID p, USIZE s) noexcept
{
	Allocator::ReleaseMemory(p, s);
}

VOID operator delete[](PVOID p, USIZE s) noexcept
{
	Allocator::ReleaseMemory(p, s);
}
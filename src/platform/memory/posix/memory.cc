/**
 * @file memory.cc
 * @brief POSIX memory allocation implementation.
 * @details Provides Allocator::AllocateMemory and ReleaseMemory using
 * mmap/munmap syscalls. A USIZE header is prepended to every allocation
 * to store the total mapped size for munmap.
 */

#include "platform/memory/allocator.h"
#if defined(PLATFORM_LINUX)
#include "platform/kernel/linux/syscall.h"
#include "platform/kernel/linux/system.h"
#elif defined(PLATFORM_ANDROID)
#include "platform/kernel/android/syscall.h"
#include "platform/kernel/android/system.h"
#elif defined(PLATFORM_MACOS)
#include "platform/kernel/macos/syscall.h"
#include "platform/kernel/macos/system.h"
#elif defined(PLATFORM_IOS)
#include "platform/kernel/ios/syscall.h"
#include "platform/kernel/ios/system.h"
#elif defined(PLATFORM_SOLARIS)
#include "platform/kernel/solaris/syscall.h"
#include "platform/kernel/solaris/system.h"
#elif defined(PLATFORM_FREEBSD)
#include "platform/kernel/freebsd/syscall.h"
#include "platform/kernel/freebsd/system.h"
#endif

// Memory allocator using mmap/munmap
// Each allocation is a separate mmap, which is simple but not efficient for
// many small allocations. Suitable for basic needs.
//
// A USIZE header is prepended to every allocation to store the total mmap'd
// size. This allows ReleaseMemory to call munmap without requiring the caller
// to supply the size (unsized operator delete passes size = 0).

PVOID Allocator::AllocateMemory(USIZE size)
{
	if (size == 0)
		return nullptr;

	// Total allocation = header + requested size, aligned to page boundary
	USIZE totalSize = (size + sizeof(USIZE) + 4095) & ~(USIZE)4095;

	// Use mmap to allocate memory
	// fd = -1 for anonymous mapping (no file backing)
	PVOID addr = nullptr;
	INT32 prot = PROT_READ | PROT_WRITE;
	INT32 flags = MAP_PRIVATE | MAP_ANONYMOUS;

#if (defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)) && (defined(ARCHITECTURE_I386) || defined(ARCHITECTURE_ARMV7A) || defined(ARCHITECTURE_RISCV32))
	// 32-bit Linux architectures use mmap2 with page-shifted offset
	USIZE offset = 0;
	SSIZE result = System::Call(SYS_MMAP2, (USIZE)addr, totalSize, prot, flags, -1, offset);
#elif defined(PLATFORM_FREEBSD) && defined(ARCHITECTURE_I386)
	// FreeBSD i386: SYS_MMAP takes off_t pos (64-bit), which occupies
	// two 32-bit stack slots. The 6-arg System::Call only pushes one slot for
	// the offset, leaving garbage in pos_hi. FreeBSD rejects MAP_ANONYMOUS
	// with non-zero offset (EINVAL), causing allocation failure and segfault.
	// Use direct assembly to push all 7 argument slots + dummy = 32 bytes.
	SSIZE result;
	register USIZE r1 __asm__("ebx") = (USIZE)addr;
	register USIZE r2 __asm__("ecx") = totalSize;
	register USIZE r3 __asm__("edx") = (USIZE)prot;
	register USIZE r4 __asm__("esi") = (USIZE)flags;
	register USIZE r5 __asm__("edi") = (USIZE)(SSIZE)-1; // fd
	__asm__ volatile(
		"pushl $0\n"          // off_t pos high 32 bits = 0
		"pushl $0\n"          // off_t pos low 32 bits = 0
		"pushl %%edi\n"       // fd = -1
		"pushl %%esi\n"       // flags
		"pushl %%edx\n"       // prot
		"pushl %%ecx\n"       // len
		"pushl %%ebx\n"       // addr
		"pushl $0\n"          // dummy return address
		"int $0x80\n"
		"jnc 1f\n"
		"negl %%eax\n"
		"1:\n"
		"addl $32, %%esp\n"
		: "=a"(result)
		: "a"((USIZE)SYS_MMAP), "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5)
		: "memory", "cc"
	);
#else
	USIZE offset = 0;
	SSIZE result = System::Call(SYS_MMAP, (USIZE)addr, totalSize, prot, flags, -1, offset);
#endif

	// mmap returns -1 on error (well, actually MAP_FAILED which is (void*)-1)
	if (result < 0 && result >= -4095)
		return nullptr;

	// Store total mmap'd size in the header
	PCHAR base = (PCHAR)result;
	*(USIZE*)base = totalSize;

	// Return pointer past the header
	return (PVOID)(base + sizeof(USIZE));
}

// Release memory allocated by AllocateMemory
VOID Allocator::ReleaseMemory(PVOID address, USIZE)
{	
	// Validate address
	if (address == nullptr)
		return;

	// Recover base pointer and total size from the prepended header
	PCHAR base = (PCHAR)address - sizeof(USIZE);
	USIZE totalSize = *(USIZE*)base;

	// Use munmap to release the entire mapping
	System::Call(SYS_MUNMAP, (USIZE)base, totalSize);
}

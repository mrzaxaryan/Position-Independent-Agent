/**
 * @file system.aarch64.h
 * @brief AArch64 FreeBSD syscall implementation via inline assembly.
 *
 * @details Provides System::Call overloads (0-6 arguments) that invoke FreeBSD
 * syscalls using svc #0 with the syscall number in x8 (standard ARM64
 * convention, same as Linux/Solaris, not svc #0x80 / x16 as on macOS).
 * Arguments are passed in x0-x5. Errors are indicated by the carry flag
 * (C bit in NZCV); the inline assembly negates x0 on error to normalize
 * to the negative-return convention. X1 is clobbered by the kernel's rval[1]
 * secondary return value.
 */
#pragma once

#include "core/types/primitives.h"

class System
{
public:

	// Syscall with 0 arguments
	// Note: FreeBSD kernel may write rval[1] to X1 on return, so X1 must be clobbered.
	static NOINLINE SSIZE Call(USIZE number)
	{
		register USIZE x0 __asm__("x0");
		register USIZE x8 __asm__("x8") = number;
		__asm__ volatile(
			"svc #0\n"
			"b.cc 1f\n"
			"neg x0, x0\n"
			"1:\n"
			: "=r"(x0)
			: "r"(x8)
			: "x1", "memory", "cc"
		);
		return (SSIZE)x0;
	}

	// Syscall with 1 argument
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1)
	{
		register USIZE x0 __asm__("x0") = arg1;
		register USIZE x8 __asm__("x8") = number;
		__asm__ volatile(
			"svc #0\n"
			"b.cc 1f\n"
			"neg x0, x0\n"
			"1:\n"
			: "+r"(x0)
			: "r"(x8)
			: "x1", "memory", "cc"
		);
		return (SSIZE)x0;
	}

	// Syscall with 2 arguments
	// X1 is used as input and clobbered by kernel (rval[1]), so mark as "+r".
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2)
	{
		register USIZE x0 __asm__("x0") = arg1;
		register USIZE x1 __asm__("x1") = arg2;
		register USIZE x8 __asm__("x8") = number;
		__asm__ volatile(
			"svc #0\n"
			"b.cc 1f\n"
			"neg x0, x0\n"
			"1:\n"
			: "+r"(x0), "+r"(x1)
			: "r"(x8)
			: "memory", "cc"
		);
		return (SSIZE)x0;
	}

	// Syscall with 3 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3)
	{
		register USIZE x0 __asm__("x0") = arg1;
		register USIZE x1 __asm__("x1") = arg2;
		register USIZE x2 __asm__("x2") = arg3;
		register USIZE x8 __asm__("x8") = number;
		__asm__ volatile(
			"svc #0\n"
			"b.cc 1f\n"
			"neg x0, x0\n"
			"1:\n"
			: "+r"(x0), "+r"(x1)
			: "r"(x2), "r"(x8)
			: "memory", "cc"
		);
		return (SSIZE)x0;
	}

	// Syscall with 4 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4)
	{
		register USIZE x0 __asm__("x0") = arg1;
		register USIZE x1 __asm__("x1") = arg2;
		register USIZE x2 __asm__("x2") = arg3;
		register USIZE x3 __asm__("x3") = arg4;
		register USIZE x8 __asm__("x8") = number;
		__asm__ volatile(
			"svc #0\n"
			"b.cc 1f\n"
			"neg x0, x0\n"
			"1:\n"
			: "+r"(x0), "+r"(x1)
			: "r"(x2), "r"(x3), "r"(x8)
			: "memory", "cc"
		);
		return (SSIZE)x0;
	}

	// Syscall with 5 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5)
	{
		register USIZE x0 __asm__("x0") = arg1;
		register USIZE x1 __asm__("x1") = arg2;
		register USIZE x2 __asm__("x2") = arg3;
		register USIZE x3 __asm__("x3") = arg4;
		register USIZE x4 __asm__("x4") = arg5;
		register USIZE x8 __asm__("x8") = number;
		__asm__ volatile(
			"svc #0\n"
			"b.cc 1f\n"
			"neg x0, x0\n"
			"1:\n"
			: "+r"(x0), "+r"(x1)
			: "r"(x2), "r"(x3), "r"(x4), "r"(x8)
			: "memory", "cc"
		);
		return (SSIZE)x0;
	}

	// Syscall with 6 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5, USIZE arg6)
	{
		register USIZE x0 __asm__("x0") = arg1;
		register USIZE x1 __asm__("x1") = arg2;
		register USIZE x2 __asm__("x2") = arg3;
		register USIZE x3 __asm__("x3") = arg4;
		register USIZE x4 __asm__("x4") = arg5;
		register USIZE x5 __asm__("x5") = arg6;
		register USIZE x8 __asm__("x8") = number;
		__asm__ volatile(
			"svc #0\n"
			"b.cc 1f\n"
			"neg x0, x0\n"
			"1:\n"
			: "+r"(x0), "+r"(x1)
			: "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x8)
			: "memory", "cc"
		);
		return (SSIZE)x0;
	}

};  // class System

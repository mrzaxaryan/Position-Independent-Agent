/**
 * @file system.aarch64.h
 * @brief AArch64 Linux syscall implementation via inline assembly.
 *
 * @details Provides System::Call overloads (0-6 arguments) that invoke Linux
 * syscalls using the svc #0 supervisor call instruction. Arguments are passed
 * in registers x0-x5 with the syscall number in x8. The return value is in x0.
 */
#pragma once

#include "core/types/primitives.h"
class System
{
public:

	// Syscall with 0 arguments
	static NOINLINE SSIZE Call(USIZE number)
	{
		register USIZE x0 __asm__("x0");
		register USIZE x8 __asm__("x8") = number;
		__asm__ volatile(
			"svc #0\n"
			: "=r"(x0)
			: "r"(x8)
			: "memory", "cc"
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
			: "+r"(x0)
			: "r"(x8)
			: "memory", "cc"
		);
		return (SSIZE)x0;
	}

	// Syscall with 2 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2)
	{
		register USIZE x0 __asm__("x0") = arg1;
		register USIZE x1 __asm__("x1") = arg2;
		register USIZE x8 __asm__("x8") = number;
		__asm__ volatile(
			"svc #0\n"
			: "+r"(x0)
			: "r"(x1), "r"(x8)
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
			: "+r"(x0)
			: "r"(x1), "r"(x2), "r"(x8)
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
			: "+r"(x0)
			: "r"(x1), "r"(x2), "r"(x3), "r"(x8)
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
			: "+r"(x0)
			: "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x8)
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
			: "+r"(x0)
			: "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x8)
			: "memory", "cc"
		);
		return (SSIZE)x0;
	}

};  // class System

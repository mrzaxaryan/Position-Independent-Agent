/**
 * @file system.armv7a.h
 * @brief ARMv7-A Linux syscall implementation via inline assembly.
 *
 * @details Provides System::Call overloads (0-6 arguments) that invoke Linux
 * syscalls using the svc #0 supervisor call instruction. Arguments are passed
 * in registers r0-r5 with the syscall number in r7. The return value is in r0.
 */
#pragma once

#include "core/types/primitives.h"
class System
{
public:

	// Syscall with 0 arguments
	static NOINLINE SSIZE Call(USIZE number)
	{
		register USIZE r0 __asm__("r0");
		register USIZE r7 __asm__("r7") = number;
		__asm__ volatile(
			"svc #0\n"
			: "=r"(r0)
			: "r"(r7)
			: "memory", "cc"
		);
		return (SSIZE)r0;
	}

	// Syscall with 1 argument
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1)
	{
		register USIZE r0 __asm__("r0") = arg1;
		register USIZE r7 __asm__("r7") = number;
		__asm__ volatile(
			"svc #0\n"
			: "+r"(r0)
			: "r"(r7)
			: "memory", "cc"
		);
		return (SSIZE)r0;
	}

	// Syscall with 2 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2)
	{
		register USIZE r0 __asm__("r0") = arg1;
		register USIZE r1 __asm__("r1") = arg2;
		register USIZE r7 __asm__("r7") = number;
		__asm__ volatile(
			"svc #0\n"
			: "+r"(r0)
			: "r"(r1), "r"(r7)
			: "memory", "cc"
		);
		return (SSIZE)r0;
	}

	// Syscall with 3 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3)
	{
		register USIZE r0 __asm__("r0") = arg1;
		register USIZE r1 __asm__("r1") = arg2;
		register USIZE r2 __asm__("r2") = arg3;
		register USIZE r7 __asm__("r7") = number;
		__asm__ volatile(
			"svc #0\n"
			: "+r"(r0)
			: "r"(r1), "r"(r2), "r"(r7)
			: "memory", "cc"
		);
		return (SSIZE)r0;
	}

	// Syscall with 4 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4)
	{
		register USIZE r0 __asm__("r0") = arg1;
		register USIZE r1 __asm__("r1") = arg2;
		register USIZE r2 __asm__("r2") = arg3;
		register USIZE r3 __asm__("r3") = arg4;
		register USIZE r7 __asm__("r7") = number;
		__asm__ volatile(
			"svc #0\n"
			: "+r"(r0)
			: "r"(r1), "r"(r2), "r"(r3), "r"(r7)
			: "memory", "cc"
		);
		return (SSIZE)r0;
	}

	// Syscall with 5 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5)
	{
		register USIZE r0 __asm__("r0") = arg1;
		register USIZE r1 __asm__("r1") = arg2;
		register USIZE r2 __asm__("r2") = arg3;
		register USIZE r3 __asm__("r3") = arg4;
		register USIZE r4 __asm__("r4") = arg5;
		register USIZE r7 __asm__("r7") = number;
		__asm__ volatile(
			"svc #0\n"
			: "+r"(r0)
			: "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r7)
			: "memory", "cc"
		);
		return (SSIZE)r0;
	}

	// Syscall with 6 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5, USIZE arg6)
	{
		register USIZE r0 __asm__("r0") = arg1;
		register USIZE r1 __asm__("r1") = arg2;
		register USIZE r2 __asm__("r2") = arg3;
		register USIZE r3 __asm__("r3") = arg4;
		register USIZE r4 __asm__("r4") = arg5;
		register USIZE r5 __asm__("r5") = arg6;
		register USIZE r7 __asm__("r7") = number;
		__asm__ volatile(
			"svc #0\n"
			: "+r"(r0)
			: "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5), "r"(r7)
			: "memory", "cc"
		);
		return (SSIZE)r0;
	}

};  // class System

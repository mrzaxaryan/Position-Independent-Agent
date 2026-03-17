/**
 * @file system.x86_64.h
 * @brief x86_64 FreeBSD syscall implementation via inline assembly.
 *
 * @details Provides System::Call overloads (0-6 arguments) that invoke FreeBSD
 * syscalls using the x86_64 syscall instruction. Uses the same register
 * convention as Linux (rdi, rsi, rdx, r10, r8, r9; number in rax), but FreeBSD
 * signals errors via the carry flag with a positive errno in rax. The inline
 * assembly negates rax on error to normalize to the negative-return convention.
 * RDX is clobbered by the kernel's rval[1] secondary return value.
 */
#pragma once

#include "core/types/primitives.h"

class System
{
public:

	// Syscall with 0 arguments
	// Note: FreeBSD kernel writes rval[1] to RDX on return, so RDX must be clobbered.
	// NOINLINE: prevents the syscall asm from being inlined into complex callers
	// where register allocation at -O1+ with LTO can produce incorrect code.
	static NOINLINE SSIZE Call(USIZE number)
	{
		register USIZE r_rax __asm__("rax") = number;
		__asm__ volatile(
			"syscall\n"
			"jnc 1f\n"
			"negq %%rax\n"
			"1:\n"
			: "+r"(r_rax)
			:
			: "rcx", "rdx", "r11", "memory", "cc"
		);
		return (SSIZE)r_rax;
	}

	// Syscall with 1 argument
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1)
	{
		register USIZE r_rdi __asm__("rdi") = arg1;
		register USIZE r_rax __asm__("rax") = number;
		__asm__ volatile(
			"syscall\n"
			"jnc 1f\n"
			"negq %%rax\n"
			"1:\n"
			: "+r"(r_rax)
			: "r"(r_rdi)
			: "rcx", "rdx", "r11", "memory", "cc"
		);
		return (SSIZE)r_rax;
	}

	// Syscall with 2 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2)
	{
		register USIZE r_rdi __asm__("rdi") = arg1;
		register USIZE r_rsi __asm__("rsi") = arg2;
		register USIZE r_rax __asm__("rax") = number;
		__asm__ volatile(
			"syscall\n"
			"jnc 1f\n"
			"negq %%rax\n"
			"1:\n"
			: "+r"(r_rax)
			: "r"(r_rdi), "r"(r_rsi)
			: "rcx", "rdx", "r11", "memory", "cc"
		);
		return (SSIZE)r_rax;
	}

	// Syscall with 3 arguments
	// RDX is used as input and clobbered by kernel (rval[1]), so mark as "+r".
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3)
	{
		register USIZE r_rdi __asm__("rdi") = arg1;
		register USIZE r_rsi __asm__("rsi") = arg2;
		register USIZE r_rdx __asm__("rdx") = arg3;
		register USIZE r_rax __asm__("rax") = number;
		__asm__ volatile(
			"syscall\n"
			"jnc 1f\n"
			"negq %%rax\n"
			"1:\n"
			: "+r"(r_rax), "+r"(r_rdx)
			: "r"(r_rdi), "r"(r_rsi)
			: "rcx", "r11", "memory", "cc"
		);
		return (SSIZE)r_rax;
	}

	// Syscall with 4 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4)
	{
		register USIZE r_rdi __asm__("rdi") = arg1;
		register USIZE r_rsi __asm__("rsi") = arg2;
		register USIZE r_rdx __asm__("rdx") = arg3;
		register USIZE r_r10 __asm__("r10") = arg4;
		register USIZE r_rax __asm__("rax") = number;
		__asm__ volatile(
			"syscall\n"
			"jnc 1f\n"
			"negq %%rax\n"
			"1:\n"
			: "+r"(r_rax), "+r"(r_rdx)
			: "r"(r_rdi), "r"(r_rsi), "r"(r_r10)
			: "rcx", "r11", "memory", "cc"
		);
		return (SSIZE)r_rax;
	}

	// Syscall with 5 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5)
	{
		register USIZE r_rdi __asm__("rdi") = arg1;
		register USIZE r_rsi __asm__("rsi") = arg2;
		register USIZE r_rdx __asm__("rdx") = arg3;
		register USIZE r_r10 __asm__("r10") = arg4;
		register USIZE r_r8 __asm__("r8") = arg5;
		register USIZE r_rax __asm__("rax") = number;
		__asm__ volatile(
			"syscall\n"
			"jnc 1f\n"
			"negq %%rax\n"
			"1:\n"
			: "+r"(r_rax), "+r"(r_rdx)
			: "r"(r_rdi), "r"(r_rsi), "r"(r_r10), "r"(r_r8)
			: "rcx", "r11", "memory", "cc"
		);
		return (SSIZE)r_rax;
	}

	// Syscall with 6 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5, USIZE arg6)
	{
		register USIZE r_rdi __asm__("rdi") = arg1;
		register USIZE r_rsi __asm__("rsi") = arg2;
		register USIZE r_rdx __asm__("rdx") = arg3;
		register USIZE r_r10 __asm__("r10") = arg4;
		register USIZE r_r8 __asm__("r8") = arg5;
		register USIZE r_r9 __asm__("r9") = arg6;
		register USIZE r_rax __asm__("rax") = number;
		__asm__ volatile(
			"syscall\n"
			"jnc 1f\n"
			"negq %%rax\n"
			"1:\n"
			: "+r"(r_rax), "+r"(r_rdx)
			: "r"(r_rdi), "r"(r_rsi), "r"(r_r10), "r"(r_r8), "r"(r_r9)
			: "rcx", "r11", "memory", "cc"
		);
		return (SSIZE)r_rax;
	}

};  // class System

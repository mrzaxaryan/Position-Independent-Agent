/**
 * @file system.i386.h
 * @brief i386 FreeBSD syscall implementation via inline assembly.
 *
 * @details Provides System::Call overloads (0-6 arguments) that invoke FreeBSD
 * syscalls using the int $0x80 trap gate. Unlike Linux i386 (which passes
 * arguments in registers via int $0x80), FreeBSD passes arguments on the stack
 * with a dummy return address at ESP. The syscall number is in EAX, and the
 * carry flag indicates error (positive errno in EAX). The inline assembly
 * negates eax on error to normalize to the negative-return convention.
 */
#pragma once

#include "core/types/primitives.h"

class System
{
public:

	// All arguments are loaded into registers first (via "r" constraints) to
	// avoid ESP-relative operand issues when the stack pointer is modified.

	// Syscall with 0 arguments
	static NOINLINE SSIZE Call(USIZE number)
	{
		SSIZE ret;
		__asm__ volatile(
			"pushl $0\n"
			"int $0x80\n"
			"jnc 1f\n"
			"negl %%eax\n"
			"1:\n"
			"addl $4, %%esp\n"
			: "=a"(ret)
			: "a"(number)
			: "memory", "cc"
		);
		return ret;
	}

	// Syscall with 1 argument
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1)
	{
		SSIZE ret;
		register USIZE r_ebx __asm__("ebx") = arg1;
		__asm__ volatile(
			"pushl %%ebx\n"
			"pushl $0\n"
			"int $0x80\n"
			"jnc 1f\n"
			"negl %%eax\n"
			"1:\n"
			"addl $8, %%esp\n"
			: "=a"(ret)
			: "a"(number), "r"(r_ebx)
			: "memory", "cc"
		);
		return ret;
	}

	// Syscall with 2 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2)
	{
		SSIZE ret;
		register USIZE r_ebx __asm__("ebx") = arg1;
		register USIZE r_ecx __asm__("ecx") = arg2;
		__asm__ volatile(
			"pushl %%ecx\n"
			"pushl %%ebx\n"
			"pushl $0\n"
			"int $0x80\n"
			"jnc 1f\n"
			"negl %%eax\n"
			"1:\n"
			"addl $12, %%esp\n"
			: "=a"(ret)
			: "a"(number), "r"(r_ebx), "r"(r_ecx)
			: "memory", "cc"
		);
		return ret;
	}

	// Syscall with 3 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3)
	{
		SSIZE ret;
		register USIZE r_ebx __asm__("ebx") = arg1;
		register USIZE r_ecx __asm__("ecx") = arg2;
		register USIZE r_edx __asm__("edx") = arg3;
		__asm__ volatile(
			"pushl %%edx\n"
			"pushl %%ecx\n"
			"pushl %%ebx\n"
			"pushl $0\n"
			"int $0x80\n"
			"jnc 1f\n"
			"negl %%eax\n"
			"1:\n"
			"addl $16, %%esp\n"
			: "=a"(ret)
			: "a"(number), "r"(r_ebx), "r"(r_ecx), "r"(r_edx)
			: "memory", "cc"
		);
		return ret;
	}

	// Syscall with 4 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4)
	{
		SSIZE ret;
		register USIZE r_ebx __asm__("ebx") = arg1;
		register USIZE r_ecx __asm__("ecx") = arg2;
		register USIZE r_edx __asm__("edx") = arg3;
		register USIZE r_esi __asm__("esi") = arg4;
		__asm__ volatile(
			"pushl %%esi\n"
			"pushl %%edx\n"
			"pushl %%ecx\n"
			"pushl %%ebx\n"
			"pushl $0\n"
			"int $0x80\n"
			"jnc 1f\n"
			"negl %%eax\n"
			"1:\n"
			"addl $20, %%esp\n"
			: "=a"(ret)
			: "a"(number), "r"(r_ebx), "r"(r_ecx), "r"(r_edx), "r"(r_esi)
			: "memory", "cc"
		);
		return ret;
	}

	// Syscall with 5 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5)
	{
		SSIZE ret;
		register USIZE r_ebx __asm__("ebx") = arg1;
		register USIZE r_ecx __asm__("ecx") = arg2;
		register USIZE r_edx __asm__("edx") = arg3;
		register USIZE r_esi __asm__("esi") = arg4;
		register USIZE r_edi __asm__("edi") = arg5;
		__asm__ volatile(
			"pushl %%edi\n"
			"pushl %%esi\n"
			"pushl %%edx\n"
			"pushl %%ecx\n"
			"pushl %%ebx\n"
			"pushl $0\n"
			"int $0x80\n"
			"jnc 1f\n"
			"negl %%eax\n"
			"1:\n"
			"addl $24, %%esp\n"
			: "=a"(ret)
			: "a"(number), "r"(r_ebx), "r"(r_ecx), "r"(r_edx), "r"(r_esi), "r"(r_edi)
			: "memory", "cc"
		);
		return ret;
	}

	// Syscall with 6 arguments
	// Cannot use register __asm__("ebp") for arg6 — it conflicts with the
	// frame pointer at -O1+ under LTO. Push arg6 from "g" constraint first
	// (ESP is still valid at that point), then push remaining from registers.
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5, USIZE arg6)
	{
		SSIZE ret;
		register USIZE r_ebx __asm__("ebx") = arg1;
		register USIZE r_ecx __asm__("ecx") = arg2;
		register USIZE r_edx __asm__("edx") = arg3;
		register USIZE r_esi __asm__("esi") = arg4;
		register USIZE r_edi __asm__("edi") = arg5;
		__asm__ volatile(
			"pushl %[a6]\n"
			"pushl %%edi\n"
			"pushl %%esi\n"
			"pushl %%edx\n"
			"pushl %%ecx\n"
			"pushl %%ebx\n"
			"pushl $0\n"
			"int $0x80\n"
			"jnc 1f\n"
			"negl %%eax\n"
			"1:\n"
			"addl $28, %%esp\n"
			: "=a"(ret)
			: "a"(number), "r"(r_ebx), "r"(r_ecx), "r"(r_edx), "r"(r_esi), "r"(r_edi), [a6] "g"(arg6)
			: "memory", "cc"
		);
		return ret;
	}

};  // class System

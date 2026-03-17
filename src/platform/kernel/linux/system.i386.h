/**
 * @file system.i386.h
 * @brief i386 Linux syscall implementation via inline assembly.
 *
 * @details Provides System::Call overloads (0-6 arguments) that invoke Linux
 * syscalls using the int $0x80 software interrupt. Arguments are passed in
 * registers ebx, ecx, edx, esi, edi with the syscall number in eax. The
 * 6-argument variant manually saves/restores ebp to avoid frame pointer
 * conflicts under LTO at higher optimization levels.
 */
#pragma once

#include "core/types/primitives.h"
class System
{
public:

	// Syscall with 0 arguments
	static NOINLINE SSIZE Call(USIZE number)
	{
		register USIZE r_eax __asm__("eax") = number;
		__asm__ volatile(
			"int $0x80\n"
			: "+r"(r_eax)
			:
			: "memory"
		);
		return (SSIZE)r_eax;
	}

	// Syscall with 1 argument
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1)
	{
		register USIZE r_ebx __asm__("ebx") = arg1;
		register USIZE r_eax __asm__("eax") = number;
		__asm__ volatile(
			"int $0x80\n"
			: "+r"(r_eax)
			: "r"(r_ebx)
			: "memory"
		);
		return (SSIZE)r_eax;
	}

	// Syscall with 2 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2)
	{
		register USIZE r_ebx __asm__("ebx") = arg1;
		register USIZE r_ecx __asm__("ecx") = arg2;
		register USIZE r_eax __asm__("eax") = number;
		__asm__ volatile(
			"int $0x80\n"
			: "+r"(r_eax)
			: "r"(r_ebx), "r"(r_ecx)
			: "memory"
		);
		return (SSIZE)r_eax;
	}

	// Syscall with 3 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3)
	{
		register USIZE r_ebx __asm__("ebx") = arg1;
		register USIZE r_ecx __asm__("ecx") = arg2;
		register USIZE r_edx __asm__("edx") = arg3;
		register USIZE r_eax __asm__("eax") = number;
		__asm__ volatile(
			"int $0x80\n"
			: "+r"(r_eax)
			: "r"(r_ebx), "r"(r_ecx), "r"(r_edx)
			: "memory"
		);
		return (SSIZE)r_eax;
	}

	// Syscall with 4 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4)
	{
		register USIZE r_ebx __asm__("ebx") = arg1;
		register USIZE r_ecx __asm__("ecx") = arg2;
		register USIZE r_edx __asm__("edx") = arg3;
		register USIZE r_esi __asm__("esi") = arg4;
		register USIZE r_eax __asm__("eax") = number;
		__asm__ volatile(
			"int $0x80\n"
			: "+r"(r_eax)
			: "r"(r_ebx), "r"(r_ecx), "r"(r_edx), "r"(r_esi)
			: "memory"
		);
		return (SSIZE)r_eax;
	}

	// Syscall with 5 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5)
	{
		register USIZE r_ebx __asm__("ebx") = arg1;
		register USIZE r_ecx __asm__("ecx") = arg2;
		register USIZE r_edx __asm__("edx") = arg3;
		register USIZE r_esi __asm__("esi") = arg4;
		register USIZE r_edi __asm__("edi") = arg5;
		register USIZE r_eax __asm__("eax") = number;
		__asm__ volatile(
			"int $0x80\n"
			: "+r"(r_eax)
			: "r"(r_ebx), "r"(r_ecx), "r"(r_edx), "r"(r_esi), "r"(r_edi)
			: "memory"
		);
		return (SSIZE)r_eax;
	}

	// Syscall with 6 arguments
	// Cannot use register __asm__("ebp") for arg6 — it conflicts with the
	// frame pointer at -O1+ under LTO. Save/restore EBP manually instead.
	// Use "rm" (not "r") for arg6: at -O0 with -fno-omit-frame-pointer, all 6
	// GPRs (eax-edi) are already tied to other operands, leaving no register
	// for arg6. "rm" lets the compiler use a memory operand (stack slot).
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5, USIZE arg6)
	{
		SSIZE ret;
		register USIZE r_ebx __asm__("ebx") = arg1;
		register USIZE r_ecx __asm__("ecx") = arg2;
		register USIZE r_edx __asm__("edx") = arg3;
		register USIZE r_esi __asm__("esi") = arg4;
		register USIZE r_edi __asm__("edi") = arg5;
		__asm__ volatile(
			"pushl %%ebp\n"
			"movl %[a6], %%ebp\n"
			"int $0x80\n"
			"popl %%ebp\n"
			: "=a"(ret)
			: "a"(number), "r"(r_ebx), "r"(r_ecx), "r"(r_edx), "r"(r_esi), "r"(r_edi), [a6] "rm"(arg6)
			: "memory"
		);
		return ret;
	}

};  // class System

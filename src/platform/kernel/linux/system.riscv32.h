/**
 * @file system.riscv32.h
 * @brief RISC-V 32-bit Linux syscall implementation via inline assembly.
 *
 * @details Provides System::Call overloads (0-6 arguments) that invoke Linux
 * syscalls using the ecall instruction. Arguments are passed in registers
 * a0-a5 (x10-x15) with the syscall number in a7 (x17). The return value
 * is in a0 (x10). Same register convention as RV64 but with 32-bit registers.
 */
#pragma once

#include "core/types/primitives.h"
class System
{
public:

	// Syscall with 0 arguments
	static NOINLINE SSIZE Call(USIZE number)
	{
		register USIZE a0 __asm__("a0");
		register USIZE a7 __asm__("a7") = number;
		__asm__ volatile(
			"ecall\n"
			: "=r"(a0)
			: "r"(a7)
			: "memory"
		);
		return (SSIZE)a0;
	}

	// Syscall with 1 argument
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1)
	{
		register USIZE a0 __asm__("a0") = arg1;
		register USIZE a7 __asm__("a7") = number;
		__asm__ volatile(
			"ecall\n"
			: "+r"(a0)
			: "r"(a7)
			: "memory"
		);
		return (SSIZE)a0;
	}

	// Syscall with 2 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2)
	{
		register USIZE a0 __asm__("a0") = arg1;
		register USIZE a1 __asm__("a1") = arg2;
		register USIZE a7 __asm__("a7") = number;
		__asm__ volatile(
			"ecall\n"
			: "+r"(a0)
			: "r"(a1), "r"(a7)
			: "memory"
		);
		return (SSIZE)a0;
	}

	// Syscall with 3 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3)
	{
		register USIZE a0 __asm__("a0") = arg1;
		register USIZE a1 __asm__("a1") = arg2;
		register USIZE a2 __asm__("a2") = arg3;
		register USIZE a7 __asm__("a7") = number;
		__asm__ volatile(
			"ecall\n"
			: "+r"(a0)
			: "r"(a1), "r"(a2), "r"(a7)
			: "memory"
		);
		return (SSIZE)a0;
	}

	// Syscall with 4 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4)
	{
		register USIZE a0 __asm__("a0") = arg1;
		register USIZE a1 __asm__("a1") = arg2;
		register USIZE a2 __asm__("a2") = arg3;
		register USIZE a3 __asm__("a3") = arg4;
		register USIZE a7 __asm__("a7") = number;
		__asm__ volatile(
			"ecall\n"
			: "+r"(a0)
			: "r"(a1), "r"(a2), "r"(a3), "r"(a7)
			: "memory"
		);
		return (SSIZE)a0;
	}

	// Syscall with 5 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5)
	{
		register USIZE a0 __asm__("a0") = arg1;
		register USIZE a1 __asm__("a1") = arg2;
		register USIZE a2 __asm__("a2") = arg3;
		register USIZE a3 __asm__("a3") = arg4;
		register USIZE a4 __asm__("a4") = arg5;
		register USIZE a7 __asm__("a7") = number;
		__asm__ volatile(
			"ecall\n"
			: "+r"(a0)
			: "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a7)
			: "memory"
		);
		return (SSIZE)a0;
	}

	// Syscall with 6 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5, USIZE arg6)
	{
		register USIZE a0 __asm__("a0") = arg1;
		register USIZE a1 __asm__("a1") = arg2;
		register USIZE a2 __asm__("a2") = arg3;
		register USIZE a3 __asm__("a3") = arg4;
		register USIZE a4 __asm__("a4") = arg5;
		register USIZE a5 __asm__("a5") = arg6;
		register USIZE a7 __asm__("a7") = number;
		__asm__ volatile(
			"ecall\n"
			: "+r"(a0)
			: "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a7)
			: "memory"
		);
		return (SSIZE)a0;
	}

};  // class System

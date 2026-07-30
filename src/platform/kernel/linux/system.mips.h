/**
 * @file system.mips.h
 * @brief MIPS o32 (MIPS32, 32-bit LE) Linux syscall implementation via inline asm.
 *
 * @details Provides System::Call overloads (0-6 arguments) for the MIPS o32 ABI.
 * The syscall number is in $v0 ($2), args 1-4 in $a0-$a3 ($4-$7), and the return
 * value in $v0 ($2).
 *
 * As with MIPS64, MIPS Linux uses $a3 ($7) as an out-of-band error indicator:
 * $a3=0 success, $a3!=0 error (with $v0 holding the positive errno). The asm
 * checks $a3 and negates $v0 on error to normalize to the negative-return
 * convention expected by result::FromLinux(). The branch-delay slot after
 * `beqz` is a mandatory `nop`.
 *
 * o32 vs n64: o32 passes ONLY 4 args in registers. Args 5 and 6 are passed on
 * the user stack at 16($sp) and 20($sp), which the kernel's o32 syscall path
 * reads from user memory. The 5/6-arg overloads therefore reserve a 32-byte
 * stack scratch, store arg5/arg6 there, then restore $sp around the syscall.
 * Overloads 0-4 are identical to the n64 versions (args 1-4 all fit in
 * $a0-$a3; the 4-arg overload reuses $a3 for arg4 with double-duty as the
 * error-flag output).
 *
 * All overloads are NOINLINE to prevent LTO miscompilation of the
 * error-indicator asm pattern.
 *
 * Clobbers: $1 (at), $3 (v1), $10-$15 (t2-t7), $24-$25 (t8-t9), hi, lo.
 * ($8/$9 are input operands for the staged stack args, not clobbered.)
 *
 * @see Linux kernel arch/mips/include/asm/unistd.h (O32 ABI)
 */
#pragma once

#include "core/types/primitives.h"

class System
{
public:

	// Syscall with 0 arguments
	static NOINLINE SSIZE Call(USIZE number)
	{
		register USIZE v0 __asm__("$2") = number;
		register USIZE a3 __asm__("$7");
		__asm__ volatile(
			"syscall\n"
			"beqz $7, 1f\n"
			"nop\n"
			"negu $2, $2\n"
			"1:\n"
			: "+r"(v0), "=r"(a3)
			:
			: "$1", "$3", "$10", "$11", "$12", "$13", "$14", "$15",
			  "$24", "$25", "hi", "lo", "memory"
		);
		return (SSIZE)v0;
	}

	// Syscall with 1 argument
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1)
	{
		register USIZE v0 __asm__("$2") = number;
		register USIZE a0 __asm__("$4") = arg1;
		register USIZE a3 __asm__("$7");
		__asm__ volatile(
			"syscall\n"
			"beqz $7, 1f\n"
			"nop\n"
			"negu $2, $2\n"
			"1:\n"
			: "+r"(v0), "=r"(a3)
			: "r"(a0)
			: "$1", "$3", "$10", "$11", "$12", "$13", "$14", "$15",
			  "$24", "$25", "hi", "lo", "memory"
		);
		return (SSIZE)v0;
	}

	// Syscall with 2 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2)
	{
		register USIZE v0 __asm__("$2") = number;
		register USIZE a0 __asm__("$4") = arg1;
		register USIZE a1 __asm__("$5") = arg2;
		register USIZE a3 __asm__("$7");
		__asm__ volatile(
			"syscall\n"
			"beqz $7, 1f\n"
			"nop\n"
			"negu $2, $2\n"
			"1:\n"
			: "+r"(v0), "=r"(a3)
			: "r"(a0), "r"(a1)
			: "$1", "$3", "$10", "$11", "$12", "$13", "$14", "$15",
			  "$24", "$25", "hi", "lo", "memory"
		);
		return (SSIZE)v0;
	}

	// Syscall with 3 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3)
	{
		register USIZE v0 __asm__("$2") = number;
		register USIZE a0 __asm__("$4") = arg1;
		register USIZE a1 __asm__("$5") = arg2;
		register USIZE a2 __asm__("$6") = arg3;
		register USIZE a3 __asm__("$7");
		__asm__ volatile(
			"syscall\n"
			"beqz $7, 1f\n"
			"nop\n"
			"negu $2, $2\n"
			"1:\n"
			: "+r"(v0), "=r"(a3)
			: "r"(a0), "r"(a1), "r"(a2)
			: "$1", "$3", "$10", "$11", "$12", "$13", "$14", "$15",
			  "$24", "$25", "hi", "lo", "memory"
		);
		return (SSIZE)v0;
	}

	// Syscall with 4 arguments (arg4 reuses $a3/$7 as input, then the kernel
	// writes the error flag there — same double-duty pattern as n64).
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4)
	{
		register USIZE v0 __asm__("$2") = number;
		register USIZE a0 __asm__("$4") = arg1;
		register USIZE a1 __asm__("$5") = arg2;
		register USIZE a2 __asm__("$6") = arg3;
		register USIZE a3 __asm__("$7") = arg4;
		__asm__ volatile(
			"syscall\n"
			"beqz $7, 1f\n"
			"nop\n"
			"negu $2, $2\n"
			"1:\n"
			: "+r"(v0), "+r"(a3)
			: "r"(a0), "r"(a1), "r"(a2)
			: "$1", "$3", "$10", "$11", "$12", "$13", "$14", "$15",
			  "$24", "$25", "hi", "lo", "memory"
		);
		return (SSIZE)v0;
	}

	// Syscall with 5 arguments. o32 passes args 5+ on the user stack: arg5 at
	// 16($sp). Reserve a 32-byte scratch, stage arg5, syscall, restore $sp.
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5)
	{
		register USIZE v0 __asm__("$2") = number;
		register USIZE a0 __asm__("$4") = arg1;
		register USIZE a1 __asm__("$5") = arg2;
		register USIZE a2 __asm__("$6") = arg3;
		register USIZE a3 __asm__("$7") = arg4;
		register USIZE a5 __asm__("$8") = arg5;
		__asm__ volatile(
			"addiu $sp, $sp, -32\n\t"
			"sw $8, 16($sp)\n\t"
			"syscall\n\t"
			"addiu $sp, $sp, 32\n\t"
			"beqz $7, 1f\n\t"
			"nop\n\t"
			"negu $2, $2\n\t"
			"1:"
			: "+r"(v0), "+r"(a3)
			: "r"(a0), "r"(a1), "r"(a2), "r"(a5)
			: "$1", "$3", "$10", "$11", "$12", "$13", "$14", "$15",
			  "$24", "$25", "hi", "lo", "memory"
		);
		return (SSIZE)v0;
	}

	// Syscall with 6 arguments. o32: arg5 at 16($sp), arg6 at 20($sp).
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5, USIZE arg6)
	{
		register USIZE v0 __asm__("$2") = number;
		register USIZE a0 __asm__("$4") = arg1;
		register USIZE a1 __asm__("$5") = arg2;
		register USIZE a2 __asm__("$6") = arg3;
		register USIZE a3 __asm__("$7") = arg4;
		register USIZE a5 __asm__("$8") = arg5;
		register USIZE a6 __asm__("$9") = arg6;
		__asm__ volatile(
			"addiu $sp, $sp, -32\n\t"
			"sw $8, 16($sp)\n\t"
			"sw $9, 20($sp)\n\t"
			"syscall\n\t"
			"addiu $sp, $sp, 32\n\t"
			"beqz $7, 1f\n\t"
			"nop\n\t"
			"negu $2, $2\n\t"
			"1:"
			: "+r"(v0), "+r"(a3)
			: "r"(a0), "r"(a1), "r"(a2), "r"(a5), "r"(a6)
			: "$1", "$3", "$10", "$11", "$12", "$13", "$14", "$15",
			  "$24", "$25", "hi", "lo", "memory"
		);
		return (SSIZE)v0;
	}

};  // class System

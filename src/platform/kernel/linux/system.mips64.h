/**
 * @file system.mips64.h
 * @brief MIPS64 Linux syscall implementation via inline assembly (n64 ABI).
 *
 * @details Provides System::Call overloads (0-6 arguments) that invoke Linux
 * syscalls using the MIPS syscall instruction. The syscall number is passed
 * in $v0 ($2), arguments in $a0-$a5 ($4-$9), and the return value is in
 * $v0 ($2).
 *
 * Unlike most Linux architectures (which return negative errno in the return
 * register), MIPS Linux uses $a3 ($7) as an out-of-band error indicator:
 * $a3=0 on success, $a3!=0 on error (with $v0 holding the positive errno).
 * This is analogous to the BSD carry-flag convention. The inline assembly
 * checks $a3 and negates $v0 on error to normalize to the negative-return
 * convention expected by result::FromLinux().
 *
 * All overloads are NOINLINE to prevent LTO miscompilation of the
 * error-indicator asm pattern (same rationale as FreeBSD syscall wrappers).
 *
 * The syscall instruction clobbers: $1 (at), $3 (v1), $10-$15 (t2-t7),
 * $24-$25 (t8-t9), hi, and lo registers.
 *
 * @see Linux kernel arch/mips/include/asm/unistd.h
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

	// Syscall with 4 arguments
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

	// Syscall with 5 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5)
	{
		register USIZE v0 __asm__("$2") = number;
		register USIZE a0 __asm__("$4") = arg1;
		register USIZE a1 __asm__("$5") = arg2;
		register USIZE a2 __asm__("$6") = arg3;
		register USIZE a3 __asm__("$7") = arg4;
		register USIZE a4 __asm__("$8") = arg5;
		__asm__ volatile(
			"syscall\n"
			"beqz $7, 1f\n"
			"nop\n"
			"negu $2, $2\n"
			"1:\n"
			: "+r"(v0), "+r"(a3)
			: "r"(a0), "r"(a1), "r"(a2), "r"(a4)
			: "$1", "$3", "$10", "$11", "$12", "$13", "$14", "$15",
			  "$24", "$25", "hi", "lo", "memory"
		);
		return (SSIZE)v0;
	}

	// Syscall with 6 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3, USIZE arg4, USIZE arg5, USIZE arg6)
	{
		register USIZE v0 __asm__("$2") = number;
		register USIZE a0 __asm__("$4") = arg1;
		register USIZE a1 __asm__("$5") = arg2;
		register USIZE a2 __asm__("$6") = arg3;
		register USIZE a3 __asm__("$7") = arg4;
		register USIZE a4 __asm__("$8") = arg5;
		register USIZE a5 __asm__("$9") = arg6;
		__asm__ volatile(
			"syscall\n"
			"beqz $7, 1f\n"
			"nop\n"
			"negu $2, $2\n"
			"1:\n"
			: "+r"(v0), "+r"(a3)
			: "r"(a0), "r"(a1), "r"(a2), "r"(a4), "r"(a5)
			: "$1", "$3", "$10", "$11", "$12", "$13", "$14", "$15",
			  "$24", "$25", "hi", "lo", "memory"
		);
		return (SSIZE)v0;
	}

};  // class System

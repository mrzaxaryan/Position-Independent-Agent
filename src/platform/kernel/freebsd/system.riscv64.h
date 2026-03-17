/**
 * @file system.riscv64.h
 * @brief RISC-V 64-bit FreeBSD syscall implementation via inline assembly.
 *
 * @details Provides System::Call overloads (0-6 arguments) that invoke FreeBSD
 * syscalls using the ecall instruction. Unlike Linux (which uses a7 for the
 * syscall number), FreeBSD RISC-V places the syscall number in t0 (x5).
 * Arguments are passed in a0-a5 (x10-x15). After ecall, t0 serves as the
 * error indicator: t0=0 on success, t0!=0 on error (a0 contains the positive
 * errno). The inline assembly negates a0 on error to normalize to the
 * negative-return convention used by the platform result conversion layer.
 *
 * The syscall number is loaded into t0 explicitly inside the asm block
 * (via "mv t0, %N") rather than relying on register variable bindings,
 * which the LLVM LTO backend may not honour for caller-saved temporaries.
 * All argument-register outputs use the early-clobber "&" modifier to
 * prevent the compiler from assigning the "r"(number) input to the same
 * physical register as a "+r" output — without "&", the compiler may
 * overlap them, putting arg1 into t0 instead of the syscall number.
 *
 * @see FreeBSD libsys RISC-V SYS.h
 *      https://github.com/freebsd/freebsd-src/blob/main/lib/libsys/riscv/SYS.h
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
		__asm__ volatile(
			"mv t0, %1\n"
			"ecall\n"
			"beqz t0, 1f\n"
			"neg a0, a0\n"
			"1:\n"
			: "=&r"(a0)
			: "r"(number)
			: "t0", "a1", "memory"
		);
		return (SSIZE)a0;
	}

	// Syscall with 1 argument
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1)
	{
		register USIZE a0 __asm__("a0") = arg1;
		__asm__ volatile(
			"mv t0, %1\n"
			"ecall\n"
			"beqz t0, 1f\n"
			"neg a0, a0\n"
			"1:\n"
			: "+&r"(a0)
			: "r"(number)
			: "t0", "a1", "memory"
		);
		return (SSIZE)a0;
	}

	// Syscall with 2 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2)
	{
		register USIZE a0 __asm__("a0") = arg1;
		register USIZE a1 __asm__("a1") = arg2;
		__asm__ volatile(
			"mv t0, %2\n"
			"ecall\n"
			"beqz t0, 1f\n"
			"neg a0, a0\n"
			"1:\n"
			: "+&r"(a0), "+&r"(a1)
			: "r"(number)
			: "t0", "memory"
		);
		return (SSIZE)a0;
	}

	// Syscall with 3 arguments
	static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3)
	{
		register USIZE a0 __asm__("a0") = arg1;
		register USIZE a1 __asm__("a1") = arg2;
		register USIZE a2 __asm__("a2") = arg3;
		__asm__ volatile(
			"mv t0, %3\n"
			"ecall\n"
			"beqz t0, 1f\n"
			"neg a0, a0\n"
			"1:\n"
			: "+&r"(a0), "+&r"(a1), "+&r"(a2)
			: "r"(number)
			: "t0", "memory"
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
		__asm__ volatile(
			"mv t0, %4\n"
			"ecall\n"
			"beqz t0, 1f\n"
			"neg a0, a0\n"
			"1:\n"
			: "+&r"(a0), "+&r"(a1), "+&r"(a2), "+&r"(a3)
			: "r"(number)
			: "t0", "memory"
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
		__asm__ volatile(
			"mv t0, %5\n"
			"ecall\n"
			"beqz t0, 1f\n"
			"neg a0, a0\n"
			"1:\n"
			: "+&r"(a0), "+&r"(a1), "+&r"(a2), "+&r"(a3), "+&r"(a4)
			: "r"(number)
			: "t0", "memory"
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
		__asm__ volatile(
			"mv t0, %6\n"
			"ecall\n"
			"beqz t0, 1f\n"
			"neg a0, a0\n"
			"1:\n"
			: "+&r"(a0), "+&r"(a1), "+&r"(a2), "+&r"(a3), "+&r"(a4), "+&r"(a5)
			: "r"(number)
			: "t0", "memory"
		);
		return (SSIZE)a0;
	}

};  // class System

#pragma once

/**
 * @file system.aarch64.h
 * @brief AArch64 Windows indirect syscall overloads — included inside class System { }.
 *
 * @details Provides System::Call overloads (0-14 arguments) that invoke Windows NT
 * syscalls via BLR to the resolved ntdll stub address. On ARM64, the kernel validates
 * that the SVC instruction originates from within ntdll (unlike x86_64/i386 where
 * indirect gadgets bypass ntdll). Arguments are passed in x0-x7 per AAPCS64, with
 * overflow arguments on the stack. x16 holds the ntdll stub address.
 *
 * @see Using Nt and Zw Versions of the Native System Services Routines
 *      https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/using-nt-and-zw-versions-of-the-native-system-services-routines
 */

	// Windows ARM64 indirect syscall via BLR to ntdll stub.
	//
	// On Windows ARM64, the syscall number is encoded in the SVC instruction's
	// immediate value (e.g., SVC #7 for syscall 7). The kernel extracts it from
	// ESR_EL1's ISS field. This is fundamentally different from Linux (x8 register)
	// and x64 (EAX register).
	//
	// ntdll stubs are just: SVC #N; RET
	// We set up args in x0-x7 (+ stack for 9+), then BLR to the stub.
	// The stub's SVC carries the correct syscall number in its immediate.
	//
	// x16 holds the stub address (intra-procedure-call scratch register).
	// x18 (TEB) is never touched. Callee-saved regs (x19-x28, x29) are safe
	// since the stub is only SVC+RET.

	// Indirect syscall with 0 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry)
	{
		register USIZE x0 __asm__("x0");
		register USIZE stub __asm__("x16") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"blr %[stub]\n"
			: "=r"(x0), [stub] "+r"(stub)
			:
			: "x1", "x2", "x3", "x4", "x5", "x6", "x7",
			  "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
			  "x17", "x30", "memory");
		return (NTSTATUS)x0;
	}

	// Indirect syscall with 1 argument
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1)
	{
		register USIZE x0 __asm__("x0") = a1;
		register USIZE stub __asm__("x16") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"blr %[stub]\n"
			: "+r"(x0), [stub] "+r"(stub)
			:
			: "x1", "x2", "x3", "x4", "x5", "x6", "x7",
			  "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
			  "x17", "x30", "memory");
		return (NTSTATUS)x0;
	}

	// Indirect syscall with 2 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2)
	{
		register USIZE x0 __asm__("x0") = a1;
		register USIZE x1 __asm__("x1") = a2;
		register USIZE stub __asm__("x16") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"blr %[stub]\n"
			: "+r"(x0), "+r"(x1), [stub] "+r"(stub)
			:
			: "x2", "x3", "x4", "x5", "x6", "x7",
			  "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
			  "x17", "x30", "memory");
		return (NTSTATUS)x0;
	}

	// Indirect syscall with 3 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3)
	{
		register USIZE x0 __asm__("x0") = a1;
		register USIZE x1 __asm__("x1") = a2;
		register USIZE x2 __asm__("x2") = a3;
		register USIZE stub __asm__("x16") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"blr %[stub]\n"
			: "+r"(x0), "+r"(x1), "+r"(x2), [stub] "+r"(stub)
			:
			: "x3", "x4", "x5", "x6", "x7",
			  "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
			  "x17", "x30", "memory");
		return (NTSTATUS)x0;
	}

	// Indirect syscall with 4 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4)
	{
		register USIZE x0 __asm__("x0") = a1;
		register USIZE x1 __asm__("x1") = a2;
		register USIZE x2 __asm__("x2") = a3;
		register USIZE x3 __asm__("x3") = a4;
		register USIZE stub __asm__("x16") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"blr %[stub]\n"
			: "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), [stub] "+r"(stub)
			:
			: "x4", "x5", "x6", "x7",
			  "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
			  "x17", "x30", "memory");
		return (NTSTATUS)x0;
	}

	// Indirect syscall with 5 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5)
	{
		register USIZE x0 __asm__("x0") = a1;
		register USIZE x1 __asm__("x1") = a2;
		register USIZE x2 __asm__("x2") = a3;
		register USIZE x3 __asm__("x3") = a4;
		register USIZE x4 __asm__("x4") = a5;
		register USIZE stub __asm__("x16") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"blr %[stub]\n"
			: "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), [stub] "+r"(stub)
			:
			: "x5", "x6", "x7",
			  "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
			  "x17", "x30", "memory");
		return (NTSTATUS)x0;
	}

	// Indirect syscall with 6 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6)
	{
		register USIZE x0 __asm__("x0") = a1;
		register USIZE x1 __asm__("x1") = a2;
		register USIZE x2 __asm__("x2") = a3;
		register USIZE x3 __asm__("x3") = a4;
		register USIZE x4 __asm__("x4") = a5;
		register USIZE x5 __asm__("x5") = a6;
		register USIZE stub __asm__("x16") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"blr %[stub]\n"
			: "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), [stub] "+r"(stub)
			:
			: "x6", "x7",
			  "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
			  "x17", "x30", "memory");
		return (NTSTATUS)x0;
	}

	// Indirect syscall with 7 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7)
	{
		register USIZE x0 __asm__("x0") = a1;
		register USIZE x1 __asm__("x1") = a2;
		register USIZE x2 __asm__("x2") = a3;
		register USIZE x3 __asm__("x3") = a4;
		register USIZE x4 __asm__("x4") = a5;
		register USIZE x5 __asm__("x5") = a6;
		register USIZE x6 __asm__("x6") = a7;
		register USIZE stub __asm__("x16") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"blr %[stub]\n"
			: "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), "+r"(x6), [stub] "+r"(stub)
			:
			: "x7",
			  "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
			  "x17", "x30", "memory");
		return (NTSTATUS)x0;
	}

	// Indirect syscall with 8 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8)
	{
		register USIZE x0 __asm__("x0") = a1;
		register USIZE x1 __asm__("x1") = a2;
		register USIZE x2 __asm__("x2") = a3;
		register USIZE x3 __asm__("x3") = a4;
		register USIZE x4 __asm__("x4") = a5;
		register USIZE x5 __asm__("x5") = a6;
		register USIZE x6 __asm__("x6") = a7;
		register USIZE x7 __asm__("x7") = a8;
		register USIZE stub __asm__("x16") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"blr %[stub]\n"
			: "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7), [stub] "+r"(stub)
			:
			: "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
			  "x17", "x30", "memory");
		return (NTSTATUS)x0;
	}

	// Indirect syscall with 9 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9)
	{
		register USIZE x0 __asm__("x0") = a1;
		register USIZE x1 __asm__("x1") = a2;
		register USIZE x2 __asm__("x2") = a3;
		register USIZE x3 __asm__("x3") = a4;
		register USIZE x4 __asm__("x4") = a5;
		register USIZE x5 __asm__("x5") = a6;
		register USIZE x6 __asm__("x6") = a7;
		register USIZE x7 __asm__("x7") = a8;
		register USIZE stub __asm__("x16") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"sub sp, sp, #16\n"
			"str %[a9], [sp]\n"
			"blr %[stub]\n"
			"add sp, sp, #16\n"
			: "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7), [stub] "+r"(stub)
			: [a9] "r"(a9)
			: "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
			  "x17", "x30", "memory");
		return (NTSTATUS)x0;
	}

	// Indirect syscall with 10 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10)
	{
		register USIZE x0 __asm__("x0") = a1;
		register USIZE x1 __asm__("x1") = a2;
		register USIZE x2 __asm__("x2") = a3;
		register USIZE x3 __asm__("x3") = a4;
		register USIZE x4 __asm__("x4") = a5;
		register USIZE x5 __asm__("x5") = a6;
		register USIZE x6 __asm__("x6") = a7;
		register USIZE x7 __asm__("x7") = a8;
		register USIZE stub __asm__("x16") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"sub sp, sp, #16\n"
			"str %[a9], [sp]\n"
			"str %[a10], [sp, #8]\n"
			"blr %[stub]\n"
			"add sp, sp, #16\n"
			: "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7), [stub] "+r"(stub)
			: [a9] "r"(a9), [a10] "r"(a10)
			: "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
			  "x17", "x30", "memory");
		return (NTSTATUS)x0;
	}

	// Indirect syscall with 11 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11)
	{
		register USIZE x0 __asm__("x0") = a1;
		register USIZE x1 __asm__("x1") = a2;
		register USIZE x2 __asm__("x2") = a3;
		register USIZE x3 __asm__("x3") = a4;
		register USIZE x4 __asm__("x4") = a5;
		register USIZE x5 __asm__("x5") = a6;
		register USIZE x6 __asm__("x6") = a7;
		register USIZE x7 __asm__("x7") = a8;
		register USIZE stub __asm__("x16") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"sub sp, sp, #32\n"
			"str %[a9], [sp]\n"
			"str %[a10], [sp, #8]\n"
			"str %[a11], [sp, #16]\n"
			"blr %[stub]\n"
			"add sp, sp, #32\n"
			: "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7), [stub] "+r"(stub)
			: [a9] "r"(a9), [a10] "r"(a10), [a11] "r"(a11)
			: "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
			  "x17", "x30", "memory");
		return (NTSTATUS)x0;
	}

	// Indirect syscall with 12 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11, USIZE a12)
	{
		register USIZE x0 __asm__("x0") = a1;
		register USIZE x1 __asm__("x1") = a2;
		register USIZE x2 __asm__("x2") = a3;
		register USIZE x3 __asm__("x3") = a4;
		register USIZE x4 __asm__("x4") = a5;
		register USIZE x5 __asm__("x5") = a6;
		register USIZE x6 __asm__("x6") = a7;
		register USIZE x7 __asm__("x7") = a8;
		register USIZE stub __asm__("x16") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"sub sp, sp, #32\n"
			"str %[a9], [sp]\n"
			"str %[a10], [sp, #8]\n"
			"str %[a11], [sp, #16]\n"
			"str %[a12], [sp, #24]\n"
			"blr %[stub]\n"
			"add sp, sp, #32\n"
			: "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7), [stub] "+r"(stub)
			: [a9] "r"(a9), [a10] "r"(a10), [a11] "r"(a11), [a12] "r"(a12)
			: "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
			  "x17", "x30", "memory");
		return (NTSTATUS)x0;
	}

	// Indirect syscall with 13 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11, USIZE a12, USIZE a13)
	{
		register USIZE x0 __asm__("x0") = a1;
		register USIZE x1 __asm__("x1") = a2;
		register USIZE x2 __asm__("x2") = a3;
		register USIZE x3 __asm__("x3") = a4;
		register USIZE x4 __asm__("x4") = a5;
		register USIZE x5 __asm__("x5") = a6;
		register USIZE x6 __asm__("x6") = a7;
		register USIZE x7 __asm__("x7") = a8;
		register USIZE stub __asm__("x16") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"sub sp, sp, #48\n"
			"str %[a9], [sp]\n"
			"str %[a10], [sp, #8]\n"
			"str %[a11], [sp, #16]\n"
			"str %[a12], [sp, #24]\n"
			"str %[a13], [sp, #32]\n"
			"blr %[stub]\n"
			"add sp, sp, #48\n"
			: "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7), [stub] "+r"(stub)
			: [a9] "r"(a9), [a10] "r"(a10), [a11] "r"(a11), [a12] "r"(a12), [a13] "r"(a13)
			: "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
			  "x17", "x30", "memory");
		return (NTSTATUS)x0;
	}

	// Indirect syscall with 14 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11, USIZE a12, USIZE a13, USIZE a14)
	{
		register USIZE x0 __asm__("x0") = a1;
		register USIZE x1 __asm__("x1") = a2;
		register USIZE x2 __asm__("x2") = a3;
		register USIZE x3 __asm__("x3") = a4;
		register USIZE x4 __asm__("x4") = a5;
		register USIZE x5 __asm__("x5") = a6;
		register USIZE x6 __asm__("x6") = a7;
		register USIZE x7 __asm__("x7") = a8;
		register USIZE stub __asm__("x16") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"sub sp, sp, #48\n"
			"str %[a9], [sp]\n"
			"str %[a10], [sp, #8]\n"
			"str %[a11], [sp, #16]\n"
			"str %[a12], [sp, #24]\n"
			"str %[a13], [sp, #32]\n"
			"str %[a14], [sp, #40]\n"
			"blr %[stub]\n"
			"add sp, sp, #48\n"
			: "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7), [stub] "+r"(stub)
			: [a9] "r"(a9), [a10] "r"(a10), [a11] "r"(a11), [a12] "r"(a12), [a13] "r"(a13), [a14] "r"(a14)
			: "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
			  "x17", "x30", "memory");
		return (NTSTATUS)x0;
	}

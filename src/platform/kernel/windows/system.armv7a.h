#pragma once

/**
 * @file system.armv7a.h
 * @brief ARMv7-A Windows indirect syscall overloads — included inside class System { }.
 *
 * @details Provides System::Call overloads (0-14 arguments) that invoke Windows NT
 * syscalls via BLX to the resolved ntdll stub address. On ARM32, the kernel validates
 * that the SVC instruction originates from within ntdll (similar to ARM64). Arguments
 * are passed in r0-r3 per AAPCS, with overflow arguments on the stack. r12 holds the
 * ntdll stub address (intra-procedure-call scratch register).
 *
 * Windows ARM32 (Windows RT / Windows 10 IoT) runs user-mode code in Thumb-2 mode.
 * The ntdll stubs follow the pattern: MOV r12, #SSN; SVC #1; BX LR.
 * We call these stubs directly via BLX, which handles Thumb interworking automatically
 * when the stub address has bit 0 set.
 *
 * @see Using Nt and Zw Versions of the Native System Services Routines
 *      https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/using-nt-and-zw-versions-of-the-native-system-services-routines
 */

	// Windows ARM32 indirect syscall via BLX to ntdll stub.
	//
	// On Windows ARM32, the syscall number is loaded into r12 by the ntdll stub
	// (MOV r12, #SSN), then SVC #1 is executed. The kernel reads the SSN from r12.
	// This is different from Linux ARM (syscall number in r7) and ARM64 (SVC immediate).
	//
	// ntdll stubs are: MOV.W r12, #SSN; SVC #1; BX LR
	// We set up args in r0-r3 (+ stack for 5+), then BLX to the stub.
	// The stub's MOV + SVC carries the correct syscall number.
	//
	// r12 holds the stub address before the call. The stub itself overwrites r12
	// with the SSN, so r12 is effectively clobbered.
	// r9 (TEB) is never touched. Callee-saved regs (r4-r8, r10-r11) are safe
	// since the stub is only MOV+SVC+BX LR.
	//
	// Stack alignment: AAPCS requires 8-byte alignment at public call boundaries.
	// When pushing overflow arguments, we always allocate stack space in multiples
	// of 8 bytes and pad unused slots.

	// Indirect syscall with 0 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry)
	{
		register USIZE r0 __asm__("r0");
		register USIZE stub __asm__("r12") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"blx %[stub]\n"
			: "=r"(r0), [stub] "+r"(stub)
			:
			: "r1", "r2", "r3", "lr", "memory", "cc");
		return (NTSTATUS)r0;
	}

	// Indirect syscall with 1 argument
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1)
	{
		register USIZE r0 __asm__("r0") = a1;
		register USIZE stub __asm__("r12") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"blx %[stub]\n"
			: "+r"(r0), [stub] "+r"(stub)
			:
			: "r1", "r2", "r3", "lr", "memory", "cc");
		return (NTSTATUS)r0;
	}

	// Indirect syscall with 2 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2)
	{
		register USIZE r0 __asm__("r0") = a1;
		register USIZE r1 __asm__("r1") = a2;
		register USIZE stub __asm__("r12") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"blx %[stub]\n"
			: "+r"(r0), "+r"(r1), [stub] "+r"(stub)
			:
			: "r2", "r3", "lr", "memory", "cc");
		return (NTSTATUS)r0;
	}

	// Indirect syscall with 3 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3)
	{
		register USIZE r0 __asm__("r0") = a1;
		register USIZE r1 __asm__("r1") = a2;
		register USIZE r2 __asm__("r2") = a3;
		register USIZE stub __asm__("r12") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"blx %[stub]\n"
			: "+r"(r0), "+r"(r1), "+r"(r2), [stub] "+r"(stub)
			:
			: "r3", "lr", "memory", "cc");
		return (NTSTATUS)r0;
	}

	// Indirect syscall with 4 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4)
	{
		register USIZE r0 __asm__("r0") = a1;
		register USIZE r1 __asm__("r1") = a2;
		register USIZE r2 __asm__("r2") = a3;
		register USIZE r3 __asm__("r3") = a4;
		register USIZE stub __asm__("r12") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"blx %[stub]\n"
			: "+r"(r0), "+r"(r1), "+r"(r2), "+r"(r3), [stub] "+r"(stub)
			:
			: "lr", "memory", "cc");
		return (NTSTATUS)r0;
	}

	// Indirect syscall with 5 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5)
	{
		register USIZE r0 __asm__("r0") = a1;
		register USIZE r1 __asm__("r1") = a2;
		register USIZE r2 __asm__("r2") = a3;
		register USIZE r3 __asm__("r3") = a4;
		register USIZE stub __asm__("r12") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"sub sp, sp, #8\n"
			"str %[a5], [sp]\n"
			"blx %[stub]\n"
			"add sp, sp, #8\n"
			: "+r"(r0), "+r"(r1), "+r"(r2), "+r"(r3), [stub] "+r"(stub)
			: [a5] "r"(a5)
			: "lr", "memory", "cc");
		return (NTSTATUS)r0;
	}

	// Indirect syscall with 6 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6)
	{
		register USIZE r0 __asm__("r0") = a1;
		register USIZE r1 __asm__("r1") = a2;
		register USIZE r2 __asm__("r2") = a3;
		register USIZE r3 __asm__("r3") = a4;
		register USIZE stub __asm__("r12") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"sub sp, sp, #8\n"
			"str %[a5], [sp]\n"
			"str %[a6], [sp, #4]\n"
			"blx %[stub]\n"
			"add sp, sp, #8\n"
			: "+r"(r0), "+r"(r1), "+r"(r2), "+r"(r3), [stub] "+r"(stub)
			: [a5] "r"(a5), [a6] "r"(a6)
			: "lr", "memory", "cc");
		return (NTSTATUS)r0;
	}

	// Indirect syscall with 7 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7)
	{
		register USIZE r0 __asm__("r0") = a1;
		register USIZE r1 __asm__("r1") = a2;
		register USIZE r2 __asm__("r2") = a3;
		register USIZE r3 __asm__("r3") = a4;
		register USIZE stub __asm__("r12") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"sub sp, sp, #16\n"
			"str %[a5], [sp]\n"
			"str %[a6], [sp, #4]\n"
			"str %[a7], [sp, #8]\n"
			"blx %[stub]\n"
			"add sp, sp, #16\n"
			: "+r"(r0), "+r"(r1), "+r"(r2), "+r"(r3), [stub] "+r"(stub)
			: [a5] "r"(a5), [a6] "r"(a6), [a7] "r"(a7)
			: "lr", "memory", "cc");
		return (NTSTATUS)r0;
	}

	// Indirect syscall with 8 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8)
	{
		register USIZE r0 __asm__("r0") = a1;
		register USIZE r1 __asm__("r1") = a2;
		register USIZE r2 __asm__("r2") = a3;
		register USIZE r3 __asm__("r3") = a4;
		register USIZE stub __asm__("r12") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"sub sp, sp, #16\n"
			"str %[a5], [sp]\n"
			"str %[a6], [sp, #4]\n"
			"str %[a7], [sp, #8]\n"
			"str %[a8], [sp, #12]\n"
			"blx %[stub]\n"
			"add sp, sp, #16\n"
			: "+r"(r0), "+r"(r1), "+r"(r2), "+r"(r3), [stub] "+r"(stub)
			: [a5] "r"(a5), [a6] "r"(a6), [a7] "r"(a7), [a8] "r"(a8)
			: "lr", "memory", "cc");
		return (NTSTATUS)r0;
	}

	// Indirect syscall with 9 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9)
	{
		register USIZE r0 __asm__("r0") = a1;
		register USIZE r1 __asm__("r1") = a2;
		register USIZE r2 __asm__("r2") = a3;
		register USIZE r3 __asm__("r3") = a4;
		register USIZE stub __asm__("r12") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"sub sp, sp, #24\n"
			"str %[a5], [sp]\n"
			"str %[a6], [sp, #4]\n"
			"str %[a7], [sp, #8]\n"
			"str %[a8], [sp, #12]\n"
			"str %[a9], [sp, #16]\n"
			"blx %[stub]\n"
			"add sp, sp, #24\n"
			: "+r"(r0), "+r"(r1), "+r"(r2), "+r"(r3), [stub] "+r"(stub)
			: [a5] "r"(a5), [a6] "r"(a6), [a7] "r"(a7), [a8] "r"(a8), [a9] "r"(a9)
			: "lr", "memory", "cc");
		return (NTSTATUS)r0;
	}

	// Indirect syscall with 10 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10)
	{
		register USIZE r0 __asm__("r0") = a1;
		register USIZE r1 __asm__("r1") = a2;
		register USIZE r2 __asm__("r2") = a3;
		register USIZE r3 __asm__("r3") = a4;
		register USIZE stub __asm__("r12") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"sub sp, sp, #24\n"
			"str %[a5], [sp]\n"
			"str %[a6], [sp, #4]\n"
			"str %[a7], [sp, #8]\n"
			"str %[a8], [sp, #12]\n"
			"str %[a9], [sp, #16]\n"
			"str %[a10], [sp, #20]\n"
			"blx %[stub]\n"
			"add sp, sp, #24\n"
			: "+r"(r0), "+r"(r1), "+r"(r2), "+r"(r3), [stub] "+r"(stub)
			: [a5] "r"(a5), [a6] "r"(a6), [a7] "r"(a7), [a8] "r"(a8), [a9] "r"(a9), [a10] "r"(a10)
			: "lr", "memory", "cc");
		return (NTSTATUS)r0;
	}

	// Indirect syscall with 11 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11)
	{
		register USIZE r0 __asm__("r0") = a1;
		register USIZE r1 __asm__("r1") = a2;
		register USIZE r2 __asm__("r2") = a3;
		register USIZE r3 __asm__("r3") = a4;
		register USIZE stub __asm__("r12") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"sub sp, sp, #32\n"
			"str %[a5], [sp]\n"
			"str %[a6], [sp, #4]\n"
			"str %[a7], [sp, #8]\n"
			"str %[a8], [sp, #12]\n"
			"str %[a9], [sp, #16]\n"
			"str %[a10], [sp, #20]\n"
			"str %[a11], [sp, #24]\n"
			"blx %[stub]\n"
			"add sp, sp, #32\n"
			: "+r"(r0), "+r"(r1), "+r"(r2), "+r"(r3), [stub] "+r"(stub)
			: [a5] "r"(a5), [a6] "r"(a6), [a7] "r"(a7), [a8] "r"(a8), [a9] "r"(a9), [a10] "r"(a10), [a11] "r"(a11)
			: "lr", "memory", "cc");
		return (NTSTATUS)r0;
	}

	// Indirect syscall with 12 arguments
	//
	// Split into two asm blocks to reduce per-block register pressure.
	// At -O0 with -fno-omit-frame-pointer, ARM32 has only 12 allocatable
	// GPRs (r0-r10, r12; r11=fp). A single block with 5 output + 8 input
	// operands needs 13 — exceeding the limit. The first block stores
	// overflow args (8 "r" inputs, 0 outputs = 8 regs needed); the second
	// does the BLX call (5 "+r" outputs, 0 inputs = 5 regs needed).
	// At -O0 all variable access is fp-relative, so sp modification in the
	// first block does not affect operand addressing in the second.
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11, USIZE a12)
	{
		__asm__ volatile(
			"sub sp, sp, #32\n"
			"str %[a5], [sp]\n"
			"str %[a6], [sp, #4]\n"
			"str %[a7], [sp, #8]\n"
			"str %[a8], [sp, #12]\n"
			"str %[a9], [sp, #16]\n"
			"str %[a10], [sp, #20]\n"
			"str %[a11], [sp, #24]\n"
			"str %[a12], [sp, #28]\n"
			:
			: [a5] "r"(a5), [a6] "r"(a6), [a7] "r"(a7), [a8] "r"(a8), [a9] "r"(a9), [a10] "r"(a10), [a11] "r"(a11), [a12] "r"(a12)
			: "memory");
		register USIZE r0 __asm__("r0") = a1;
		register USIZE r1 __asm__("r1") = a2;
		register USIZE r2 __asm__("r2") = a3;
		register USIZE r3 __asm__("r3") = a4;
		register USIZE stub __asm__("r12") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"blx %[stub]\n"
			"add sp, sp, #32\n"
			: "+r"(r0), "+r"(r1), "+r"(r2), "+r"(r3), [stub] "+r"(stub)
			:
			: "lr", "memory", "cc");
		return (NTSTATUS)r0;
	}

	// Indirect syscall with 13 arguments (two-stage asm, see 12-arg comment)
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11, USIZE a12, USIZE a13)
	{
		__asm__ volatile(
			"sub sp, sp, #40\n"
			"str %[a5], [sp]\n"
			"str %[a6], [sp, #4]\n"
			"str %[a7], [sp, #8]\n"
			"str %[a8], [sp, #12]\n"
			"str %[a9], [sp, #16]\n"
			"str %[a10], [sp, #20]\n"
			"str %[a11], [sp, #24]\n"
			"str %[a12], [sp, #28]\n"
			"str %[a13], [sp, #32]\n"
			:
			: [a5] "r"(a5), [a6] "r"(a6), [a7] "r"(a7), [a8] "r"(a8), [a9] "r"(a9), [a10] "r"(a10), [a11] "r"(a11), [a12] "r"(a12), [a13] "r"(a13)
			: "memory");
		register USIZE r0 __asm__("r0") = a1;
		register USIZE r1 __asm__("r1") = a2;
		register USIZE r2 __asm__("r2") = a3;
		register USIZE r3 __asm__("r3") = a4;
		register USIZE stub __asm__("r12") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"blx %[stub]\n"
			"add sp, sp, #40\n"
			: "+r"(r0), "+r"(r1), "+r"(r2), "+r"(r3), [stub] "+r"(stub)
			:
			: "lr", "memory", "cc");
		return (NTSTATUS)r0;
	}

	// Indirect syscall with 14 arguments (two-stage asm, see 12-arg comment)
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11, USIZE a12, USIZE a13, USIZE a14)
	{
		__asm__ volatile(
			"sub sp, sp, #40\n"
			"str %[a5], [sp]\n"
			"str %[a6], [sp, #4]\n"
			"str %[a7], [sp, #8]\n"
			"str %[a8], [sp, #12]\n"
			"str %[a9], [sp, #16]\n"
			"str %[a10], [sp, #20]\n"
			"str %[a11], [sp, #24]\n"
			"str %[a12], [sp, #28]\n"
			"str %[a13], [sp, #32]\n"
			"str %[a14], [sp, #36]\n"
			:
			: [a5] "r"(a5), [a6] "r"(a6), [a7] "r"(a7), [a8] "r"(a8), [a9] "r"(a9), [a10] "r"(a10), [a11] "r"(a11), [a12] "r"(a12), [a13] "r"(a13), [a14] "r"(a14)
			: "memory");
		register USIZE r0 __asm__("r0") = a1;
		register USIZE r1 __asm__("r1") = a2;
		register USIZE r2 __asm__("r2") = a3;
		register USIZE r3 __asm__("r3") = a4;
		register USIZE stub __asm__("r12") = (USIZE)entry.SyscallAddress;
		__asm__ volatile(
			"blx %[stub]\n"
			"add sp, sp, #40\n"
			: "+r"(r0), "+r"(r1), "+r"(r2), "+r"(r3), [stub] "+r"(stub)
			:
			: "lr", "memory", "cc");
		return (NTSTATUS)r0;
	}

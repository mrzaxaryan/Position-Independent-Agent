#pragma once

/**
 * @file system.x86_64.h
 * @brief x86_64 Windows indirect syscall overloads — included inside class System { }.
 *
 * @details Provides System::Call overloads (0-14 arguments) that invoke Windows NT
 * syscalls via indirect calls through a gadget address within ntdll.dll. The SSN
 * is placed in RAX, the first argument is moved from RCX to R10 (since syscall
 * clobbers RCX), and arguments 2-4 go in RDX/R8/R9. Arguments 5+ are placed on
 * the stack above the 32-byte shadow space per the Microsoft x64 calling convention.
 *
 * @see Using Nt and Zw Versions of the Native System Services Routines
 *      https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/using-nt-and-zw-versions-of-the-native-system-services-routines
 */

	// Indirect syscall with 0 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry)
	{
		register USIZE r_rax __asm__("rax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"call *%[gadget]\n"
			: "+r"(r_rax)
			: [gadget] "r"(entry.SyscallAddress)
			: "rcx", "rdx", "r8", "r9", "r10", "r11", "memory");
		return (NTSTATUS)r_rax;
	}

	// Indirect syscall with 1 argument
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1)
	{
		register USIZE r_r10 __asm__("r10") = a1;
		register USIZE r_rax __asm__("rax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"call *%[gadget]\n"
			: "+r"(r_rax), "+r"(r_r10)
			: [gadget] "r"(entry.SyscallAddress)
			: "rcx", "rdx", "r8", "r9", "r11", "memory");
		return (NTSTATUS)r_rax;
	}

	// Indirect syscall with 2 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2)
	{
		register USIZE r_r10 __asm__("r10") = a1;
		register USIZE r_rdx __asm__("rdx") = a2;
		register USIZE r_rax __asm__("rax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"call *%[gadget]\n"
			: "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx)
			: [gadget] "r"(entry.SyscallAddress)
			: "rcx", "r8", "r9", "r11", "memory");
		return (NTSTATUS)r_rax;
	}

	// Indirect syscall with 3 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3)
	{
		register USIZE r_r10 __asm__("r10") = a1;
		register USIZE r_rdx __asm__("rdx") = a2;
		register USIZE r_r8 __asm__("r8") = a3;
		register USIZE r_rax __asm__("rax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"call *%[gadget]\n"
			: "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx), "+r"(r_r8)
			: [gadget] "r"(entry.SyscallAddress)
			: "rcx", "r9", "r11", "memory");
		return (NTSTATUS)r_rax;
	}

	// Indirect syscall with 4 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4)
	{
		register USIZE r_r10 __asm__("r10") = a1;
		register USIZE r_rdx __asm__("rdx") = a2;
		register USIZE r_r8 __asm__("r8") = a3;
		register USIZE r_r9 __asm__("r9") = a4;
		register USIZE r_rax __asm__("rax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"call *%[gadget]\n"
			: "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx), "+r"(r_r8), "+r"(r_r9)
			: [gadget] "r"(entry.SyscallAddress)
			: "rcx", "r11", "memory");
		return (NTSTATUS)r_rax;
	}

	// Indirect syscall with 5 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5)
	{
		register USIZE r_r10 __asm__("r10") = a1;
		register USIZE r_rdx __asm__("rdx") = a2;
		register USIZE r_r8 __asm__("r8") = a3;
		register USIZE r_r9 __asm__("r9") = a4;
		register USIZE r_rax __asm__("rax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"sub $0x28, %%rsp\n"
			"mov %[a5], 0x20(%%rsp)\n"
			"call *%[gadget]\n"
			"add $0x28, %%rsp\n"
			: "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx), "+r"(r_r8), "+r"(r_r9)
			: [a5] "r"(a5), [gadget] "r"(entry.SyscallAddress)
			: "rcx", "r11", "memory");
		return (NTSTATUS)r_rax;
	}

	// Indirect syscall with 6 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6)
	{
		register USIZE r_r10 __asm__("r10") = a1;
		register USIZE r_rdx __asm__("rdx") = a2;
		register USIZE r_r8 __asm__("r8") = a3;
		register USIZE r_r9 __asm__("r9") = a4;
		register USIZE r_rax __asm__("rax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"sub $0x30, %%rsp\n"
			"mov %[a5], 0x20(%%rsp)\n"
			"mov %[a6], 0x28(%%rsp)\n"
			"call *%[gadget]\n"
			"add $0x30, %%rsp\n"
			: "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx), "+r"(r_r8), "+r"(r_r9)
			: [a5] "r"(a5), [a6] "r"(a6), [gadget] "r"(entry.SyscallAddress)
			: "rcx", "r11", "memory");
		return (NTSTATUS)r_rax;
	}

	// Indirect syscall with 7 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7)
	{
		register USIZE r_r10 __asm__("r10") = a1;
		register USIZE r_rdx __asm__("rdx") = a2;
		register USIZE r_r8 __asm__("r8") = a3;
		register USIZE r_r9 __asm__("r9") = a4;
		register USIZE r_rax __asm__("rax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"sub $0x38, %%rsp\n"
			"mov %[a5], 0x20(%%rsp)\n"
			"mov %[a6], 0x28(%%rsp)\n"
			"mov %[a7], 0x30(%%rsp)\n"
			"call *%[gadget]\n"
			"add $0x38, %%rsp\n"
			: "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx), "+r"(r_r8), "+r"(r_r9)
			: [a5] "r"(a5), [a6] "r"(a6), [a7] "r"(a7),
			  [gadget] "r"(entry.SyscallAddress)
			: "rcx", "r11", "memory");
		return (NTSTATUS)r_rax;
	}

	// Indirect syscall with 8 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8)
	{
		register USIZE r_r10 __asm__("r10") = a1;
		register USIZE r_rdx __asm__("rdx") = a2;
		register USIZE r_r8 __asm__("r8") = a3;
		register USIZE r_r9 __asm__("r9") = a4;
		register USIZE r_rax __asm__("rax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"sub $0x40, %%rsp\n"
			"mov %[a5], 0x20(%%rsp)\n"
			"mov %[a6], 0x28(%%rsp)\n"
			"mov %[a7], 0x30(%%rsp)\n"
			"mov %[a8], 0x38(%%rsp)\n"
			"call *%[gadget]\n"
			"add $0x40, %%rsp\n"
			: "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx), "+r"(r_r8), "+r"(r_r9)
			: [a5] "r"(a5), [a6] "r"(a6), [a7] "r"(a7), [a8] "r"(a8),
			  [gadget] "r"(entry.SyscallAddress)
			: "rcx", "r11", "memory");
		return (NTSTATUS)r_rax;
	}

	// Indirect syscall with 9 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9)
	{
		register USIZE r_r10 __asm__("r10") = a1;
		register USIZE r_rdx __asm__("rdx") = a2;
		register USIZE r_r8 __asm__("r8") = a3;
		register USIZE r_r9 __asm__("r9") = a4;
		register USIZE r_rax __asm__("rax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"sub $0x48, %%rsp\n"
			"mov %[a5], 0x20(%%rsp)\n"
			"mov %[a6], 0x28(%%rsp)\n"
			"mov %[a7], 0x30(%%rsp)\n"
			"mov %[a8], 0x38(%%rsp)\n"
			"mov %[a9], 0x40(%%rsp)\n"
			"call *%[gadget]\n"
			"add $0x48, %%rsp\n"
			: "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx), "+r"(r_r8), "+r"(r_r9)
			: [a5] "r"(a5), [a6] "r"(a6), [a7] "r"(a7), [a8] "r"(a8),
			  [a9] "r"(a9), [gadget] "r"(entry.SyscallAddress)
			: "rcx", "r11", "memory");
		return (NTSTATUS)r_rax;
	}

	// Indirect syscall with 10 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10)
	{
		register USIZE r_r10 __asm__("r10") = a1;
		register USIZE r_rdx __asm__("rdx") = a2;
		register USIZE r_r8 __asm__("r8") = a3;
		register USIZE r_r9 __asm__("r9") = a4;
		register USIZE r_rax __asm__("rax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"sub $0x50, %%rsp\n"
			"mov %[a5],  0x20(%%rsp)\n"
			"mov %[a6],  0x28(%%rsp)\n"
			"mov %[a7],  0x30(%%rsp)\n"
			"mov %[a8],  0x38(%%rsp)\n"
			"mov %[a9],  0x40(%%rsp)\n"
			"mov %[a10], 0x48(%%rsp)\n"
			"call *%[gadget]\n"
			"add $0x50, %%rsp\n"
			: "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx), "+r"(r_r8), "+r"(r_r9)
			: [a5] "r"(a5), [a6] "r"(a6), [a7] "r"(a7), [a8] "r"(a8),
			  [a9] "r"(a9), [a10] "r"(a10), [gadget] "r"(entry.SyscallAddress)
			: "rcx", "r11", "memory");
		return (NTSTATUS)r_rax;
	}

	// Indirect syscall with 11 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11)
	{
		register USIZE r_r10 __asm__("r10") = a1;
		register USIZE r_rdx __asm__("rdx") = a2;
		register USIZE r_r8 __asm__("r8") = a3;
		register USIZE r_r9 __asm__("r9") = a4;
		register USIZE r_rax __asm__("rax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"mov %[a11], %%rcx\n"
			"sub $0x58, %%rsp\n"
			"mov %[a5],  0x20(%%rsp)\n"
			"mov %[a6],  0x28(%%rsp)\n"
			"mov %[a7],  0x30(%%rsp)\n"
			"mov %[a8],  0x38(%%rsp)\n"
			"mov %[a9],  0x40(%%rsp)\n"
			"mov %[a10], 0x48(%%rsp)\n"
			"mov %%rcx,  0x50(%%rsp)\n"
			"call *%[gadget]\n"
			"add $0x58, %%rsp\n"
			: "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx), "+r"(r_r8), "+r"(r_r9)
			: [a5] "r"(a5), [a6] "r"(a6), [a7] "r"(a7), [a8] "r"(a8),
			  [a9] "r"(a9), [a10] "r"(a10), [a11] "m"(a11), [gadget] "r"(entry.SyscallAddress)
			: "rcx", "r11", "memory");
		return (NTSTATUS)r_rax;
	}

	// Indirect syscall with 12 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11, USIZE a12)
	{
		register USIZE r_r10 __asm__("r10") = a1;
		register USIZE r_rdx __asm__("rdx") = a2;
		register USIZE r_r8 __asm__("r8") = a3;
		register USIZE r_r9 __asm__("r9") = a4;
		register USIZE r_rax __asm__("rax") = (USIZE)entry.Ssn;
		USIZE args[] = {a5, a6, a7, a8, a9, a10, a11, a12};
		__asm__ volatile(
			"sub $0x60, %%rsp\n"
			"mov (%[args]), %%rcx\n"
			"mov %%rcx, 0x20(%%rsp)\n"
			"mov 0x8(%[args]), %%rcx\n"
			"mov %%rcx, 0x28(%%rsp)\n"
			"mov 0x10(%[args]), %%rcx\n"
			"mov %%rcx, 0x30(%%rsp)\n"
			"mov 0x18(%[args]), %%rcx\n"
			"mov %%rcx, 0x38(%%rsp)\n"
			"mov 0x20(%[args]), %%rcx\n"
			"mov %%rcx, 0x40(%%rsp)\n"
			"mov 0x28(%[args]), %%rcx\n"
			"mov %%rcx, 0x48(%%rsp)\n"
			"mov 0x30(%[args]), %%rcx\n"
			"mov %%rcx, 0x50(%%rsp)\n"
			"mov 0x38(%[args]), %%rcx\n"
			"mov %%rcx, 0x58(%%rsp)\n"
			"call *%[gadget]\n"
			"add $0x60, %%rsp\n"
			: "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx), "+r"(r_r8), "+r"(r_r9)
			: [args] "r"(args), [gadget] "r"(entry.SyscallAddress)
			: "rcx", "r11", "memory");
		return (NTSTATUS)r_rax;
	}

	// Indirect syscall with 13 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11, USIZE a12, USIZE a13)
	{
		register USIZE r_r10 __asm__("r10") = a1;
		register USIZE r_rdx __asm__("rdx") = a2;
		register USIZE r_r8 __asm__("r8") = a3;
		register USIZE r_r9 __asm__("r9") = a4;
		register USIZE r_rax __asm__("rax") = (USIZE)entry.Ssn;
		USIZE args[] = {a5, a6, a7, a8, a9, a10, a11, a12, a13};
		__asm__ volatile(
			"sub $0x68, %%rsp\n"
			"mov (%[args]), %%rcx\n"
			"mov %%rcx, 0x20(%%rsp)\n"
			"mov 0x8(%[args]), %%rcx\n"
			"mov %%rcx, 0x28(%%rsp)\n"
			"mov 0x10(%[args]), %%rcx\n"
			"mov %%rcx, 0x30(%%rsp)\n"
			"mov 0x18(%[args]), %%rcx\n"
			"mov %%rcx, 0x38(%%rsp)\n"
			"mov 0x20(%[args]), %%rcx\n"
			"mov %%rcx, 0x40(%%rsp)\n"
			"mov 0x28(%[args]), %%rcx\n"
			"mov %%rcx, 0x48(%%rsp)\n"
			"mov 0x30(%[args]), %%rcx\n"
			"mov %%rcx, 0x50(%%rsp)\n"
			"mov 0x38(%[args]), %%rcx\n"
			"mov %%rcx, 0x58(%%rsp)\n"
			"mov 0x40(%[args]), %%rcx\n"
			"mov %%rcx, 0x60(%%rsp)\n"
			"call *%[gadget]\n"
			"add $0x68, %%rsp\n"
			: "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx), "+r"(r_r8), "+r"(r_r9)
			: [args] "r"(args), [gadget] "r"(entry.SyscallAddress)
			: "rcx", "r11", "memory");
		return (NTSTATUS)r_rax;
	}

	// Indirect syscall with 14 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11, USIZE a12, USIZE a13, USIZE a14)
	{
		register USIZE r_r10 __asm__("r10") = a1;
		register USIZE r_rdx __asm__("rdx") = a2;
		register USIZE r_r8 __asm__("r8") = a3;
		register USIZE r_r9 __asm__("r9") = a4;
		register USIZE r_rax __asm__("rax") = (USIZE)entry.Ssn;
		USIZE args[] = {a5, a6, a7, a8, a9, a10, a11, a12, a13, a14};
		__asm__ volatile(
			"sub $0x70, %%rsp\n"
			"mov (%[args]), %%rcx\n"
			"mov %%rcx, 0x20(%%rsp)\n"
			"mov 0x8(%[args]), %%rcx\n"
			"mov %%rcx, 0x28(%%rsp)\n"
			"mov 0x10(%[args]), %%rcx\n"
			"mov %%rcx, 0x30(%%rsp)\n"
			"mov 0x18(%[args]), %%rcx\n"
			"mov %%rcx, 0x38(%%rsp)\n"
			"mov 0x20(%[args]), %%rcx\n"
			"mov %%rcx, 0x40(%%rsp)\n"
			"mov 0x28(%[args]), %%rcx\n"
			"mov %%rcx, 0x48(%%rsp)\n"
			"mov 0x30(%[args]), %%rcx\n"
			"mov %%rcx, 0x50(%%rsp)\n"
			"mov 0x38(%[args]), %%rcx\n"
			"mov %%rcx, 0x58(%%rsp)\n"
			"mov 0x40(%[args]), %%rcx\n"
			"mov %%rcx, 0x60(%%rsp)\n"
			"mov 0x48(%[args]), %%rcx\n"
			"mov %%rcx, 0x68(%%rsp)\n"
			"call *%[gadget]\n"
			"add $0x70, %%rsp\n"
			: "+r"(r_rax), "+r"(r_r10), "+r"(r_rdx), "+r"(r_r8), "+r"(r_r9)
			: [args] "r"(args), [gadget] "r"(entry.SyscallAddress)
			: "rcx", "r11", "memory");
		return (NTSTATUS)r_rax;
	}

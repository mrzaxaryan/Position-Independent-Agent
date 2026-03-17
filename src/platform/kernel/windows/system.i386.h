#pragma once

/**
 * @file system.i386.h
 * @brief i386 Windows indirect syscall overloads — included inside class System { }.
 *
 * @details Provides System::Call overloads (0-14 arguments) that invoke Windows NT
 * syscalls via indirect calls through a gadget address within ntdll.dll. All arguments
 * are pushed onto the stack in reverse order with a dummy DWORD for the expected
 * return address slot. EAX holds the SSN and the gadget address (KiFastSystemCall or
 * WoW64 trampoline) is called indirectly via EDX.
 *
 * @see Using Nt and Zw Versions of the Native System Services Routines
 *      https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/using-nt-and-zw-versions-of-the-native-system-services-routines
 */

	// Windows i386 syscalls are STACK-BASED: all arguments are pushed onto the stack,
	// EAX holds the syscall number, and the gadget handles the kernel transition.
	//
	// Native i386: gadget = KiFastSystemCall (mov edx,esp; sysenter)
	// WoW64:       gadget = trampoline (jmp to wow64cpu 32-to-64 transition)
	//
	// We push a dummy DWORD to fill the "original caller return address" slot,
	// so the kernel/handler finds arguments at the expected stack offset:
	//   [ESP+0x00] = return address (from our call instruction)
	//   [ESP+0x04] = dummy (fills expected second return address slot)
	//   [ESP+0x08] = arg1
	//   [ESP+0x0C] = arg2 ...
	//
	// EAX is pre-loaded with the SSN by the compiler via register variable,
	// and the gadget address is passed as a separate register operand to
	// prevent register aliasing (the compiler must not assign [gadget] to EAX).

	// Indirect syscall with 0 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry)
	{
		register USIZE r_eax __asm__("eax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"pushl $0\n"
			"movl %[gadget], %%edx\n"
			"call *%%edx\n"
			"addl $4, %%esp\n"
			: "+a"(r_eax)
			: [gadget] "r"(entry.SyscallAddress)
			: "ecx", "edx", "memory");
		return (NTSTATUS)r_eax;
	}

	// Indirect syscall with 1 argument
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1)
	{
		register USIZE r_eax __asm__("eax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"pushl %[a1]\n"
			"pushl $0\n"
			"movl %[gadget], %%edx\n"
			"call *%%edx\n"
			"addl $8, %%esp\n"
			: "+a"(r_eax)
			: [gadget] "r"(entry.SyscallAddress), [a1] "r"(a1)
			: "ecx", "edx", "memory");
		return (NTSTATUS)r_eax;
	}

	// Indirect syscall with 2 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2)
	{
		USIZE args[] = {a1, a2};
		register USIZE r_eax __asm__("eax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"pushl 4(%[args])\n"
			"pushl (%[args])\n"
			"pushl $0\n"
			"movl %[gadget], %%edx\n"
			"call *%%edx\n"
			"addl $12, %%esp\n"
			: "+a"(r_eax)
			: [gadget] "r"(entry.SyscallAddress), [args] "r"(args)
			: "ecx", "edx", "memory");
		return (NTSTATUS)r_eax;
	}

	// Indirect syscall with 3 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3)
	{
		USIZE args[] = {a1, a2, a3};
		register USIZE r_eax __asm__("eax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"pushl 8(%[args])\n"
			"pushl 4(%[args])\n"
			"pushl (%[args])\n"
			"pushl $0\n"
			"movl %[gadget], %%edx\n"
			"call *%%edx\n"
			"addl $16, %%esp\n"
			: "+a"(r_eax)
			: [gadget] "r"(entry.SyscallAddress), [args] "r"(args)
			: "ecx", "edx", "memory");
		return (NTSTATUS)r_eax;
	}

	// Indirect syscall with 4 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4)
	{
		USIZE args[] = {a1, a2, a3, a4};
		register USIZE r_eax __asm__("eax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"pushl 12(%[args])\n"
			"pushl 8(%[args])\n"
			"pushl 4(%[args])\n"
			"pushl (%[args])\n"
			"pushl $0\n"
			"movl %[gadget], %%edx\n"
			"call *%%edx\n"
			"addl $20, %%esp\n"
			: "+a"(r_eax)
			: [gadget] "r"(entry.SyscallAddress), [args] "r"(args)
			: "ecx", "edx", "memory");
		return (NTSTATUS)r_eax;
	}

	// Indirect syscall with 5 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5)
	{
		USIZE args[] = {a1, a2, a3, a4, a5};
		register USIZE r_eax __asm__("eax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"pushl 16(%[args])\n"
			"pushl 12(%[args])\n"
			"pushl 8(%[args])\n"
			"pushl 4(%[args])\n"
			"pushl (%[args])\n"
			"pushl $0\n"
			"movl %[gadget], %%edx\n"
			"call *%%edx\n"
			"addl $24, %%esp\n"
			: "+a"(r_eax)
			: [gadget] "r"(entry.SyscallAddress), [args] "r"(args)
			: "ecx", "edx", "memory");
		return (NTSTATUS)r_eax;
	}

	// Indirect syscall with 6 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6)
	{
		USIZE args[] = {a1, a2, a3, a4, a5, a6};
		register USIZE r_eax __asm__("eax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"pushl 20(%[args])\n"
			"pushl 16(%[args])\n"
			"pushl 12(%[args])\n"
			"pushl 8(%[args])\n"
			"pushl 4(%[args])\n"
			"pushl (%[args])\n"
			"pushl $0\n"
			"movl %[gadget], %%edx\n"
			"call *%%edx\n"
			"addl $28, %%esp\n"
			: "+a"(r_eax)
			: [gadget] "r"(entry.SyscallAddress), [args] "r"(args)
			: "ecx", "edx", "memory");
		return (NTSTATUS)r_eax;
	}

	// Indirect syscall with 7 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7)
	{
		USIZE args[] = {a1, a2, a3, a4, a5, a6, a7};
		register USIZE r_eax __asm__("eax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"pushl 24(%[args])\n"
			"pushl 20(%[args])\n"
			"pushl 16(%[args])\n"
			"pushl 12(%[args])\n"
			"pushl 8(%[args])\n"
			"pushl 4(%[args])\n"
			"pushl (%[args])\n"
			"pushl $0\n"
			"movl %[gadget], %%edx\n"
			"call *%%edx\n"
			"addl $32, %%esp\n"
			: "+a"(r_eax)
			: [gadget] "r"(entry.SyscallAddress), [args] "r"(args)
			: "ecx", "edx", "memory");
		return (NTSTATUS)r_eax;
	}

	// Indirect syscall with 8 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8)
	{
		USIZE args[] = {a1, a2, a3, a4, a5, a6, a7, a8};
		register USIZE r_eax __asm__("eax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"pushl 28(%[args])\n"
			"pushl 24(%[args])\n"
			"pushl 20(%[args])\n"
			"pushl 16(%[args])\n"
			"pushl 12(%[args])\n"
			"pushl 8(%[args])\n"
			"pushl 4(%[args])\n"
			"pushl (%[args])\n"
			"pushl $0\n"
			"movl %[gadget], %%edx\n"
			"call *%%edx\n"
			"addl $36, %%esp\n"
			: "+a"(r_eax)
			: [gadget] "r"(entry.SyscallAddress), [args] "r"(args)
			: "ecx", "edx", "memory");
		return (NTSTATUS)r_eax;
	}

	// Indirect syscall with 9 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9)
	{
		USIZE args[] = {a1, a2, a3, a4, a5, a6, a7, a8, a9};
		register USIZE r_eax __asm__("eax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"pushl 32(%[args])\n"
			"pushl 28(%[args])\n"
			"pushl 24(%[args])\n"
			"pushl 20(%[args])\n"
			"pushl 16(%[args])\n"
			"pushl 12(%[args])\n"
			"pushl 8(%[args])\n"
			"pushl 4(%[args])\n"
			"pushl (%[args])\n"
			"pushl $0\n"
			"movl %[gadget], %%edx\n"
			"call *%%edx\n"
			"addl $40, %%esp\n"
			: "+a"(r_eax)
			: [gadget] "r"(entry.SyscallAddress), [args] "r"(args)
			: "ecx", "edx", "memory");
		return (NTSTATUS)r_eax;
	}

	// Indirect syscall with 10 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10)
	{
		USIZE args[] = {a1, a2, a3, a4, a5, a6, a7, a8, a9, a10};
		register USIZE r_eax __asm__("eax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"pushl 36(%[args])\n"
			"pushl 32(%[args])\n"
			"pushl 28(%[args])\n"
			"pushl 24(%[args])\n"
			"pushl 20(%[args])\n"
			"pushl 16(%[args])\n"
			"pushl 12(%[args])\n"
			"pushl 8(%[args])\n"
			"pushl 4(%[args])\n"
			"pushl (%[args])\n"
			"pushl $0\n"
			"movl %[gadget], %%edx\n"
			"call *%%edx\n"
			"addl $44, %%esp\n"
			: "+a"(r_eax)
			: [gadget] "r"(entry.SyscallAddress), [args] "r"(args)
			: "ecx", "edx", "memory");
		return (NTSTATUS)r_eax;
	}

	// Indirect syscall with 11 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11)
	{
		USIZE args[] = {a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11};
		register USIZE r_eax __asm__("eax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"pushl 40(%[args])\n"
			"pushl 36(%[args])\n"
			"pushl 32(%[args])\n"
			"pushl 28(%[args])\n"
			"pushl 24(%[args])\n"
			"pushl 20(%[args])\n"
			"pushl 16(%[args])\n"
			"pushl 12(%[args])\n"
			"pushl 8(%[args])\n"
			"pushl 4(%[args])\n"
			"pushl (%[args])\n"
			"pushl $0\n"
			"movl %[gadget], %%edx\n"
			"call *%%edx\n"
			"addl $48, %%esp\n"
			: "+a"(r_eax)
			: [gadget] "r"(entry.SyscallAddress), [args] "r"(args)
			: "ecx", "edx", "memory");
		return (NTSTATUS)r_eax;
	}

	// Indirect syscall with 12 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11, USIZE a12)
	{
		USIZE args[] = {a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12};
		register USIZE r_eax __asm__("eax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"pushl 44(%[args])\n"
			"pushl 40(%[args])\n"
			"pushl 36(%[args])\n"
			"pushl 32(%[args])\n"
			"pushl 28(%[args])\n"
			"pushl 24(%[args])\n"
			"pushl 20(%[args])\n"
			"pushl 16(%[args])\n"
			"pushl 12(%[args])\n"
			"pushl 8(%[args])\n"
			"pushl 4(%[args])\n"
			"pushl (%[args])\n"
			"pushl $0\n"
			"movl %[gadget], %%edx\n"
			"call *%%edx\n"
			"addl $52, %%esp\n"
			: "+a"(r_eax)
			: [gadget] "r"(entry.SyscallAddress), [args] "r"(args)
			: "ecx", "edx", "memory");
		return (NTSTATUS)r_eax;
	}

	// Indirect syscall with 13 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11, USIZE a12, USIZE a13)
	{
		USIZE args[] = {a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13};
		register USIZE r_eax __asm__("eax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"pushl 48(%[args])\n"
			"pushl 44(%[args])\n"
			"pushl 40(%[args])\n"
			"pushl 36(%[args])\n"
			"pushl 32(%[args])\n"
			"pushl 28(%[args])\n"
			"pushl 24(%[args])\n"
			"pushl 20(%[args])\n"
			"pushl 16(%[args])\n"
			"pushl 12(%[args])\n"
			"pushl 8(%[args])\n"
			"pushl 4(%[args])\n"
			"pushl (%[args])\n"
			"pushl $0\n"
			"movl %[gadget], %%edx\n"
			"call *%%edx\n"
			"addl $56, %%esp\n"
			: "+a"(r_eax)
			: [gadget] "r"(entry.SyscallAddress), [args] "r"(args)
			: "ecx", "edx", "memory");
		return (NTSTATUS)r_eax;
	}

	// Indirect syscall with 14 arguments
	static NOINLINE NTSTATUS Call(SYSCALL_ENTRY entry, USIZE a1, USIZE a2, USIZE a3, USIZE a4, USIZE a5, USIZE a6, USIZE a7, USIZE a8, USIZE a9, USIZE a10, USIZE a11, USIZE a12, USIZE a13, USIZE a14)
	{
		USIZE args[] = {a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14};
		register USIZE r_eax __asm__("eax") = (USIZE)entry.Ssn;
		__asm__ volatile(
			"pushl 52(%[args])\n"
			"pushl 48(%[args])\n"
			"pushl 44(%[args])\n"
			"pushl 40(%[args])\n"
			"pushl 36(%[args])\n"
			"pushl 32(%[args])\n"
			"pushl 28(%[args])\n"
			"pushl 24(%[args])\n"
			"pushl 20(%[args])\n"
			"pushl 16(%[args])\n"
			"pushl 12(%[args])\n"
			"pushl 8(%[args])\n"
			"pushl 4(%[args])\n"
			"pushl (%[args])\n"
			"pushl $0\n"
			"movl %[gadget], %%edx\n"
			"call *%%edx\n"
			"addl $60, %%esp\n"
			: "+a"(r_eax)
			: [gadget] "r"(entry.SyscallAddress), [args] "r"(args)
			: "ecx", "edx", "memory");
		return (NTSTATUS)r_eax;
	}

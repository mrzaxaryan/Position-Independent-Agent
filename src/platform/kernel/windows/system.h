/**
 * @file system.h
 * @brief Windows System Call Interface and Indirect Syscall Dispatch
 *
 * @details Provides the low-level system call mechanism for Windows NT. On
 * x86_64 and i386, system calls are executed via indirect syscall gadgets
 * found within ntdll.dll, avoiding direct calls into ntdll and bypassing
 * userland hooks. On ARM64, where the kernel validates that the SVC
 * instruction originates from within ntdll, calls are dispatched directly
 * through the resolved ntdll stub address.
 *
 * The System Service Number (SSN) and a syscall gadget address are resolved
 * at runtime by scanning the ntdll export table for the target Zw* function,
 * then extracting the SSN from the function's prologue bytes and locating a
 * nearby syscall/sysenter instruction to use as the indirect call target.
 *
 * The System::Call overloads accept 0-14 arguments and set up the correct
 * calling convention for each architecture:
 * - x86_64: rcx->r10 swap, args in r10/rdx/r8/r9 + stack shadow space
 * - i386: all args pushed onto the stack, EAX holds SSN
 * - ARM64: args in x0-x7 + stack, BLR to ntdll stub address in x16
 * - ARM32: args in r0-r3 + stack, BLX to ntdll stub address in r12
 *
 * @see Using Nt and Zw Versions of the Native System Services Routines
 *      https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/using-nt-and-zw-versions-of-the-native-system-services-routines
 * @see System Service Descriptor Table (SSDT)
 *      https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/system-service-dispatch-tables
 */

#pragma once

#include "platform/platform.h"
#include "platform/kernel/windows/peb.h"

/** @brief Sentinel value indicating that a valid SSN could not be resolved. */
#define SYSCALL_SSN_INVALID ((INT32) - 1)

/**
 * @brief Holds a resolved System Service Number and its associated syscall gadget address.
 *
 * @details Produced by System::ResolveSyscallEntry after scanning the ntdll
 * export for the target Zw* function. On x86_64/i386 the SSN is extracted
 * from the function prologue (mov eax, <SSN>) and the syscall address points
 * to a syscall/sysenter instruction within ntdll. On ARM64 the syscall
 * address points to the ntdll stub itself (SVC #N; RET).
 *
 * If resolution fails, Ssn is set to SYSCALL_SSN_INVALID and SyscallAddress
 * is nullptr.
 */
typedef struct SYSCALL_ENTRY
{
	INT32 Ssn;           ///< System Service Number (index into the SSDT), or SYSCALL_SSN_INVALID
	PVOID SyscallAddress; ///< Address of the syscall/sysenter gadget in ntdll, or the ntdll stub on ARM64
} SYSCALL_ENTRY;

/**
 * @brief Provides indirect system call dispatch for Windows NT Native API functions.
 *
 * @details The System class resolves SSNs at runtime and dispatches system
 * calls through architecture-specific inline assembly. This avoids direct
 * calls into ntdll, making the calls invisible to userland API hooks.
 *
 * Usage pattern:
 * 1. Resolve: `auto entry = ResolveSyscall("ZwFunctionName");`
 * 2. Validate: Check `entry.Ssn != SYSCALL_SSN_INVALID`
 * 3. Dispatch: `NTSTATUS status = System::Call(entry, arg1, arg2, ...);`
 *
 * @see Using Nt and Zw Versions of the Native System Services Routines
 *      https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/using-nt-and-zw-versions-of-the-native-system-services-routines
 */
class System
{
public:
	/**
	 * @brief Resolves the SSN and syscall gadget address for an NT Native API function.
	 *
	 * @details Locates the target Zw* function in ntdll's export table using
	 * a DJB2 hash of the function name, then extracts the System Service
	 * Number from the function's prologue and locates a suitable syscall
	 * gadget for indirect invocation.
	 *
	 * On ARM64, the syscall address is the ntdll stub itself, since the
	 * kernel requires SVC to originate from within ntdll.
	 *
	 * @param functionNameHash DJB2 hash of the target Zw* function name.
	 *
	 * @return SYSCALL_ENTRY with the resolved SSN and gadget address, or
	 *         {SYSCALL_SSN_INVALID, nullptr} if resolution fails.
	 */
	static SYSCALL_ENTRY ResolveSyscallEntry(UINT64 functionNameHash);

#if defined(ARCHITECTURE_X86_64)
#include "platform/kernel/windows/system.x86_64.h"
#elif defined(ARCHITECTURE_I386)
#include "platform/kernel/windows/system.i386.h"
#elif defined(ARCHITECTURE_AARCH64)
#include "platform/kernel/windows/system.aarch64.h"
#elif defined(ARCHITECTURE_ARMV7A)
#include "platform/kernel/windows/system.armv7a.h"
#else
#error "Unsupported architecture"
#endif
};

/**
 * @brief Convenience macro that resolves a SYSCALL_ENTRY by function name string.
 *
 * @details Computes the DJB2 hash of the function name at compile time and
 * passes it to System::ResolveSyscallEntry for runtime resolution. The hash
 * is embedded directly in the .text section, avoiding string literal storage
 * in data sections.
 *
 * @param functionName The Zw* function name as a string literal (e.g., "ZwClose").
 *
 * @return SYSCALL_ENTRY with the resolved SSN and gadget address.
 */
#define ResolveSyscall(functionName) System::ResolveSyscallEntry(Djb2::HashCompileTime(functionName))

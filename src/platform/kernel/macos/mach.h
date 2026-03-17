/**
 * @file mach.h
 * @brief Mach kernel trap types and helpers for macOS/iOS
 *
 * @details Provides Mach trap numbers, IPC message types, and inline-asm helpers
 * for invoking Mach kernel traps directly (without libSystem). Mach traps use
 * negative syscall numbers: on x86_64 the number goes in RAX, on AArch64 in X16,
 * and the trap is invoked via the standard syscall/svc instruction.
 *
 * Unlike BSD syscalls, Mach traps do NOT set the carry flag on error — they
 * return kern_return_t directly in the result register. The existing
 * System::Call carry-flag normalization is harmless (carry is always clear).
 *
 * @see XNU osfmk/mach/mach_traps.h
 * @see XNU osfmk/mach/message.h
 */
#pragma once

#include "core/types/primitives.h"
#include "platform/kernel/macos/system.h"

// =============================================================================
// Mach trap numbers (negative — dispatched via kernel trap table)
// =============================================================================

/// @brief Return the calling task's Mach port name
constexpr SSIZE MACH_TRAP_TASK_SELF = -28;

/// @brief Allocate a new reply port for the calling thread
constexpr SSIZE MACH_TRAP_REPLY_PORT = -26;

/// @brief Send/receive a Mach IPC message (7 arguments)
constexpr SSIZE MACH_TRAP_MACH_MSG = -31;

// =============================================================================
// Mach IPC constants
// =============================================================================

/// @brief kern_return_t success
constexpr UINT32 KERN_SUCCESS = 0;

/// @brief mach_msg option: send a message
constexpr UINT32 MACH_SEND_MSG = 0x00000001;

/// @brief mach_msg option: receive a message
constexpr UINT32 MACH_RCV_MSG = 0x00000002;

/// @brief No timeout for mach_msg
constexpr UINT32 MACH_MSG_TIMEOUT_NONE = 0;

/// @brief Null Mach port
constexpr UINT32 MACH_PORT_NULL = 0;

/// @brief Port right type: copy-send right (for remote port)
constexpr UINT32 MACH_MSG_TYPE_COPY_SEND = 19;

/// @brief Port right type: make send-once right (for local/reply port)
constexpr UINT32 MACH_MSG_TYPE_MAKE_SEND_ONCE = 21;

/// @brief Compose msgh_bits from remote and local disposition
constexpr UINT32 MACH_MSGH_BITS(UINT32 remote, UINT32 local)
{
	return (remote & 0x1Fu) | ((local & 0x1Fu) << 8u);
}

// =============================================================================
// Mach IPC message header
// =============================================================================

/// @brief Mach IPC message header (24 bytes, same layout on 32-bit and 64-bit)
struct MachMsgHeader
{
	UINT32 Bits;        ///< Message bits (port dispositions, complex flag)
	UINT32 Size;        ///< Total message size in bytes (header + body)
	UINT32 RemotePort;  ///< Destination port name
	UINT32 LocalPort;   ///< Reply port name
	UINT32 VoucherPort; ///< Voucher port name (usually MACH_PORT_NULL)
	INT32 Id;           ///< MIG message identifier (routine number)
};

/// @brief NDR record for MIG messages (Network Data Representation)
/// @details Little-endian, ASCII, IEEE float — matches all Apple platforms
struct NdrRecord
{
	UINT8 MigVers;     ///< MIG version (0)
	UINT8 IfVers;      ///< Interface version (0)
	UINT8 Reserved1;   ///< Reserved (0)
	UINT8 MigEncoding; ///< Character encoding (0 = ASCII)
	UINT8 IntRep;      ///< Integer representation (1 = little-endian)
	UINT8 CharRep;     ///< Character representation (0 = ASCII)
	UINT8 FloatRep;    ///< Float representation (0 = IEEE 754)
	UINT8 Reserved2;   ///< Reserved (0)
};

// =============================================================================
// task_info structures
// =============================================================================

/// @brief task_info flavor for dyld image information
constexpr UINT32 TASK_DYLD_INFO = 17;

/// @brief Count of natural_t words in task_dyld_info
constexpr UINT32 TASK_DYLD_INFO_COUNT = 5;

/// @brief MIG message ID for task_info request
/// @details Subsystem "task" starts at 3400; task_info is routine index 5
constexpr INT32 TASK_INFO_MSG_ID = 3405;

/// @brief Dyld image info format: 64-bit addresses
constexpr UINT32 TASK_DYLD_ALL_IMAGE_INFO_64 = 1;

/// @brief Result of task_info(TASK_DYLD_INFO)
struct TaskDyldInfo
{
	UINT64 AllImageInfoAddr;   ///< Address of dyld_all_image_infos in task
	UINT64 AllImageInfoSize;   ///< Size of the structure
	UINT32 AllImageInfoFormat; ///< TASK_DYLD_ALL_IMAGE_INFO_32 or _64
};

// =============================================================================
// Mach trap helpers
// =============================================================================

/// @brief Get the current task's Mach port name
/// @return Task port name (positive integer)
static FORCE_INLINE UINT32 MachTaskSelf()
{
	SSIZE result = System::Call((USIZE)(SSIZE)MACH_TRAP_TASK_SELF);
	return (UINT32)result;
}

/// @brief Allocate a reply port for the calling thread
/// @return Reply port name (positive integer)
static FORCE_INLINE UINT32 MachReplyPort()
{
	SSIZE result = System::Call((USIZE)(SSIZE)MACH_TRAP_REPLY_PORT);
	return (UINT32)result;
}

// =============================================================================
// mach_msg_trap — 7-argument Mach trap via custom inline asm
// =============================================================================

/// @brief Send and/or receive a Mach IPC message
/// @param msg Pointer to the message buffer (used for both send and receive)
/// @param option MACH_SEND_MSG, MACH_RCV_MSG, or both
/// @param sendSize Size of the outgoing message in bytes
/// @param rcvSize Size of the receive buffer in bytes
/// @param rcvName Port to receive on (usually the reply port)
/// @param timeout Timeout in milliseconds (0 = MACH_MSG_TIMEOUT_NONE)
/// @param notify Notification port (usually MACH_PORT_NULL)
/// @return 0 (MACH_MSG_SUCCESS) on success, mach_msg_return_t error otherwise
static NOINLINE SSIZE MachMsgTrap(
	PVOID msg, UINT32 option, UINT32 sendSize, UINT32 rcvSize,
	UINT32 rcvName, UINT32 timeout, UINT32 notify)
{
#if defined(ARCHITECTURE_X86_64)
	register USIZE r_rdi __asm__("rdi") = (USIZE)msg;
	register USIZE r_rsi __asm__("rsi") = (USIZE)option;
	register USIZE r_rdx __asm__("rdx") = (USIZE)sendSize;
	register USIZE r_r10 __asm__("r10") = (USIZE)rcvSize;
	register USIZE r_r8 __asm__("r8") = (USIZE)rcvName;
	register USIZE r_r9 __asm__("r9") = (USIZE)timeout;
	register USIZE r_rax __asm__("rax") = (USIZE)(SSIZE)MACH_TRAP_MACH_MSG;
	__asm__ volatile(
		"pushq %[notify]\n"
		"syscall\n"
		"addq $8, %%rsp\n"
		: "+r"(r_rax), "+r"(r_rdx)
		: "r"(r_rdi), "r"(r_rsi), "r"(r_r10), "r"(r_r8), "r"(r_r9),
		  [notify]"r"((USIZE)notify)
		: "rcx", "r11", "memory", "cc"
	);
	return (SSIZE)r_rax;
#elif defined(ARCHITECTURE_AARCH64)
	register USIZE x0 __asm__("x0") = (USIZE)msg;
	register USIZE x1 __asm__("x1") = (USIZE)option;
	register USIZE x2 __asm__("x2") = (USIZE)sendSize;
	register USIZE x3 __asm__("x3") = (USIZE)rcvSize;
	register USIZE x4 __asm__("x4") = (USIZE)rcvName;
	register USIZE x5 __asm__("x5") = (USIZE)timeout;
	register USIZE x6 __asm__("x6") = (USIZE)notify;
	register USIZE x16 __asm__("x16") = (USIZE)(SSIZE)MACH_TRAP_MACH_MSG;
	__asm__ volatile(
		"svc #0x80\n"
		: "+r"(x0), "+r"(x1)
		: "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x6), "r"(x16)
		: "memory", "cc"
	);
	return (SSIZE)x0;
#else
#error "MachMsgTrap: unsupported architecture"
#endif
}

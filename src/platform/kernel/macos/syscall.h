/**
 * @file syscall.h
 * @brief macOS syscall numbers and BSD type definitions.
 *
 * @details Defines macOS BSD syscall numbers (class 2, 0x2000000 prefix), POSIX/BSD
 * constants, file descriptor flags, memory protection flags, socket options,
 * errno values, and kernel structures (BsdDirent64, Timeval, Pollfd). Syscall
 * numbers are shared across both x86_64 and AArch64 macOS architectures.
 * Note that many constant values differ from Linux (e.g., O_CREAT, MAP_ANONYMOUS,
 * SOL_SOCKET, EINPROGRESS).
 */
#pragma once

#include "core/types/primitives.h"

// BSD syscall class prefix
constexpr USIZE SYSCALL_CLASS_UNIX = 0x2000000;

// File I/O
constexpr USIZE SYS_EXIT       = SYSCALL_CLASS_UNIX | 1;
constexpr USIZE SYS_FORK       = SYSCALL_CLASS_UNIX | 2;
constexpr USIZE SYS_READ       = SYSCALL_CLASS_UNIX | 3;
constexpr USIZE SYS_WRITE      = SYSCALL_CLASS_UNIX | 4;
constexpr USIZE SYS_OPEN       = SYSCALL_CLASS_UNIX | 5;
constexpr USIZE SYS_CLOSE      = SYSCALL_CLASS_UNIX | 6;
constexpr USIZE SYS_LSEEK      = SYSCALL_CLASS_UNIX | 199;

// File operations
constexpr USIZE SYS_STAT64     = SYSCALL_CLASS_UNIX | 338;
constexpr USIZE SYS_FSTAT64    = SYSCALL_CLASS_UNIX | 339;
constexpr USIZE SYS_UNLINK     = SYSCALL_CLASS_UNIX | 10;

// Directory operations
constexpr USIZE SYS_MKDIR      = SYSCALL_CLASS_UNIX | 136;
constexpr USIZE SYS_RMDIR      = SYSCALL_CLASS_UNIX | 137;
constexpr USIZE SYS_GETDIRENTRIES64 = SYSCALL_CLASS_UNIX | 344;

// *at syscalls (available on all macOS architectures)
constexpr USIZE SYS_OPENAT     = SYSCALL_CLASS_UNIX | 463;
constexpr USIZE SYS_UNLINKAT   = SYSCALL_CLASS_UNIX | 472;
constexpr USIZE SYS_MKDIRAT    = SYSCALL_CLASS_UNIX | 475;
constexpr USIZE SYS_FSTATAT64  = SYSCALL_CLASS_UNIX | 470;
constexpr SSIZE AT_FDCWD       = -2;
constexpr INT32 AT_REMOVEDIR   = 0x0080;

// Memory operations
constexpr USIZE SYS_MMAP       = SYSCALL_CLASS_UNIX | 197;
constexpr USIZE SYS_MUNMAP     = SYSCALL_CLASS_UNIX | 73;

// Socket operations
constexpr USIZE SYS_SOCKET     = SYSCALL_CLASS_UNIX | 97;
constexpr USIZE SYS_CONNECT    = SYSCALL_CLASS_UNIX | 98;
constexpr USIZE SYS_BIND       = SYSCALL_CLASS_UNIX | 104;
constexpr USIZE SYS_SENDTO     = SYSCALL_CLASS_UNIX | 133;
constexpr USIZE SYS_RECVFROM   = SYSCALL_CLASS_UNIX | 29;
constexpr USIZE SYS_SHUTDOWN   = SYSCALL_CLASS_UNIX | 134;
constexpr USIZE SYS_SETSOCKOPT = SYSCALL_CLASS_UNIX | 105;
constexpr USIZE SYS_GETSOCKOPT = SYSCALL_CLASS_UNIX | 118;
constexpr USIZE SYS_FCNTL      = SYSCALL_CLASS_UNIX | 92;
constexpr USIZE SYS_IOCTL      = SYSCALL_CLASS_UNIX | 54;
constexpr USIZE SYS_POLL       = SYSCALL_CLASS_UNIX | 230;

// Time operations
constexpr USIZE SYS_GETTIMEOFDAY   = SYSCALL_CLASS_UNIX | 116;

// Process operations
constexpr USIZE SYS_EXECVE     = SYSCALL_CLASS_UNIX | 59;
constexpr USIZE SYS_DUP2       = SYSCALL_CLASS_UNIX | 90;
constexpr USIZE SYS_SETSID     = SYSCALL_CLASS_UNIX | 147;
constexpr USIZE SYS_WAIT4      = SYSCALL_CLASS_UNIX | 7;
constexpr USIZE SYS_KILL       = SYSCALL_CLASS_UNIX | 37;
constexpr USIZE SYS_PIPE       = SYSCALL_CLASS_UNIX | 42;

// =============================================================================
// POSIX/BSD Constants
// =============================================================================

// Standard file descriptors
constexpr INT32 STDIN_FILENO  = 0;
constexpr INT32 STDOUT_FILENO = 1;
constexpr INT32 STDERR_FILENO = 2;

// File open flags (BSD values -- differ from Linux!)
constexpr INT32 O_RDONLY    = 0x0000;
constexpr INT32 O_WRONLY    = 0x0001;
constexpr INT32 O_RDWR      = 0x0002;
constexpr INT32 O_NONBLOCK  = 0x0004;
constexpr INT32 O_APPEND    = 0x0008;
constexpr INT32 O_CREAT     = 0x0200;
constexpr INT32 O_TRUNC     = 0x0400;
constexpr INT32 O_DIRECTORY  = 0x100000;

// lseek whence values
constexpr INT32 SEEK_SET = 0;
constexpr INT32 SEEK_CUR = 1;
constexpr INT32 SEEK_END = 2;

// File mode/permission bits (same as POSIX)
constexpr INT32 S_IRUSR = 0x0100;  // User read
constexpr INT32 S_IWUSR = 0x0080;  // User write
constexpr INT32 S_IXUSR = 0x0040;  // User execute
constexpr INT32 S_IRGRP = 0x0020;  // Group read
constexpr INT32 S_IWGRP = 0x0010;  // Group write
constexpr INT32 S_IXGRP = 0x0008;  // Group execute
constexpr INT32 S_IROTH = 0x0004;  // Others read
constexpr INT32 S_IWOTH = 0x0002;  // Others write
constexpr INT32 S_IXOTH = 0x0001;  // Others execute

// Directory entry types (same as BSD)
constexpr UINT8 DT_UNKNOWN = 0;
constexpr UINT8 DT_FIFO    = 1;
constexpr UINT8 DT_CHR     = 2;
constexpr UINT8 DT_DIR     = 4;
constexpr UINT8 DT_BLK     = 6;
constexpr UINT8 DT_REG     = 8;
constexpr UINT8 DT_LNK     = 10;
constexpr UINT8 DT_SOCK    = 12;

// Memory protection flags (same as POSIX)
constexpr INT32 PROT_READ  = 0x01;
constexpr INT32 PROT_WRITE = 0x02;
constexpr INT32 PROT_EXEC  = 0x04;

// Memory mapping flags (macOS values -- differ from Linux!)
constexpr INT32 MAP_PRIVATE   = 0x0002;
constexpr INT32 MAP_ANONYMOUS = 0x1000;
#define MAP_FAILED ((PVOID)(-1))

// Socket options (BSD values -- differ from Linux!)
constexpr INT32 SOL_SOCKET   = 0xFFFF;
constexpr INT32 SO_ERROR     = 0x1007;
constexpr INT32 SO_RCVTIMEO  = 0x1006;
constexpr INT32 SO_SNDTIMEO  = 0x1005;
constexpr INT32 IPPROTO_TCP  = 6;
constexpr INT32 TCP_NODELAY  = 1;

// fcntl commands
constexpr INT32 F_GETFL = 3;
constexpr INT32 F_SETFL = 4;

// errno values
constexpr INT32 EEXIST = 17;
constexpr INT32 EINPROGRESS = 36;

// poll event flags
constexpr INT16 POLLIN  = 0x0001;
constexpr INT16 POLLOUT = 0x0004;
constexpr INT16 POLLERR = 0x0008;
constexpr INT16 POLLHUP = 0x0010;

// PTY flags
constexpr INT32 O_NOCTTY  = 0x20000;
constexpr INT32 O_CLOEXEC = 0x1000000;

// Signal numbers
constexpr INT32 SIGKILL = 9;

// wait4 options
constexpr INT32 WNOHANG = 1;

// Invalid file descriptor
constexpr SSIZE INVALID_FD = -1;

// =============================================================================
// BSD Structures
// =============================================================================

/// @brief BSD directory entry returned by the getdirentries64 syscall.
struct BsdDirent64
{
	UINT64 Ino;    ///< Inode number
	UINT64 Seekoff; ///< Seek offset for the next entry
	UINT16 Reclen; ///< Total size of this record in bytes (including padding)
	UINT16 Namlen; ///< Length of the filename in bytes (excluding null terminator)
	UINT8 Type;    ///< File type (DT_REG, DT_DIR, DT_LNK, etc.)
	CHAR Name[];   ///< Null-terminated filename
};

/// @brief POSIX time value with microsecond precision, used for gettimeofday and socket timeouts.
///
/// @details The macOS kernel gettimeofday copies out user64_timeval for 64-bit processes:
/// both Sec and Usec are 8 bytes (int64_t), unlike the standard userspace timeval where
/// Usec is 4 bytes (__darwin_suseconds_t). Since PIR calls the raw syscall (bypassing
/// libc), we match the kernel layout.
struct Timeval
{
	SSIZE Sec;  ///< Seconds since the Unix epoch (1970-01-01T00:00:00Z)
	SSIZE Usec; ///< Microseconds (0 to 999,999)
};

/// @brief File descriptor entry for the poll syscall.
struct Pollfd
{
	INT32 Fd;      ///< File descriptor to monitor
	INT16 Events;  ///< Requested event bitmask (e.g., POLLOUT)
	INT16 Revents; ///< Returned event bitmask filled by the kernel (e.g., POLLERR, POLLHUP)
};

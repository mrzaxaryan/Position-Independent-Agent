/**
 * @file syscall.h
 * @brief Oracle Solaris 11.4 syscall numbers and type definitions.
 *
 * @details Defines Oracle Solaris 11.4 syscall numbers (sourced from
 * /etc/name_to_sysnum on the CI VM), POSIX constants, file descriptor flags,
 * memory protection flags, socket options, errno values, and kernel structures
 * (SolarisDirent64, Timespec, Pollfd). Solaris uses the carry flag to indicate
 * errors (like BSD/macOS), not negative return values (like Linux). Syscall
 * numbers are the same across all supported architectures (x86_64, i386,
 * AArch64), unlike Linux where they differ per architecture. Note that many
 * constant values differ from Linux (e.g., O_CREAT, MAP_ANONYMOUS, AT_FDCWD,
 * CLOCK_REALTIME, EINPROGRESS).
 *
 * IMPORTANT: Oracle Solaris 11.4 has a different syscall table from
 * illumos-gate. Many legacy syscalls (open, stat, fstat, unlink, mkdir,
 * rmdir) have been removed or repurposed. Use the *at variants (openat,
 * fstatat, unlinkat, mkdirat) exclusively.
 */
#pragma once

#include "core/types/primitives.h"

#if defined(ARCHITECTURE_X86_64) || defined(ARCHITECTURE_I386) || defined(ARCHITECTURE_AARCH64)

// File I/O
constexpr USIZE SYS_READ       = 3;
constexpr USIZE SYS_WRITE      = 4;
constexpr USIZE SYS_OPEN       = 5;
constexpr USIZE SYS_CLOSE      = 6;
constexpr USIZE SYS_LSEEK      = 19;
constexpr USIZE SYS_OPENAT     = 68;

// File operations
constexpr USIZE SYS_STAT       = 18;
constexpr USIZE SYS_FSTAT      = 28;
constexpr USIZE SYS_FSTATAT    = 66;
constexpr USIZE SYS_UNLINK     = 10;
constexpr USIZE SYS_UNLINKAT   = 76;

// Directory operations
constexpr USIZE SYS_MKDIR      = 80;
constexpr USIZE SYS_MKDIRAT    = 102;
constexpr USIZE SYS_RMDIR      = 79;
constexpr USIZE SYS_GETDENTS   = 81;   // LP64: native 64-bit dirent; ILP32: 32-bit dirent (use getdents64)
constexpr USIZE SYS_GETDENTS64 = 213;  // ILP32 only: 64-bit dirent for 32-bit processes

// I/O control
constexpr USIZE SYS_IOCTL      = 54;

// Memory operations
constexpr USIZE SYS_MMAP       = 115;
constexpr USIZE SYS_MUNMAP     = 117;

// Socket operations (Solaris uses SYS_so_* names)
constexpr USIZE SYS_SO_SOCKET    = 230;
constexpr USIZE SYS_SO_SOCKETPAIR = 231;
constexpr USIZE SYS_BIND         = 232;
constexpr USIZE SYS_LISTEN       = 233;
constexpr USIZE SYS_ACCEPT       = 234;
constexpr USIZE SYS_CONNECT      = 235;
constexpr USIZE SYS_SHUTDOWN     = 236;
constexpr USIZE SYS_RECV         = 237;
constexpr USIZE SYS_RECVFROM     = 238;
constexpr USIZE SYS_RECVMSG      = 239;
constexpr USIZE SYS_SEND         = 240;
constexpr USIZE SYS_SENDMSG      = 241;
constexpr USIZE SYS_SENDTO       = 242;
constexpr USIZE SYS_GETPEERNAME  = 243;
constexpr USIZE SYS_GETSOCKNAME  = 244;
constexpr USIZE SYS_GETSOCKOPT   = 245;
constexpr USIZE SYS_SETSOCKOPT   = 246;

// I/O multiplexing
constexpr USIZE SYS_POLLSYS    = 183;

// fcntl
constexpr USIZE SYS_FCNTL      = 62;

// Time operations
constexpr USIZE SYS_CLOCK_GETTIME = 191;

// Process operations
constexpr USIZE SYS_EXIT       = 1;
constexpr USIZE SYS_FORKSYS    = 142;  // Multiplexed: subcode 0 = fork, 1 = vfork, 2 = forkall
constexpr USIZE SYS_EXECVE     = 59;
constexpr USIZE SYS_PGRPSYS    = 39;   // Multiplexed: subcode 3 = setsid
constexpr USIZE SYS_KILL       = 37;
constexpr USIZE SYS_PIPE       = 42;
constexpr USIZE SYS_WAITID     = 107;

// Forksys subcodes
constexpr USIZE FORKSYS_FORK    = 0;
constexpr USIZE FORKSYS_VFORK   = 1;
constexpr USIZE FORKSYS_FORKALL = 2;

// Pgrpsys subcodes
constexpr USIZE PGRPSYS_GETPGRP = 0;
constexpr USIZE PGRPSYS_SETPGRP = 1;
constexpr USIZE PGRPSYS_GETSID  = 2;
constexpr USIZE PGRPSYS_SETSID  = 3;
constexpr USIZE PGRPSYS_GETPGID = 4;
constexpr USIZE PGRPSYS_SETPGID = 5;

#endif

// =============================================================================
// Solaris/illumos Constants
// =============================================================================

// Standard file descriptors
constexpr INT32 STDIN_FILENO  = 0;
constexpr INT32 STDOUT_FILENO = 1;
constexpr INT32 STDERR_FILENO = 2;

// File open flags (Solaris values -- differ from Linux!)
constexpr INT32 O_RDONLY    = 0x0000;
constexpr INT32 O_WRONLY    = 0x0001;
constexpr INT32 O_RDWR      = 0x0002;
constexpr INT32 O_NDELAY    = 0x04;
constexpr INT32 O_APPEND    = 0x08;     // Linux: 0x0400
constexpr INT32 O_NONBLOCK  = 0x80;     // Linux: 0x0800
constexpr INT32 O_CREAT     = 0x100;    // Linux: 0x0040
constexpr INT32 O_TRUNC     = 0x200;    // Same as Linux
constexpr INT32 O_EXCL      = 0x400;    // Linux: 0x0080
constexpr INT32 O_DIRECTORY  = 0x1000000; // Linux: 0x10000

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

// Memory protection flags (same as POSIX)
constexpr INT32 PROT_READ  = 0x01;
constexpr INT32 PROT_WRITE = 0x02;
constexpr INT32 PROT_EXEC  = 0x04;

// Memory mapping flags (Solaris values -- differ from Linux!)
constexpr INT32 MAP_SHARED    = 0x01;
constexpr INT32 MAP_PRIVATE   = 0x02;
constexpr INT32 MAP_ANONYMOUS = 0x100;  // Linux: 0x20
#define MAP_FAILED ((PVOID)(-1))

// AT constants
constexpr SSIZE AT_FDCWD     = (SSIZE)(INT32)0xffd19553; // Linux: -100, macOS: -2
constexpr INT32 AT_REMOVEDIR = 0x01;               // Linux: 0x200

// Clock IDs (Solaris values -- differ from Linux!)
constexpr INT32 CLOCK_REALTIME  = 3;   // Linux: 0
constexpr INT32 CLOCK_MONOTONIC = 4;   // Linux: 1

// Socket options (BSD-style values, same as macOS)
constexpr INT32 SOL_SOCKET   = 0xFFFF;
constexpr INT32 SO_ERROR     = 0x1007;
constexpr INT32 SO_RCVTIMEO  = 0x1006;
constexpr INT32 SO_SNDTIMEO  = 0x1005;
constexpr INT32 IPPROTO_TCP  = 6;
constexpr INT32 TCP_NODELAY  = 1;

// fcntl commands
constexpr INT32 F_DUPFD  = 0;
constexpr INT32 F_GETFL  = 3;
constexpr INT32 F_SETFL  = 4;
constexpr INT32 F_DUP2FD = 9;  // Solaris dup2 equivalent via fcntl

// errno values (Solaris-specific values)
constexpr INT32 EEXIST = 17;                    // same across all POSIX platforms
constexpr INT32 EINPROGRESS = 150;  // Linux: 115, macOS: 36

// Signal numbers
constexpr INT32 SIGKILL = 9;
constexpr INT32 SIGCHLD = 18;  // Linux: 17

// waitid constants
constexpr INT32 P_PID = 0;
constexpr INT32 WEXITED = 1;
constexpr INT32 WNOHANG = 0x40;

// poll event flags
constexpr INT16 POLLIN  = 0x0001;
constexpr INT16 POLLOUT = 0x0004;
constexpr INT16 POLLERR = 0x0008;
constexpr INT16 POLLHUP = 0x0010;

// PTY flags
constexpr INT32 O_NOCTTY  = 0x800;
constexpr INT32 O_CLOEXEC = 0x800000;

// Invalid file descriptor
constexpr SSIZE INVALID_FD = -1;

// =============================================================================
// Solaris Structures
// =============================================================================

/// @brief Solaris directory entry returned by getdents (LP64) / getdents64 (ILP32).
///
/// @details On LP64, getdents natively returns 64-bit ino_t/off_t matching this
/// layout. On ILP32, getdents64 returns the same layout. Unlike Linux's
/// LinuxDirent64, the Solaris variant has no Type field; file type must be
/// determined through a separate fstatat call.
struct SolarisDirent64
{
	UINT64 Ino;    ///< Inode number
	INT64 Off;     ///< Offset to the next entry in the directory stream
	UINT16 Reclen; ///< Total size of this record in bytes (including padding)
	CHAR Name[];   ///< Null-terminated filename
};

/// @brief POSIX time specification with nanosecond precision.
struct Timespec
{
	SSIZE Sec;  ///< Seconds since the Unix epoch (1970-01-01T00:00:00Z)
	SSIZE Nsec; ///< Nanoseconds (0 to 999,999,999)
};

/// @brief File descriptor entry for the pollsys syscall.
struct Pollfd
{
	INT32 Fd;      ///< File descriptor to monitor
	INT16 Events;  ///< Requested event bitmask (e.g., POLLOUT)
	INT16 Revents; ///< Returned event bitmask filled by the kernel (e.g., POLLERR, POLLHUP)
};

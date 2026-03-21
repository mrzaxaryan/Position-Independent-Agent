# Solaris Kernel Interface

Position-independent Solaris/illumos syscall layer supporting **3 architectures** with BSD-style carry-flag error semantics and SVR4-heritage syscall conventions.

## Architecture Support

| Architecture | Trap Instruction | Syscall Number | Arg Registers | Error Detection |
|---|---|---|---|---|
| **x86_64** | `syscall` | `RAX` | `RDI, RSI, RDX, R10, R8, R9` | Carry flag (`CF`) |
| **i386** | `int $0x91` | `EAX` | Stack-based (above `ESP`) | Carry flag (`CF`) |
| **AArch64** | `svc #0` | `X8` | `X0, X1, X2, X3, X4, X5` | Carry (via `X1`) |

**Note:** i386 Solaris uses `int $0x91` — NOT `int $0x80` (which is Linux/FreeBSD).

## File Map

```
solaris/
├── syscall.h              # Syscall numbers, Solaris constants, structures
├── system.h               # Architecture dispatcher
├── system.x86_64.h        # x86_64 inline assembly (0-6 args)
├── system.i386.h          # i386 inline assembly (0-6 args)
├── system.aarch64.h       # AArch64 inline assembly (0-6 args)
└── platform_result.h      # Carry-flag → Result<T, Error> conversion
```

## Error Model

Solaris uses the **carry-flag** error model (same as BSD). On error, the carry flag is set and `EAX`/`RAX` contains the positive errno. The `System::Call` wrappers negate the return value when carry is set.

## Syscalls

Syscall numbers are **shared across all Solaris architectures**.

### File I/O

| Syscall | Number | Purpose |
|---|---|---|
| `read` | 3 | Read from file descriptor |
| `write` | 4 | Write to file descriptor |
| `open` | 5 | Open file (legacy — removed in Solaris 11.4) |
| `close` | 6 | Close file descriptor |
| `lseek` | 19 | Reposition file offset |
| `openat` | 68 | Open relative to directory fd |
| `ioctl` | 54 | Device I/O control |

### File Operations

| Syscall | Number | Purpose |
|---|---|---|
| `fstatat` | 66 | Get file status relative to dir fd |
| `unlinkat` | 76 | Delete file relative to dir fd |

### Directory Operations

| Syscall | Number | Purpose |
|---|---|---|
| `mkdirat` | 102 | Create directory relative to dir fd |
| `getdents` | 81 | Read directory entries |
| `getdents64` | 213 | Read directory entries (64-bit) |

### Memory

| Syscall | Number | Purpose |
|---|---|---|
| `mmap` | 115 | Map memory pages |
| `munmap` | 117 | Unmap memory pages |

### Network

| Syscall | Number | Purpose |
|---|---|---|
| `socket` | 230 | Create socket |
| `connect` | 235 | Connect to remote |
| `bind` | 232 | Bind to local address |
| `sendto` | 242 | Send data |
| `recvfrom` | 238 | Receive data |
| `shutdown` | 244 | Shutdown connection |
| `setsockopt` | 245 | Set socket option |
| `getsockopt` | 246 | Get socket option |

### Process

| Syscall | Number | Purpose |
|---|---|---|
| `exit` | 1 | Exit process |
| `forksys` | 142 | Fork (subcodes: fork=0, vfork=1, forkall=2) |
| `execve` | 59 | Execute program |
| `pgrpsys` | 39 | Process group ops (getpgrp=0, setpgrp=1, getsid=2, setsid=3) |
| `kill` | 37 | Send signal |
| `pipe` | 42 | Create pipe |
| `waitid` | 107 | Wait for child |

### Other

| Syscall | Number | Purpose |
|---|---|---|
| `pollsys` | 183 | Poll with timeout |
| `clock_gettime` | 191 | Get clock time |

## Multiplexed Syscalls

Solaris uses **multiplexed syscalls** — a single syscall number with sub-operation codes:

### `forksys` (142)

```
Subcode 0: fork     — create child process
Subcode 1: vfork    — create child sharing address space
Subcode 2: forkall  — fork all LWPs (lightweight processes)
```

### `pgrpsys` (39)

```
Subcode 0: getpgrp  — get process group
Subcode 1: setpgrp  — set process group
Subcode 2: getsid   — get session ID
Subcode 3: setsid   — create new session
```

## Solaris-Specific Constants

Many constants differ significantly from Linux and BSD:

| Constant | Solaris | Linux | FreeBSD | Notes |
|---|---|---|---|---|
| `AT_FDCWD` | `0xffd19553` | `-100` | `-100` | Solaris-unique value |
| `AT_REMOVEDIR` | `0x01` | `0x200` | `0x0800` | Solaris-unique |
| `O_CREAT` | `0x100` | `0x040` | `0x200` | SVR4 heritage |
| `O_APPEND` | `0x08` | `0x400` | `0x08` | Same as FreeBSD |
| `O_NONBLOCK` | `0x80` | `0x800` | `0x04` | Unique |
| `O_DIRECTORY` | `0x1000000` | `0x10000` | `0x20000` | Very large value |
| `MAP_ANONYMOUS` | `0x100` | `0x20` | `0x1000` | Unique |
| `CLOCK_REALTIME` | `3` | `0` | `0` | Different ID |
| `CLOCK_MONOTONIC` | `4` | `1` | `4` | Same as FreeBSD |
| `EINPROGRESS` | `150` | `115` | `36` | Same as MIPS Linux |
| `SOL_SOCKET` | `0xFFFF` | `1` | `0xFFFF` | BSD-style |
| `SO_ERROR` | `0x1007` | `4` | `0x1007` | BSD-style |

### Legacy Syscall Removal

Oracle Solaris 11.4 removed many legacy syscalls. The following must use `*at` variants:

- `open` → `openat`
- `stat`/`fstat` → `fstatat`
- `unlink` → `unlinkat`
- `mkdir` → `mkdirat`
- `rmdir` → `unlinkat` with `AT_REMOVEDIR`

## Structures

| Structure | Fields | Notes |
|---|---|---|
| `SolarisDirent64` | `Ino`, `Off`, `Reclen`, `Name[]` | **No `Type` field** — must use `fstatat` to determine file type |
| `Timespec` | `Sec`, `Nsec` | Nanosecond precision |
| `Pollfd` | `Fd`, `Events`, `Revents` | Standard poll structure |

**Important:** `SolarisDirent64` lacks a `Type` field (unlike Linux's `d_type` and BSD's `d_type`). To determine whether an entry is a file or directory, a separate `fstatat` call is required.

# FreeBSD Kernel Interface

Position-independent FreeBSD syscall layer supporting **4 architectures** with BSD carry-flag error semantics.

## Architecture Support

| Architecture | Trap Instruction | Syscall Number | Arg Registers | Error Detection |
|---|---|---|---|---|
| **x86_64** | `syscall` | `RAX` | `RDI, RSI, RDX, R10, R8, R9` | Carry flag (`CF`) |
| **i386** | `int $0x80` | `EAX` | Stack-based (above `ESP`) | Carry flag (`CF`) |
| **AArch64** | `svc #0` | `X8` | `X0, X1, X2, X3, X4, X5` | Carry flag (via `X1`) |
| **RISC-V 64** | `ecall` | `T0 (X5)` | `A0-A5 (X10-X15)` | `T0` error indicator |

## File Map

```
freebsd/
├── syscall.h              # Syscall numbers, BSD constants, structures
├── system.h               # Architecture dispatcher
├── system.x86_64.h        # x86_64 inline assembly (0-6 args)
├── system.i386.h          # i386 inline assembly (0-6 args)
├── system.aarch64.h       # AArch64 inline assembly (0-6 args)
├── system.riscv64.h       # RISC-V 64 inline assembly (0-6 args)
└── platform_result.h      # Carry-flag → Result<T, Error> conversion
```

## Error Model

FreeBSD uses the **BSD carry-flag** model. On error, the carry flag is set and `EAX`/`RAX` contains the positive errno. The `System::Call` wrappers normalize this by negating the return value when the carry flag is set:

```
CF = 0  →  success (RAX = result)
CF = 1  →  failure (RAX = errno, negated to -errno by wrapper)
```

After normalization, `result::FromFreeBSD<T>()` converts the value identically to Linux (negative = error).

### x86_64 carry-flag handling:

```asm
syscall
jnc 1f          ; jump if no carry (success)
neg rax         ; negate errno to make it negative
1:
```

### RISC-V 64 error handling:

RISC-V has no carry flag. FreeBSD uses `T0` (X5) as an error indicator:

```
T0 = 0  →  success (A0 = result)
T0 != 0 →  failure (A0 = errno)
```

## Syscalls

Syscall numbers are **shared across all FreeBSD architectures** (unlike Linux).

### File I/O

| Syscall | Number | Purpose |
|---|---|---|
| `read` | 3 | Read from file descriptor |
| `write` | 4 | Write to file descriptor |
| `open` | 5 | Open file |
| `close` | 6 | Close file descriptor |
| `lseek` | 478 | Reposition file offset |
| `openat` | 499 | Open file relative to directory fd |
| `ioctl` | 54 | Device I/O control |

### File Operations

| Syscall | Number | Purpose |
|---|---|---|
| `stat` | 188 | Get file status |
| `fstat` | 551 | Get file status by fd |
| `fstatat` | 552 | Get file status relative to dir fd |
| `unlink` | 10 | Delete file |
| `unlinkat` | 503 | Delete file relative to dir fd |

### Directory Operations

| Syscall | Number | Purpose |
|---|---|---|
| `mkdir` | 136 | Create directory |
| `mkdirat` | 496 | Create directory relative to dir fd |
| `rmdir` | 137 | Remove directory |
| `getdirentries` | 554 | Read directory entries |

### Memory

| Syscall | Number | Purpose |
|---|---|---|
| `mmap` | 477 | Map memory pages |
| `munmap` | 73 | Unmap memory pages |

### Network

| Syscall | Number | Purpose |
|---|---|---|
| `socket` | 97 | Create socket |
| `connect` | 98 | Connect to remote |
| `bind` | 104 | Bind to local address |
| `sendto` | 133 | Send data |
| `recvfrom` | 29 | Receive data |
| `shutdown` | 134 | Shutdown connection |
| `setsockopt` | 105 | Set socket option |
| `getsockopt` | 118 | Get socket option |
| `fcntl` | 92 | File control |
| `poll` | 209 | Poll with timeout |

### Process

| Syscall | Number | Purpose |
|---|---|---|
| `exit` | 1 | Exit process |
| `fork` | 2 | Create child process |
| `execve` | 59 | Execute program |
| `dup2` | 90 | Duplicate fd |
| `wait4` | 7 | Wait for child |
| `kill` | 37 | Send signal |
| `setsid` | 147 | Create session |
| `pipe` | 42 | Create pipe |

### PTY / Time

| Syscall | Number | Purpose |
|---|---|---|
| `posix_openpt` | 504 | Open pseudo-terminal master |
| `clock_gettime` | 232 | Get clock time |
| `gettimeofday` | 116 | Get time of day |

## Constants

FreeBSD shares BSD heritage with macOS — many constant values are identical but differ from Linux:

| Constant | FreeBSD | Linux | Notes |
|---|---|---|---|
| `O_CREAT` | `0x0200` | `0x0040` | BSD vs Linux |
| `O_NONBLOCK` | `0x0004` | `0x0800` | BSD vs Linux |
| `O_DIRECTORY` | `0x20000` | `0x10000`/`0x4000` | Varies |
| `MAP_ANONYMOUS` | `0x1000` | `0x20` | BSD vs Linux |
| `SOL_SOCKET` | `0xFFFF` | `1` | BSD-style |
| `SO_ERROR` | `0x1007` | `4` | BSD-style |
| `CLOCK_MONOTONIC` | `4` | `1` | Different IDs |
| `EINPROGRESS` | `36` | `115` | Different errno |
| `AT_FDCWD` | `-100` | `-100` | Same |
| `AT_REMOVEDIR` | `0x0800` | `0x200` | Different |
| `O_NOCTTY` | `0x8000` | `0x100` | Different |

## Structures

| Structure | Fields | Notes |
|---|---|---|
| `FreeBsdDirent` | `Fileno`, `Off`, `Reclen`, `Type`, `Pad0`, `Namlen`, `Pad1`, `Name[]` | FreeBSD 12+ ABI; has `Namlen` field (macOS-style) |
| `Timespec` | `Sec`, `Nsec` | Nanosecond precision |
| `Pollfd` | `Fd`, `Events`, `Revents` | Standard poll structure |

## Architecture-Specific Notes

### x86_64

- Uses `syscall` instruction, same register convention as Linux x86_64
- `RDX` is clobbered by `rval[1]` (FreeBSD returns two values from some syscalls like `fork` and `pipe`)

### i386

- Uses `int $0x80` — same as Linux i386
- Arguments passed on the stack (above `ESP`), not in registers

### AArch64

- Uses `svc #0` instruction
- `X1` is clobbered by `rval[1]` (dual-return syscalls)

### RISC-V 64

- Uses `ecall` instruction
- Error flag in `T0` (X5) instead of carry flag
- Syscall number in `T0` (X5) as well (FreeBSD convention, not `A7` like Linux)

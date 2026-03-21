# Linux Kernel Interface

Position-independent Linux syscall layer supporting **7 architectures** — the widest architecture coverage of any platform in the project.

## Architecture Support

| Architecture | Trap Instruction | Syscall Number | Arg Registers | Clobbered |
|---|---|---|---|---|
| **x86_64** | `syscall` | `RAX` | `RDI, RSI, RDX, R10, R8, R9` | `RCX, R11` |
| **i386** | `int $0x80` | `EAX` | `EBX, ECX, EDX, ESI, EDI, EBP` | — |
| **AArch64** | `svc #0` | `X8` | `X0, X1, X2, X3, X4, X5` | — |
| **ARMv7-A** | `svc #0` | `R7` | `R0, R1, R2, R3, R4, R5` | — |
| **RISC-V 64** | `ecall` | `A7 (X17)` | `A0-A5 (X10-X15)` | — |
| **RISC-V 32** | `ecall` | `A7 (X17)` | `A0-A5 (X10-X15)` | — |
| **MIPS64** | `syscall` | `$V0 ($2)` | `$A0-$A5 ($4-$9)` | `$AT ($1)` |

## File Map

```
linux/
├── syscall.h              # Shared POSIX constants, structures, arch-selector
├── syscall.x86_64.h       # x86_64 syscall numbers
├── syscall.i386.h         # i386 syscall numbers
├── syscall.aarch64.h      # AArch64 syscall numbers
├── syscall.armv7a.h       # ARMv7-A syscall numbers
├── syscall.riscv64.h      # RISC-V 64 syscall numbers
├── syscall.riscv32.h      # RISC-V 32 syscall numbers
├── syscall.mips64.h       # MIPS64 syscall numbers
├── system.h               # Architecture dispatcher
├── system.x86_64.h        # x86_64 inline assembly (0-6 args)
├── system.i386.h          # i386 inline assembly (0-6 args)
├── system.aarch64.h       # AArch64 inline assembly (0-6 args)
├── system.armv7a.h        # ARMv7-A inline assembly (0-6 args)
├── system.riscv64.h       # RISC-V 64 inline assembly (0-6 args)
├── system.riscv32.h       # RISC-V 32 inline assembly (0-6 args)
├── system.mips64.h        # MIPS64 inline assembly (0-6 args)
└── platform_result.h      # Negative-errno → Result<T, Error> conversion
```

## Error Model

Linux uses the **negative return** model — on failure, the syscall returns `-errno` directly in the return register:

```
Return >= 0  →  success (value is the result)
Return < 0   →  failure (negate to get errno)
```

Converted via `result::FromLinux<T>(rawResult)` → `Result<T, Error>`.

## Syscall Dispatch

Each architecture provides `System::Call` overloads for 0 to 6 arguments. Example (x86_64):

```c
static NOINLINE SSIZE Call(USIZE number, USIZE arg1, USIZE arg2, USIZE arg3)
{
    register USIZE r_rdi __asm__("rdi") = arg1;
    register USIZE r_rsi __asm__("rsi") = arg2;
    register USIZE r_rdx __asm__("rdx") = arg3;
    register USIZE r_rax __asm__("rax") = number;
    __asm__ volatile("syscall\n" : "+r"(r_rax)
        : "r"(r_rdi), "r"(r_rsi), "r"(r_rdx) : "rcx", "r11", "memory");
    return (SSIZE)r_rax;
}
```

## Syscalls by Category

### File I/O

| Syscall | x86_64 | AArch64 | i386 | Purpose |
|---|---|---|---|---|
| `read` | 0 | 63 | 3 | Read from file descriptor |
| `write` | 1 | 64 | 4 | Write to file descriptor |
| `open` | 2 | — | 5 | Open file (legacy) |
| `openat` | 257 | 56 | 295 | Open file relative to directory fd |
| `close` | 3 | 57 | 6 | Close file descriptor |
| `lseek` | 8 | 62 | 19 | Reposition file offset |
| `ioctl` | 16 | 29 | 54 | Device I/O control |

### File Operations

| Syscall | x86_64 | AArch64 | Purpose |
|---|---|---|---|
| `stat`/`fstatat` | 4/262 | 79 | Get file status |
| `fstat` | 5 | 80 | Get file status by fd |
| `unlink`/`unlinkat` | 87/— | 35 | Delete file |
| `mkdir`/`mkdirat` | 83/— | 34 | Create directory |
| `rmdir` | 84 | — | Remove directory |
| `getdents64` | 217 | 61 | Read directory entries |

### Memory

| Syscall | x86_64 | AArch64 | Purpose |
|---|---|---|---|
| `mmap` | 9 | 222 | Map memory pages |
| `munmap` | 11 | 215 | Unmap memory pages |

### Network

| Syscall | x86_64 | AArch64 | Purpose |
|---|---|---|---|
| `socket` | 41 | 198 | Create socket |
| `connect` | 42 | 203 | Connect to remote |
| `bind` | 49 | 200 | Bind to local address |
| `sendto` | 44 | 206 | Send data |
| `recvfrom` | 45 | 207 | Receive data |
| `shutdown` | 48 | 210 | Shutdown connection |
| `setsockopt` | 54 | 208 | Set socket option |
| `getsockopt` | 55 | 209 | Get socket option |
| `ppoll` | 271 | 73 | Poll with timeout |
| `fcntl` | 72 | 25 | File control |

### Process

| Syscall | x86_64 | AArch64 | Purpose |
|---|---|---|---|
| `exit` | 60 | 93 | Exit thread |
| `exit_group` | 231 | 94 | Exit all threads |
| `fork`/`clone` | 57 | 220 | Create child process |
| `execve` | 59 | 221 | Execute program |
| `dup2`/`dup3` | 33 | 24 | Duplicate fd |
| `wait4` | 61 | 260 | Wait for child |
| `kill` | 62 | 129 | Send signal |
| `setsid` | 112 | 157 | Create session |
| `pipe`/`pipe2` | 22 | 59 | Create pipe |

### Other

| Syscall | x86_64 | AArch64 | Purpose |
|---|---|---|---|
| `clock_gettime` | 228 | 113 | Get clock time |
| `getrandom` | 318 | 278 | Get random bytes |

## Architecture-Specific Notes

### AArch64 / RISC-V 64 / RISC-V 32

These architectures use the **modern Linux syscall table** exclusively:
- No legacy `open` / `stat` / `unlink` / `mkdir` / `rmdir` — must use `openat`, `fstatat`, `unlinkat`, `mkdirat` with `AT_FDCWD` (-100)
- `clone` instead of `fork`
- `dup3` instead of `dup2`
- `pipe2` instead of `pipe`

### i386

- Uses `socketcall` (102) multiplexer for all socket operations — individual socket syscalls are dispatched via sub-numbers
- Uses `mmap2` instead of `mmap` (page-offset-based)
- Uses `fcntl64` instead of `fcntl`

### RISC-V 32

- No legacy 32-bit time syscalls — uses `clock_gettime64` and `ppoll_time64`
- `Timespec` and `Timeval` fields are `INT64` (not `SSIZE`) to match kernel ABI
- `SO_RCVTIMEO` = 66, `SO_SNDTIMEO` = 67 (time64 variants, not 20/21)

### MIPS64

- Syscall numbers start at **5000** (e.g., `read` = 5000, `open` = 5002)
- **Special error handling:** `$A3` register is the error flag (0 = success, non-zero = error), errno in `$V0` — differs from all other Linux architectures
- Inherited IRIX/SVR4 constant values:
  - `O_CREAT` = 0x100, `O_APPEND` = 0x08, `O_NONBLOCK` = 0x80
  - `SOL_SOCKET` = 0xFFFF, `SO_ERROR` = 0x1007
  - `MAP_ANONYMOUS` = 0x0800
  - `EINPROGRESS` = 150

## Shared Constants and Structures

### Constants (`syscall.h`)

| Category | Constants |
|---|---|
| **File descriptors** | `STDIN_FILENO` (0), `STDOUT_FILENO` (1), `STDERR_FILENO` (2) |
| **Open flags** | `O_RDONLY`, `O_WRONLY`, `O_RDWR`, `O_CREAT`, `O_TRUNC`, `O_APPEND`, `O_NONBLOCK`, `O_DIRECTORY` |
| **Permissions** | `S_IRUSR`, `S_IWUSR`, `S_IXUSR`, `S_IRGRP`, `S_IWGRP`, etc. |
| **Memory** | `PROT_READ`, `PROT_WRITE`, `PROT_EXEC`, `MAP_SHARED`, `MAP_PRIVATE`, `MAP_ANONYMOUS` |
| **Socket** | `SOL_SOCKET`, `SO_ERROR`, `SO_RCVTIMEO`, `SO_SNDTIMEO`, `IPPROTO_TCP`, `TCP_NODELAY` |
| **Poll** | `POLLIN`, `POLLOUT`, `POLLERR`, `POLLHUP` |
| **Signals** | `SIGKILL` (9), `WNOHANG` (1) |

### Structures

| Structure | Purpose |
|---|---|
| `LinuxDirent64` | Directory entry: `Ino`, `Off`, `Reclen`, `Type`, `Name[]` |
| `Timespec` | Nanosecond-precision time: `Sec`, `Nsec` (64-bit on RISC-V 32) |
| `Timeval` | Microsecond-precision time: `Sec`, `Usec` (64-bit on RISC-V 32) |
| `Pollfd` | Poll descriptor: `Fd`, `Events`, `Revents` |

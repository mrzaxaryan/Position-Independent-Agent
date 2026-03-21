# macOS (XNU) Kernel Interface

Position-independent macOS/XNU syscall layer supporting **x86_64** and **AArch64** (Apple Silicon), with BSD syscalls, Mach trap support, and dynamic framework resolution via dyld.

## Architecture Support

| Architecture | Trap Instruction | Syscall Number | Arg Registers | Error Detection |
|---|---|---|---|---|
| **x86_64** | `syscall` | `RAX` | `RDI, RSI, RDX, R10, R8, R9` | Carry flag (`CF`) |
| **AArch64** | `svc #0x80` | `X16` | `X0, X1, X2, X3, X4, X5` | Carry flag (via condition) |

**Important AArch64 differences from Linux:**
- Trap instruction is `svc #0x80` (not `svc #0`)
- Syscall number goes in `X16` (not `X8`)

## File Map

```
macos/
├── syscall.h              # BSD syscall numbers (class 2), constants, structures
├── system.h               # Architecture dispatcher
├── system.x86_64.h        # x86_64 inline assembly (0-6 args)
├── system.aarch64.h       # AArch64 inline assembly (0-6 args)
├── mach.h                 # Mach trap definitions and IPC structures
├── dyld.h                 # Dynamic framework resolution declarations
├── dyld.cc                # Dyld/Mach-O parsing implementation
└── platform_result.h      # Carry-flag → Result<T, Error> conversion
```

## Error Model

macOS uses the same **BSD carry-flag** model as FreeBSD. The `System::Call` wrappers negate the return value when carry is set, normalizing to the negative-errno convention.

## Syscall Number Format

XNU classifies syscalls by class. BSD syscalls use class 2 with the `0x2000000` prefix:

```c
constexpr USIZE SYSCALL_CLASS_UNIX = 0x2000000;
constexpr USIZE SYS_READ  = SYSCALL_CLASS_UNIX | 3;   // = 0x2000003
constexpr USIZE SYS_WRITE = SYSCALL_CLASS_UNIX | 4;   // = 0x2000004
```

Mach traps use **negative** numbers (class 1).

## BSD Syscalls

Syscall numbers are **shared across x86_64 and AArch64**.

### File I/O

| Syscall | Number | Purpose |
|---|---|---|
| `read` | 3 | Read from file descriptor |
| `write` | 4 | Write to file descriptor |
| `open` | 5 | Open file |
| `close` | 6 | Close file descriptor |
| `lseek` | 199 | Reposition file offset |
| `openat` | 463 | Open relative to directory fd |
| `ioctl` | 54 | Device I/O control |

### File / Directory Operations

| Syscall | Number | Purpose |
|---|---|---|
| `stat64` | 338 | Get file status (64-bit) |
| `fstat64` | 339 | Get file status by fd (64-bit) |
| `fstatat64` | 470 | Get file status relative to dir fd |
| `unlink` | 10 | Delete file |
| `unlinkat` | 472 | Delete relative to dir fd |
| `mkdir` | 136 | Create directory |
| `mkdirat` | 475 | Create directory relative to dir fd |
| `rmdir` | 137 | Remove directory |
| `getdirentries64` | 344 | Read directory entries |

### Memory / Network / Process

| Syscall | Number | Purpose |
|---|---|---|
| `mmap` | 197 | Map memory pages |
| `munmap` | 73 | Unmap memory pages |
| `socket` | 97 | Create socket |
| `connect` | 98 | Connect to remote |
| `bind` | 104 | Bind to local address |
| `sendto` | 133 | Send data |
| `recvfrom` | 29 | Receive data |
| `fork` | 2 | Create child process |
| `execve` | 59 | Execute program |
| `exit` | 1 | Exit process |
| `pipe` | 42 | Create pipe |
| `kill` | 37 | Send signal |
| `poll` | 230 | Poll with timeout |
| `gettimeofday` | 116 | Get time of day |

### macOS-Specific

| Constant | Value | Notes |
|---|---|---|
| `AT_FDCWD` | `-2` | Different from Linux (-100) and FreeBSD (-100) |
| `AT_REMOVEDIR` | `0x0080` | Different from Linux (0x200) and FreeBSD (0x0800) |
| `O_DIRECTORY` | `0x100000` | Different from FreeBSD (0x20000) |
| `O_NOCTTY` | `0x20000` | Same as FreeBSD |
| `O_CLOEXEC` | `0x1000000` | Same as FreeBSD |

## Mach Trap Support

**File:** `mach.h`

macOS provides Mach kernel traps alongside BSD syscalls. These use **negative syscall numbers**:

| Trap | Number | Purpose |
|---|---|---|
| `mach_task_self` | -28 | Get current task port |
| `mach_reply_port` | -26 | Allocate a reply port |
| `mach_msg` | -31 | Send/receive IPC message (7 args) |

Mach traps are invoked using the same `System::Call` mechanism but with negative numbers in the syscall register.

### Mach IPC

The `mach_msg` trap is used for inter-process communication with the XNU kernel. It supports:
- `MACH_SEND_MSG` — send a message
- `MACH_RCV_MSG` — receive a message
- Combined send+receive in a single call

This is used by the dyld resolution system to query `task_info` for locating the dyld image.

## Dynamic Framework Resolution (dyld)

**Files:** `dyld.h`, `dyld.cc`

macOS-specific mechanism for loading frameworks and resolving function addresses at runtime without any dependency on libSystem or dyld stubs.

### Resolution Flow

```
ResolveFrameworkFunction(L"CoreFoundation", hash("CFStringCreateWithCString"))
  │
  ├─ 1. mach_task_self()   → get task port
  ├─ 2. task_info(TASK_DYLD_INFO) via mach_msg → locate dyld image info
  ├─ 3. Parse dyld's Mach-O headers to find its symbol table
  ├─ 4. Locate _dlopen and _dlsym in dyld's export table
  ├─ 5. dlopen("CoreFoundation.framework/CoreFoundation")
  └─ 6. dlsym(handle, "CFStringCreateWithCString") → function pointer
```

### Mach-O Parsing Structures

| Structure | Purpose |
|---|---|
| `MachHeader64` | Mach-O header: `magic` (0xFEEDFACF), `cpuType`, `ncmds` |
| `LoadCommand` | Generic load command: `cmd` type, `cmdSize` |
| `SegmentCommand64` | Segment: `segName`, `vmAddr`, `vmSize`, `fileOff` |
| `SymtabCommand` | Symbol table: `symOff`, `nSyms`, `strOff`, `strSize` |
| `Nlist64` | Symbol entry: `strIndex`, `type`, `sect`, `value` |

## Constants (BSD Values)

macOS shares BSD heritage with FreeBSD. Most constant values are identical to FreeBSD:

| Constant | Value | Same as FreeBSD? |
|---|---|---|
| `O_CREAT` | `0x0200` | Yes |
| `O_NONBLOCK` | `0x0004` | Yes |
| `MAP_ANONYMOUS` | `0x1000` | Yes |
| `SOL_SOCKET` | `0xFFFF` | Yes |
| `SO_ERROR` | `0x1007` | Yes |
| `EINPROGRESS` | `36` | Yes |

## Structures

| Structure | Fields | Notes |
|---|---|---|
| `BsdDirent64` | `Ino`, `Seekoff`, `Reclen`, `Namlen`, `Type`, `Name[]` | macOS-specific layout |
| `Timeval` | `Sec` (8 bytes), `Usec` (8 bytes) | 64-bit fields for raw syscall ABI |
| `Pollfd` | `Fd`, `Events`, `Revents` | Standard poll structure |

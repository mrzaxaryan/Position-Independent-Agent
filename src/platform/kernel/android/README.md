# Android Kernel Interface

Position-independent Android syscall layer — re-exports Linux kernel definitions since Android runs on the Linux kernel with identical syscall ABIs.

## Architecture Support

Android inherits all architecture support from the Linux layer. The primary target architectures for Android are:

| Architecture | Trap Instruction | Syscall Number | Arg Registers |
|---|---|---|---|
| **AArch64** | `svc #0` | `X8` | `X0-X5` |
| **ARMv7-A** | `svc #0` | `R7` | `R0-R5` |
| **x86_64** | `syscall` | `RAX` | `RDI, RSI, RDX, R10, R8, R9` |
| **i386** | `int $0x80` | `EAX` | `EBX, ECX, EDX, ESI, EDI, EBP` |

## File Map

```
android/
├── syscall.h              # Re-exports Linux syscall definitions
├── system.h               # Re-exports Linux system dispatch
└── platform_result.h      # Re-exports Linux result conversion
```

## Relationship to Linux

Android uses the **Linux kernel** with the same syscall ABI. All files in this directory re-export the Linux definitions:

```c
// android/syscall.h
#include "platform/kernel/linux/syscall.h"

// android/system.h
#include "platform/kernel/linux/system.h"
```

## What's Different from Desktop Linux

While the syscall interface is identical, Android differs at higher levels:
- **Bionic libc** instead of glibc (but we bypass libc entirely via direct syscalls)
- **SELinux** mandatory access control — stricter policy enforcement
- **Seccomp-BPF** syscall filtering — some syscalls may be blocked by the sandbox
- Android-specific kernel extensions (binder IPC, ashmem, etc.) are not used by this runtime

For full syscall documentation, see the [Linux Kernel Interface](../linux/README.md).

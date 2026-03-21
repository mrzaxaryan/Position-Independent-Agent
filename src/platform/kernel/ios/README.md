# iOS (XNU) Kernel Interface

Position-independent iOS syscall layer for **AArch64** — re-exports macOS XNU definitions since iOS shares the same kernel and BSD syscall ABI.

## Architecture Support

| Architecture | Trap Instruction | Syscall Number | Arg Registers |
|---|---|---|---|
| **AArch64** | `svc #0x80` | `X16` | `X0, X1, X2, X3, X4, X5` |

## File Map

```
ios/
├── syscall.h              # Re-exports macOS syscall definitions
├── system.h               # Re-exports macOS system dispatch
├── system.aarch64.h       # Re-exports macOS AArch64 inline assembly
└── platform_result.h      # Re-exports macOS result conversion
```

## Relationship to macOS

iOS runs on the **XNU kernel** — the same kernel as macOS. The BSD syscall interface is identical:
- Same syscall numbers (class 2, `0x2000000` prefix)
- Same `svc #0x80` trap instruction on AArch64
- Same `X16` register for syscall number
- Same carry-flag error model
- Same BSD constants (`O_CREAT`, `SOL_SOCKET`, `AT_FDCWD`, etc.)

All files in this directory simply re-export the macOS definitions:

```c
// ios/syscall.h
#include "platform/kernel/macos/syscall.h"

// ios/system.h
#include "platform/kernel/macos/system.h"
```

## What's Different from macOS

While the kernel syscall interface is identical, iOS differs at higher levels:
- No dyld framework resolution (iOS apps use a different loading model)
- Restricted syscall access (sandboxing is stricter)
- AArch64 only (no x86_64 support)

For full syscall documentation, see the [macOS Kernel Interface](../macos/README.md).

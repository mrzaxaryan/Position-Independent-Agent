# PowerShell Loader

Windows shellcode loader for position-independent code. Downloads pre-built agents from GitHub Releases and executes in-process via function pointer. Architecture is auto-detected from the running PowerShell process.

**PowerShell 5.1+** (Desktop) or **PowerShell 7+** (Core), no third-party dependencies.

## Usage

```powershell
# Download latest preview build and run:
.\loader.ps1

# Download a specific release:
.\loader.ps1 -Tag v1.0.0
```

## How it works

Executes shellcode in the current PowerShell process:

1. Auto-detect architecture from `PROCESSOR_ARCHITECTURE`
2. Download matching `windows-<arch>.bin` from GitHub Releases
3. `VirtualAlloc` with `PAGE_READWRITE` (RW)
4. Copy shellcode into the allocation
5. `VirtualProtect` to `PAGE_EXECUTE_READ` (RX)
6. Invoke as `Func<int>` delegate (function pointer call)
7. `VirtualFree` on return

At no point is memory simultaneously writable and executable (W^X).

## Supported architectures

Architecture is determined automatically from the PowerShell process:

| PowerShell | Architecture | Shellcode |
|------------|--------------|-----------|
| 64-bit on x86_64 | AMD64 | `windows-x86_64.bin` |
| 64-bit on ARM64 | ARM64 | `windows-aarch64.bin` |
| 32-bit on x86 | x86 | `windows-i386.bin` |
| 32-bit on ARM | ARM | `windows-armv7a.bin` |

## SSL

SSL certificate verification is disabled for downloads — the loader downloads unsigned shellcode from public GitHub Releases, so verification adds no security value and breaks on hosts with outdated CA stores.

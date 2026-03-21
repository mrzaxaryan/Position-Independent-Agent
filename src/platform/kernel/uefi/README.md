# UEFI Kernel Interface

Position-independent UEFI 2.10 firmware interface supporting **x86_64** and **AArch64**. Unlike all other platforms, UEFI uses **firmware-provided function pointer tables** (protocols) rather than syscall instructions.

## Architecture Support

| Architecture | Calling Convention | Context Register | Context Access |
|---|---|---|---|
| **x86_64** | Microsoft x64 ABI (`ms_abi`) | `IA32_GS_BASE` MSR (0xC0000101) | `WRMSR` / `RDMSR` |
| **AArch64** | Standard AAPCS64 | `TPIDR_EL0` | `MSR` / `MRS` |

**Note:** x86_64 UEFI uses the **Microsoft** calling convention (`RCX, RDX, R8, R9`), not the System V ABI used by Linux/BSD. All function pointers are declared with `__attribute__((ms_abi))`.

## File Map

```
uefi/
├── efi_types.h                    # Core EFI types, GUIDs, status codes
├── efi_system_table.h             # EFI System Table, text I/O protocols
├── efi_boot_services.h            # Boot Services (memory, events, protocols)
├── efi_runtime_services.h         # Runtime Services (time, variables, reset)
├── efi_protocols.h                # Protocol GUID definitions
├── efi_context.h                  # EFI context structure and arch dispatcher
├── efi_context.x86_64.h           # x86_64 context register (GS_BASE MSR)
├── efi_context.aarch64.h          # AArch64 context register (TPIDR_EL0)
├── efi_file_protocol.h            # File system protocol (Open/Read/Write/Close)
├── efi_service_binding.h          # Network service binding (CreateChild/DestroyChild)
├── efi_simple_network_protocol.h  # Raw network access (Transmit/Receive)
├── efi_tcp4_protocol.h            # TCP/IPv4 protocol (Connect/Send/Receive)
├── efi_tcp6_protocol.h            # TCP/IPv6 protocol
├── efi_ip4_config2_protocol.h     # IPv4 configuration (DHCP/static)
├── efi_graphics_output_protocol.h # Framebuffer access (BLT/SetMode)
└── platform_result.h              # EFI_STATUS → Result<T, Error> conversion
```

## No Syscalls — Protocol Function Tables

UEFI operates in a fundamentally different model from OS kernels:

```
OS Kernel:     User code → syscall instruction → kernel dispatch
UEFI:          EFI app   → function pointer    → firmware implementation
```

The UEFI firmware provides functionality through **protocol interfaces** — structs containing function pointers that the application calls directly. Protocols are discovered via the Boot Services `LocateProtocol` or `HandleProtocol` functions.

## Entry Point and System Table

A UEFI application receives two parameters at entry:

```c
EFI_STATUS EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
```

The `EFI_SYSTEM_TABLE` is the root of all firmware services:

```
EFI_SYSTEM_TABLE
├── ConIn                  → Simple Text Input Protocol (keyboard)
├── ConOut                 → Simple Text Output Protocol (screen text)
├── BootServices           → EFI_BOOT_SERVICES (memory, events, protocols)
├── RuntimeServices        → EFI_RUNTIME_SERVICES (time, variables, reset)
└── ConfigurationTable[]   → ACPI, SMBIOS table pointers
```

## Boot Services

18+ firmware functions for memory, event, protocol, and image management:

### Memory Management

| Function | Purpose |
|---|---|
| `AllocatePages` | Allocate physical memory pages |
| `FreePages` | Free allocated pages |
| `AllocatePool` | Allocate from pool (heap-like) |
| `FreePool` | Free pool allocation |
| `GetMemoryMap` | Get physical memory map |

### Event Management

| Function | Purpose |
|---|---|
| `CreateEvent` | Create asynchronous event |
| `SetTimer` | Set timer on event |
| `WaitForEvent` | Wait for one of multiple events |
| `SignalEvent` | Signal an event |
| `CloseEvent` | Destroy event |

### Protocol Management

| Function | Purpose |
|---|---|
| `InstallProtocolInterface` | Install protocol on handle |
| `HandleProtocol` | Get protocol from handle |
| `LocateProtocol` | Find protocol by GUID |
| `OpenProtocol` | Open protocol with tracking |
| `CloseProtocol` | Close protocol handle |

### Image Management

| Function | Purpose |
|---|---|
| `LoadImage` | Load EFI image from device |
| `StartImage` | Start loaded image |
| `Exit` | Exit current image |
| `ExitBootServices` | Transition to runtime (OS takes control) |

### Utility

| Function | Purpose |
|---|---|
| `Stall` | Busy-wait delay (microseconds) |
| `SetWatchdogTimer` | Set/disable watchdog |
| `CopyMem` | Memory copy |
| `SetMem` | Memory fill |

## Runtime Services

11+ firmware functions available after `ExitBootServices`:

| Function | Purpose |
|---|---|
| `GetTime` / `SetTime` | Real-time clock |
| `GetWakeupTime` / `SetWakeupTime` | Wake alarm |
| `GetVariable` / `SetVariable` | NVRAM variables |
| `GetNextVariableName` | Enumerate variables |
| `ResetSystem` | Reboot/shutdown |
| `SetVirtualAddressMap` | Remap runtime services to virtual addresses |
| `UpdateCapsule` | Firmware update |

## File Protocol

`EFI_FILE_PROTOCOL` provides filesystem access through the Simple File System protocol:

| Function | Purpose |
|---|---|
| `Open` | Open file/directory |
| `Close` | Close file handle |
| `Delete` | Delete file |
| `Read` | Read file data |
| `Write` | Write file data |
| `GetPosition` / `SetPosition` | File cursor management |
| `GetInfo` / `SetInfo` | File metadata (size, attributes, timestamps) |
| `Flush` | Flush buffers |

File attributes: `EFI_FILE_READ_ONLY`, `EFI_FILE_HIDDEN`, `EFI_FILE_SYSTEM`, `EFI_FILE_DIRECTORY`, `EFI_FILE_ARCHIVE`.

## Network Protocols

### Simple Network Protocol (SNP)

Raw network frame access — lowest level:

| Function | Purpose |
|---|---|
| `Start` / `Stop` | Enable/disable NIC |
| `Initialize` / `Shutdown` | Initialize/shutdown NIC |
| `Transmit` / `Receive` | Send/receive raw frames |
| `GetStatus` | Check link/transmit status |
| `ReceiveFilters` | Set receive filter mask |

### TCP4 Protocol

Full TCP/IPv4 stack:

| Function | Purpose |
|---|---|
| `Configure` | Set local/remote address and port |
| `Connect` | Initiate TCP connection |
| `Accept` | Accept incoming connection |
| `Transmit` / `Receive` | Send/receive data |
| `Close` | Graceful close |
| `Poll` | Check for pending operations |

Connection states: `Closed`, `Listen`, `SynSent`, `SynReceived`, `Established`, `FinWait1`, `FinWait2`, `Closing`, `TimeWait`, `CloseWait`, `LastAck`.

Options: `ReceiveBufferSize`, `SendBufferSize`, `KeepAliveProbes`, `EnableNagle`, `TimeToLive`.

### TCP6 Protocol

Identical to TCP4 but for IPv6 — uses `EFI_IPv6_ADDRESS` (16-byte array).

### IPv4 Configuration 2

Network configuration management:

| Function | Purpose |
|---|---|
| `SetData` | Set configuration (policy, address, gateway, DNS) |
| `GetData` | Get configuration |
| `RegisterDataNotify` | Register for config changes |

Policies: `Static` (manual) or `Dhcp` (automatic).

### Service Binding Protocol

Generic protocol for network stack child handle management — used to create per-connection TCP handles:

| Function | Purpose |
|---|---|
| `CreateChild` | Create child handle for new connection |
| `DestroyChild` | Destroy child handle |

## Graphics Output Protocol (GOP)

Framebuffer access for screen capture and display:

| Function | Purpose |
|---|---|
| `QueryMode` | Get display mode info |
| `SetMode` | Change display mode |
| `Blt` | Block transfer (pixel operations) |

Pixel formats: `PixelRedGreenBlueReserved8BitPerColor`, `PixelBlueGreenRedReserved8BitPerColor`, `PixelBitMask`, `PixelBltOnly`.

BLT operations: `VideoFill`, `VideoToBltBuffer`, `BufferToVideo`, `VideoToVideo`.

## EFI Context

The `EFI_CONTEXT` structure stores the firmware state for the running application:

```c
struct EFI_CONTEXT {
    EFI_HANDLE ImageHandle;
    EFI_SYSTEM_TABLE *SystemTable;
    BOOL NetworkInitialized;
    BOOL DhcpConfigured;
    BOOL TcpStackReady;
};
```

This context is stored in an architecture-specific register so it can be accessed from any position-independent code:

- **x86_64:** Stored via `IA32_GS_BASE` MSR (0xC0000101) using `WRMSR`/`RDMSR`
  - Uses MSR path (not `WRGSBASE`/`RDGSBASE`) for compatibility with firmware that doesn't enable `CR4.FSGSBASE`
- **AArch64:** Stored in `TPIDR_EL0` (thread pointer register) using `MSR`/`MRS`

## Status Codes

UEFI uses `EFI_STATUS` (same width as `USIZE`). The high bit indicates error:

| Status | Value | Meaning |
|---|---|---|
| `EFI_SUCCESS` | 0 | Operation succeeded |
| `EFI_LOAD_ERROR` | bit63 \| 1 | Image load failure |
| `EFI_INVALID_PARAMETER` | bit63 \| 2 | Bad parameter |
| `EFI_UNSUPPORTED` | bit63 \| 3 | Not supported |
| `EFI_NOT_FOUND` | bit63 \| 14 | Not found |
| `EFI_ACCESS_DENIED` | bit63 \| 15 | Access denied |
| `EFI_TIMEOUT` | bit63 \| 18 | Timeout |
| `EFI_OUT_OF_RESOURCES` | bit63 \| 9 | Out of memory |

Warnings (positive, non-zero) also exist: `EFI_WARN_UNKNOWN_GLYPH`, `EFI_WARN_DELETE_FAILURE`, etc.

## Configuration Table GUIDs

| GUID | Purpose |
|---|---|
| `EFI_ACPI_20_TABLE_GUID` | ACPI 2.0+ RSDP |
| `EFI_ACPI_TABLE_GUID` | ACPI 1.0 RSDP |
| `SMBIOS_TABLE_GUID` | SMBIOS 2.x entry point |
| `SMBIOS3_TABLE_GUID` | SMBIOS 3.x entry point |

## Key Differences from OS Platforms

| Aspect | OS Platforms (Linux/FreeBSD/etc.) | UEFI |
|---|---|---|
| **Invocation** | Trap instruction (syscall/svc/ecall) | Direct function pointer call |
| **Discovery** | Fixed syscall numbers | Protocol GUID lookup |
| **Calling convention** | Platform ABI | Microsoft x64 ABI (x86_64) |
| **Error model** | Return value / carry flag | High-bit status code |
| **Networking** | Socket syscalls | Protocol-based TCP stack |
| **Filesystem** | Path-based syscalls | Protocol-based file handles |
| **Memory** | mmap/munmap | AllocatePages/FreePages/AllocatePool |

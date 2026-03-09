# Position-Independent Agent

A cross-platform remote agent built on the [Position-Independent Runtime (PIR)](https://github.com/mrzaxaryan/Position-Independent-Runtime). Compiles to fully position-independent, zero-dependency binaries that communicate over WebSocket.

## Features

- **WebSocket Communication** - Connects to a server and processes commands in a persistent loop
- **Command Protocol** - Binary protocol with the following command types:
  - `GetUUID` - Returns the agent's unique identifier
  - `GetDirectoryContent` - Lists directory entries for a given path
  - `GetFileContent` - Reads file content at a specified offset and length
  - `GetFileChunkHash` - Computes SHA-256 hash of a file chunk
- **Cross-Platform** - Builds for Windows, Linux, macOS, UEFI, and Solaris across x86, x86_64, ARM, and AArch64 architectures
- **Position-Independent** - Output binary is fully relocatable with no external dependencies

## Prerequisites

- [CMake](https://cmake.org/) 3.20+
- [Ninja](https://ninja-build.org/)
- [Clang/LLVM](https://llvm.org/) toolchain

## Building

```bash
# Clone with submodules
git clone --recursive https://github.com/mrzaxaryan/Position-Independent-Agent.git
cd Position-Independent-Agent

# Configure and build (pick a preset)
cmake --preset windows-x86_64-release
cmake --build --preset windows-x86_64-release
```

### Available Presets

| Platform | Architectures                                  | Build Types      |
|----------|------------------------------------------------|------------------|
| Windows  | i386, x86_64, armv7a, aarch64                  | debug, release   |
| Linux    | i386, x86_64, armv7a, aarch64, riscv32, riscv64, mips64 | debug, release |
| macOS    | x86_64, aarch64                                | debug, release   |
| UEFI     | x86_64, aarch64                                | debug, release   |
| Solaris  | i386, x86_64, aarch64                          | debug, release   |
| FreeBSD  | i386, x86_64, aarch64, riscv64                 | debug, release   |
| Android  | x86_64, armv7a, aarch64                        | debug, release   |
| iOS      | aarch64                                        | debug, release   |

Preset format: `<platform>-<arch>-<build_type>` (e.g., `linux-aarch64-debug`)

## Build System

This project shares its CMake build system, VSCode configuration, and preset structure with the [PIR runtime](https://github.com/mrzaxaryan/Position-Independent-Runtime). The runtime submodule provides `Toolchain.cmake`, `Common.cmake`, and `Target.cmake` which handle cross-compilation, source collection, and post-build artifact generation. When the runtime updates its build system or adds new platform/architecture support, this project's `CMakePresets.json` and `.vscode/` configs should be synced to match.

## Project Structure

```
├── CMakeLists.txt          # Project build configuration
├── CMakePresets.json        # Build presets for all platform/arch combinations
├── src/
│   ├── main.cc             # Entry point and WebSocket message loop
│   ├── commandsHandler.cc  # Command handler implementations
│   └── terminal.h          # Command types and handler declarations
└── runtime/                # PIR submodule (build system + runtime library)
```

## Supported Commands

All commands use a binary protocol over WebSocket. Each message starts with a `UINT8` command type byte followed by command-specific payload. Every response begins with a `UINT32` status code (`0` = success, non-zero = error).

### `GetUUID` (0x00)

Returns the agent's unique identifier.

- **Request**: No payload (command type byte only)
- **Response**: `UINT32 status` + `UUID` (16 bytes)

### `GetDirectoryContent` (0x01)

Lists all entries in a directory (excluding `.` and `..`).

- **Request**: `WCHAR[] directoryPath` (null-terminated wide string)
- **Response**: `UINT32 status` + `UINT64 entryCount` + `DirectoryEntry[entryCount]`

### `GetFileContent` (0x02)

Reads file content at a specified offset.

- **Request**: `UINT64 readCount` + `UINT64 offset` + `WCHAR[] filePath`
- **Response**: `UINT32 status` + `UINT64 bytesRead` + `UINT8[bytesRead]` (file data)

### `GetFileChunkHash` (0x03)

Computes a SHA-256 hash of a file chunk.

- **Request**: `UINT64 chunkSize` + `UINT64 offset` + `WCHAR[] filePath`
- **Response**: `UINT32 status` + `UINT8[32]` (SHA-256 digest)

## Configuration

The agent UUID and server URL are configured in the source:

- **Agent UUID**: Defined in `src/terminal.h` (`AGENT_UUID`)
- **Server URL**: Defined in `src/main.cc` (WebSocket endpoint)

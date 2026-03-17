#include "platform/system/machine_id.h"
#include "platform/fs/file.h"
#include "platform/console/logger.h"
#include "core/string/string.h"

/// @brief Getting MachineID from File
/// @param path Path to the file containing the machine ID
/// @param hasDashes Whether the machine ID contains dashes
/// @return Result containing the UUID on success, or an error on failure
static Result<UUID, Error> ReadMachineIdFromFile(const WCHAR *path, bool hasDashes)
{
	auto openResult = File::Open(path, File::ModeRead);
	if (!openResult)
		return Result<UUID, Error>::Err(Error(Error::None));

	File &file = openResult.Value();

	if (hasDashes)
	{
		// boot_id format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx (36 chars)
		UINT8 buf[37]{};
		auto readResult = file.Read(Span<UINT8>(buf, 36));
		if (!readResult || readResult.Value() < 36)
			return Result<UUID, Error>::Err(Error(Error::None));

		return UUID::FromString(Span<const CHAR>((const CHAR *)buf, 36));
	}

	// machine-id format: 32 hex characters, no dashes
	UINT8 buf[33]{};
	auto readResult = file.Read(Span<UINT8>(buf, 32));
	if (!readResult || readResult.Value() < 32)
		return Result<UUID, Error>::Err(Error(Error::None));

	// Format as 8-4-4-4-12 for UUID::FromString by inserting dashes.
	CHAR uuidStr[37]{};
	INT32 src = 0;
	INT32 dst = 0;
	for (INT32 i = 0; i < 32; i++)
	{
		if (i == 8 || i == 12 || i == 16 || i == 20)
			uuidStr[dst++] = '-';
		uuidStr[dst++] = (CHAR)buf[src++];
	}
	uuidStr[dst] = '\0';

	return UUID::FromString(Span<const CHAR>(uuidStr, 36));
}

/// @brief Getting the Machine UUID
/// @return UUID on success, or an error if it cannot be retrieved
Result<UUID, Error> GetMachineUUID()
{
	// Try /etc/machine-id first (systemd, 32-char hex, constant across reboots)
	auto result = ReadMachineIdFromFile(L"/etc/machine-id", false);
	if (result)
		return result;

#if defined(PLATFORM_ANDROID)
	// Android doesn't have /etc/machine-id.
	// Fall back to /proc/sys/kernel/random/boot_id (UUID format with dashes).
	// Note: boot_id changes on each reboot.
	result = ReadMachineIdFromFile(L"/proc/sys/kernel/random/boot_id", true);
	if (result)
		return result;
#endif

	LOG_ERROR("Failed to retrieve machine UUID from OS");
	return Result<UUID, Error>::Err(Error(Error::None));
}

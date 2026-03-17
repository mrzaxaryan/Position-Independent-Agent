/**
 * environment.cc - UEFI Environment Variable Stub
 *
 * UEFI does not have traditional environment variables.
 * This stub always returns empty/not found.
 */

#include "platform/system/environment.h"

USIZE Environment::GetVariable([[maybe_unused]] const CHAR* name, Span<CHAR> buffer) noexcept
{
	if (buffer.Size() > 0)
	{
		buffer[0] = '\0';
	}

	return 0;
}

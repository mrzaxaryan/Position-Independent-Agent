/**
 * @file environment.cc
 * @brief Shared POSIX environment variable implementation
 *
 * @details Linux reads environment variables from /proc/self/environ.
 * macOS, FreeBSD, and Solaris return 0 (not found) as they lack a simple
 * procfs-based mechanism in freestanding mode.
 *
 * Future enhancements:
 * - macOS: use sysctl(kern.procargs2) to read process environment
 * - FreeBSD: use sysctl(kern.proc.env) to read process environment
 * - Solaris: use /proc/self/psinfo + /proc/self/as or walk the initial stack
 */

#include "platform/system/environment.h"

#if defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)

#if defined(PLATFORM_ANDROID)
#include "platform/kernel/android/syscall.h"
#include "platform/kernel/android/system.h"
#else
#include "platform/kernel/linux/syscall.h"
#include "platform/kernel/linux/system.h"
#endif
#include "core/memory/memory.h"

// Helper to compare strings (case-sensitive for Linux)
static BOOL CompareEnvName(const CHAR *envEntry, const CHAR *name) noexcept
{
	while (*name != '\0')
	{
		if (*envEntry != *name)
		{
			return false;
		}
		envEntry++;
		name++;
	}

	// After name, should be '='
	return *envEntry == '=';
}

USIZE Environment::GetVariable(const CHAR *name, Span<CHAR> buffer) noexcept
{
	if (name == nullptr || buffer.Size() == 0)
	{
		return 0;
	}

	// Open /proc/self/environ
	const CHAR *procEnvPath = "/proc/self/environ";
#if defined(ARCHITECTURE_AARCH64) || defined(ARCHITECTURE_RISCV64) || defined(ARCHITECTURE_RISCV32)
	// aarch64/riscv only has openat syscall
	SSIZE fd = System::Call(SYS_OPENAT, (USIZE)-100, (USIZE)procEnvPath, 0, 0);
	if (fd < 0)
	{
		buffer[0] = '\0';
		return 0;
	}
#else
	SSIZE fd = System::Call(SYS_OPEN, (USIZE)procEnvPath, 0 /* O_RDONLY */, 0);
	if (fd < 0)
	{
		// Try openat with AT_FDCWD (-100) for newer kernels
		fd = System::Call(SYS_OPENAT, (USIZE)-100, (USIZE)procEnvPath, 0, 0);
		if (fd < 0)
		{
			buffer[0] = '\0';
			return 0;
		}
	}
#endif

	// Read environment block (entries separated by null bytes)
	CHAR envBuf[4096];
	SSIZE bytesRead = System::Call(SYS_READ, (USIZE)fd, (USIZE)envBuf, sizeof(envBuf) - 1);
	System::Call(SYS_CLOSE, (USIZE)fd);

	if (bytesRead <= 0)
	{
		buffer[0] = '\0';
		return 0;
	}

	envBuf[bytesRead] = '\0';

	// Search for the variable
	const CHAR *ptr = envBuf;
	const CHAR *end = envBuf + bytesRead;

	while (ptr < end && *ptr != '\0')
	{
		if (CompareEnvName(ptr, name))
		{
			// Find the '=' and skip past it
			const CHAR *value = ptr;
			while (*value != '=' && *value != '\0')
			{
				value++;
			}
			if (*value == '=')
			{
				value++; // Skip the '='

				// Copy value to buffer
				USIZE len = 0;
				while (*value != '\0' && len < buffer.Size() - 1)
				{
					buffer[len++] = *value++;
				}
				buffer[len] = '\0';
				return len;
			}
		}

		// Skip to next entry (after null terminator)
		while (ptr < end && *ptr != '\0')
		{
			ptr++;
		}
		ptr++; // Skip the null terminator
	}

	// Variable not found
	buffer[0] = '\0';
	return 0;
}

#else // macOS, FreeBSD, Solaris — stub implementation

USIZE Environment::GetVariable(const CHAR *name, Span<CHAR> buffer) noexcept
{
	if (name == nullptr || buffer.Size() == 0)
	{
		return 0;
	}

	// No /proc filesystem or equivalent available in freestanding mode.
	// Return empty result.
	buffer[0] = '\0';
	return 0;
}

#endif

/**
 * environment.cc - Windows Environment Variable Implementation
 *
 * Accesses environment variables directly from the PEB environment block.
 * Position-independent, no .rdata dependencies.
 */

#include "platform/system/environment.h"
#include "platform/kernel/windows/peb.h"
#include "core/memory/memory.h"

// Extended RTL_USER_PROCESS_PARAMETERS with Environment field
// The standard definition in peb.h doesn't include all fields
struct RTL_USER_PROCESS_PARAMETERS_EX
{
	UINT32 MaximumLength;
	UINT32 Length;
	UINT32 Flags;
	UINT32 DebugFlags;
	PVOID ConsoleHandle;
	UINT32 ConsoleFlags;
	PVOID StandardInput;
	PVOID StandardOutput;
	PVOID StandardError;
	UNICODE_STRING CurrentDirectory_DosPath;
	PVOID CurrentDirectory_Handle;
	UNICODE_STRING DllPath;
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
	PWCHAR Environment; // Pointer to environment block
};

// Helper to compare wide string with narrow string (case-insensitive for first part)
static BOOL CompareEnvName(const WCHAR *wide, const CHAR *narrow) noexcept
{
	while (*narrow != '\0')
	{
		WCHAR w = *wide;
		CHAR n = *narrow;

		// Convert to uppercase for comparison
		if (w >= L'a' && w <= L'z')
			w -= 32;
		if (n >= 'a' && n <= 'z')
			n -= 32;

		if (w != (WCHAR)n)
		{
			return false;
		}
		wide++;
		narrow++;
	}

	// After name, should be '='
	return *wide == L'=';
}

USIZE Environment::GetVariable(const CHAR *name, Span<CHAR> buffer) noexcept
{
	if (name == nullptr || buffer.Size() == 0)
	{
		return 0;
	}

	// Get PEB
	PPEB peb = GetCurrentPEB();
	if (peb == nullptr || peb->ProcessParameters == nullptr)
	{
		return 0;
	}

	// Get extended process parameters with Environment field
	RTL_USER_PROCESS_PARAMETERS_EX *params = (RTL_USER_PROCESS_PARAMETERS_EX *)peb->ProcessParameters;
	PWCHAR envBlock = params->Environment;

	if (envBlock == nullptr)
	{
		return 0;
	}

	// Environment block is a sequence of null-terminated wide strings
	// Format: NAME=VALUE\0NAME=VALUE\0...\0\0
	while (*envBlock != L'\0')
	{
		// Check if this is the variable we're looking for
		if (CompareEnvName(envBlock, name))
		{
			// Find the '=' and skip past it
			const WCHAR *value = envBlock;
			while (*value != L'=' && *value != L'\0')
			{
				value++;
			}
			if (*value == L'=')
			{
				value++; // Skip the '='

				// Copy value to buffer (convert wide to narrow)
				USIZE len = 0;
				while (*value != L'\0' && len < buffer.Size() - 1)
				{
					// Simple wide to narrow conversion (ASCII only)
					buffer[len++] = (CHAR)*value++;
				}
				buffer[len] = '\0';
				return len;
			}
		}

		// Skip to next variable
		while (*envBlock != L'\0')
		{
			envBlock++;
		}
		envBlock++; // Skip the null terminator
	}

	// Variable not found
	buffer[0] = '\0';
	return 0;
}

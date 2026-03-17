#include "platform/kernel/windows/pe.h"
#include "platform/kernel/windows/peb.h"
#include "platform/kernel/windows/ntdll.h"
#include "core/algorithms/djb2.h"

// Get the address of an exported function from a module base address
PVOID GetExportAddress(PVOID hModule, UINT64 functionNameHash)
{
	// Validate DOS header
	PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;
	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
		return nullptr;

	// Validate NT headers
	IMAGE_NT_HEADERS *ntHeader = (IMAGE_NT_HEADERS *)((PCHAR)dosHeader + dosHeader->e_lfanew);
	if (ntHeader->Signature != IMAGE_NT_SIGNATURE)
		return nullptr;
	
	// Use the proper index for exports
	UINT32 exportRva = ntHeader->OptionalHeader
						   .DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]
						   .VirtualAddress;

	// Check exportRva validity
	if (exportRva == 0)
		return nullptr;

	PIMAGE_EXPORT_DIRECTORY exportDirectory =
		(PIMAGE_EXPORT_DIRECTORY)((PCHAR)hModule + exportRva);

	// Pointers to the export directory arrays
	PUINT32 nameRvas = (PUINT32)((PCHAR)hModule + exportDirectory->AddressOfNames);
	PUINT32 funcRvas = (PUINT32)((PCHAR)hModule + exportDirectory->AddressOfFunctions);
	PUINT16 ordinals = (PUINT16)((PCHAR)hModule + exportDirectory->AddressOfNameOrdinals);

	for (UINT32 i = 0; i < exportDirectory->NumberOfNames; ++i)
	{
		// Get the name of the i-th exported function
		PCHAR currentName = (PCHAR)hModule + nameRvas[i];
		UINT64 currentNameHash = Djb2::Hash(currentName);
		if (currentNameHash == functionNameHash)
		{
			// Get the ordinal for this name
			UINT16 ordinal = ordinals[i];

			// Use ordinal as index into AddressOfFunctions
			UINT32 funcRva = funcRvas[ordinal];

			// Check for forwarded exports (RVA points inside export directory)
			UINT32 exportSize = ntHeader->OptionalHeader
				.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
			if (funcRva >= exportRva && funcRva < (exportRva + exportSize))
			{
				// Forwarded export string: "MODULE.FunctionName"
				PCHAR forwardStr = (PCHAR)hModule + funcRva;

				// Find the dot separator
				PCHAR dot = forwardStr;
				while (*dot != '\0' && *dot != '.')
					dot++;
				if (*dot != '.')
					return nullptr;

				// Build wide module name with .dll suffix for PEB lookup
				UINT32 moduleLen = (UINT32)(dot - forwardStr);
				WCHAR wideModuleName[64];
				if (moduleLen + 4 >= 64) // +4 for ".dll"
					return nullptr;
				for (UINT32 j = 0; j < moduleLen; j++)
					wideModuleName[j] = (WCHAR)forwardStr[j];
				wideModuleName[moduleLen] = L'.';
				wideModuleName[moduleLen + 1] = L'd';
				wideModuleName[moduleLen + 2] = L'l';
				wideModuleName[moduleLen + 3] = L'l';
				wideModuleName[moduleLen + 4] = L'\0';

				UINT64 targetModuleHash = Djb2::Hash(wideModuleName);
				UINT64 targetFuncHash = Djb2::Hash(dot + 1);

				// Get the target module base address from the PEB and validate it
				PVOID targetModule = GetModuleHandleFromPEB(targetModuleHash);
				if (targetModule == nullptr)
					return nullptr;

				return GetExportAddress(targetModule, targetFuncHash);
			}

			// Convert RVA → VA and return
			return (PVOID)((PCHAR)hModule + funcRva);
		}
	}

	return nullptr; // Function was not found
}

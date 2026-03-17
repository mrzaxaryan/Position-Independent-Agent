#include "platform/kernel/windows/peb.h"
#include "platform/platform.h"
#include "core/algorithms/djb2.h"
#include "platform/kernel/windows/pe.h"
#include "platform/kernel/windows/ntdll.h"

// Returns the current process's PEB pointer
PPEB GetCurrentPEB(VOID)
{
	PPEB peb;
#if defined(PLATFORM_WINDOWS_X86_64)

	__asm__("movq %%gs:%1, %0" : "=r"(peb) : "m"(*(PUINT64)(0x60)));

#elif defined(PLATFORM_WINDOWS_I386)

	__asm__("movl %%fs:%1, %0" : "=r"(peb) : "m"(*(PUINT32)(0x30)));

#elif defined(PLATFORM_WINDOWS_ARMV7A)

	__asm__("ldr %0, [r9, %1]" : "=r"(peb) : "i"(0x30));

#elif defined(PLATFORM_WINDOWS_AARCH64)

	__asm__("ldr %0, [x18, #%1]" : "=r"(peb) : "i"(0x60));

#else
#error Unsupported platform
#endif
	return peb;
}

// Get the base address of a module by its name
PVOID GetModuleHandleFromPEB(UINT64 moduleNameHash)
{
	// Get PEB and modules
	PPEB peb = GetCurrentPEB();
	PLIST_ENTRY list = &peb->LoaderData->InMemoryOrderModuleList;
	PLIST_ENTRY entry = list->Flink;

	// Traverse the loaded modules list to find the target module by name hash
	while (entry != list)
	{
		PLDR_DATA_TABLE_ENTRY module = CONTAINING_RECORD(entry, LDR_DATA_TABLE_ENTRY, InMemoryOrderModuleList);

		if (module->BaseDllName.Buffer != nullptr && Djb2::Hash(module->BaseDllName.Buffer) == moduleNameHash)
			return module->DllBase;

		// Move to the next module in the list
		entry = entry->Flink;
	}

	return nullptr;
}

// Get the address of an exported function from PEB module
PVOID ResolveExportAddressFromPebModule(UINT64 moduleNameHash, UINT64 functionNameHash)
{
	// Resolve the module handle
	PVOID moduleBase = GetModuleHandleFromPEB(moduleNameHash);
	// Validate the module handle
	if (moduleBase == nullptr)
		return nullptr;
	// Resolve the function address
	PVOID functionAddress = GetExportAddress(moduleBase, functionNameHash);
	return functionAddress;
}

// Get the address of an exported function using module name
PVOID ResolveExportAddress(const WCHAR *moduleName, UINT64 functionNameHash)
{
	// Compute module name hash from the wide string
	UINT64 moduleNameHash = Djb2::Hash(moduleName);

	// Fast path: module already loaded
	PVOID moduleBase = GetModuleHandleFromPEB(moduleNameHash);

	// Slow path: load module via LdrLoadDll
	if (moduleBase == nullptr)
	{
		UINT16 nameLen = (UINT16)StringUtils::Length(moduleName);

		UNICODE_STRING dllName;
		dllName.Length = nameLen * sizeof(WCHAR);
		dllName.MaximumLength = dllName.Length + sizeof(WCHAR);
		dllName.Buffer = (PWCHAR)moduleName;

		// Load the DLL and get the base address
		auto r = NTDLL::LdrLoadDll(nullptr, nullptr, &dllName, &moduleBase);
		if (!r || moduleBase == nullptr)
			return nullptr;
	}

	return GetExportAddress(moduleBase, functionNameHash);
}
#include "platform/kernel/windows/system.h"
#include "platform/platform.h"
#include "platform/kernel/windows/peb.h"
#include "core/algorithms/djb2.h"

SYSCALL_ENTRY System::ResolveSyscallEntry(UINT64 functionNameHash)
{
	SYSCALL_ENTRY result;
	result.Ssn            = SYSCALL_SSN_INVALID;
	result.SyscallAddress = nullptr;

	PVOID ntdllBase = GetModuleHandleFromPEB(Djb2::HashCompileTime(L"ntdll.dll"));
	if (!ntdllBase)
		return result;

	UINT8* base      = (UINT8*)ntdllBase;
	UINT8* ntHeaders = base + *(UINT32*)(base + 0x3C);

#if defined(ARCHITECTURE_X86_64) || defined(ARCHITECTURE_AARCH64)
	UINT32 exportDirRva  = *(UINT32*)(ntHeaders + 0x88);
	UINT32 exportDirSize = *(UINT32*)(ntHeaders + 0x8C);
#elif defined(ARCHITECTURE_I386) || defined(ARCHITECTURE_ARMV7A)
	UINT32 exportDirRva  = *(UINT32*)(ntHeaders + 0x78);
	UINT32 exportDirSize = *(UINT32*)(ntHeaders + 0x7C);
#else
#error Unsupported architecture
#endif

	if (exportDirRva == 0)
		return result;

	UINT8* exportDir = base + exportDirRva;

	UINT32  numberOfNames = *(UINT32*)(exportDir + 0x18);
	UINT32* funcRvaTable  = (UINT32*)(base + *(UINT32*)(exportDir + 0x1C));
	UINT32* nameRvaTable  = (UINT32*)(base + *(UINT32*)(exportDir + 0x20));
	UINT16* ordinalTable  = (UINT16*)(base + *(UINT32*)(exportDir + 0x24));

	for (UINT32 i = 0; i < numberOfNames; i++)
	{
		const CHAR* name = (const CHAR*)(base + nameRvaTable[i]);

		// Check for "Zw" prefix (0x5A='Z', 0x77='w')
		if (name[0] != 'Z' || name[1] != 'w')
			continue;

		if (Djb2::Hash(name) != functionNameHash)
			continue;

		UINT32 funcRva = funcRvaTable[ordinalTable[i]];

		// Skip forwarded exports
		if (funcRva >= exportDirRva && funcRva < (exportDirRva + exportDirSize))
			return result;

#if defined(ARCHITECTURE_X86_64)
		{
			// Each x64 stub contains an inline syscall;ret gadget (0F 05 C3)
			UINT8* funcAddr = base + funcRva;
			for (UINT32 k = 0; k < 26; k++)
			{
				if (funcAddr[k] == 0x0F && funcAddr[k + 1] == 0x05 && funcAddr[k + 2] == 0xC3)
				{
					result.SyscallAddress = (PVOID)(funcAddr + k);
					break;
				}
			}
			if (!result.SyscallAddress)
				return result;
		}
#elif defined(ARCHITECTURE_AARCH64)
		{
			// ARM64 ntdll stubs: SVC #N; RET (each instruction is 4 bytes)
			// The syscall number is encoded in the SVC immediate, NOT in a register.
			// We find the SVC+RET pair and BLR to it (indirect syscall).
			// SVC encoding: 0xD4000001 | (imm16 << 5), mask 0xFFE0001F
			// RET encoding: 0xD65F03C0
			UINT32* instrs = (UINT32*)(base + funcRva);
			for (UINT32 k = 0; k < 6; k++)
			{
				if ((instrs[k] & 0xFFE0001F) == 0xD4000001 && instrs[k + 1] == 0xD65F03C0)
				{
					result.SyscallAddress = (PVOID)&instrs[k];
					break;
				}
			}
			if (!result.SyscallAddress)
				return result;
		}
#elif defined(ARCHITECTURE_ARMV7A)
		{
			// ARM32 ntdll stubs (Thumb-2 mode):
			// MOV.W r12, #SSN    (variable length Thumb-2 encoding)
			// SVC   #1           (Thumb-2: 0xDF01)
			// BX    LR           (Thumb-2: 0x4770)
			//
			// We scan for the SVC #1 halfword to locate the stub, then set
			// the syscall address to the function entry with bit 0 set
			// for correct Thumb interworking via BLX.
			UINT8* funcAddr = base + funcRva;
			UINT16* hw = (UINT16*)funcAddr;
			for (UINT32 k = 0; k < 16; k++)
			{
				if (hw[k] == 0xDF01) // SVC #1 in Thumb encoding
				{
					// Set bit 0 for Thumb interworking
					result.SyscallAddress = (PVOID)((USIZE)funcAddr | 1);
					break;
				}
			}
			if (!result.SyscallAddress)
				return result;
		}
#elif defined(ARCHITECTURE_I386)
		{
			// i386 stubs have two known formats, both starting with B8 [SSN:4]:
			//
			// Old format: B8 [SSN:4] BA [addr:4] {FF12|FFD2} C2 [cleanup:2]
			//   Direct mov edx,addr then call [edx] or call edx.
			//
			// New format (Windows 11 WoW64): B8 [SSN:4] E8 00000000 5A ...
			//   Uses call $+5; pop edx to get EIP, conditional branch, then
			//   falls through or jumps to BA [addr:4] {FF12|FFD2} at offset 0x1F.
			UINT8* funcAddr = base + funcRva;
			if (funcAddr[0] != 0xB8)
				return result;

			// Locate the "mov edx, addr" (BA) instruction
			INT32 baOffset;
			if (funcAddr[5] == 0xBA)
				baOffset = 5;   // old format
			else if (funcAddr[5] == 0xE8 && *(UINT32*)(funcAddr + 6) == 0
					 && funcAddr[0xA] == 0x5A && funcAddr[0x1F] == 0xBA)
				baOffset = 0x1F; // new conditional format
			else
				return result;

			PVOID rawAddr = *(PVOID*)(funcAddr + baOffset + 1);

			if (funcAddr[baOffset + 5] == 0xFF && funcAddr[baOffset + 6] == 0x12)
				result.SyscallAddress = *(PVOID*)rawAddr;   // native: dereference pointer to KiFastSystemCall
			else if (funcAddr[baOffset + 5] == 0xFF && funcAddr[baOffset + 6] == 0xD2)
				result.SyscallAddress = rawAddr;            // WoW64: direct trampoline address
			else
				return result;

			// SSN is embedded directly in the stub — no counting needed
			result.Ssn = (INT32)(*(UINT32*)(funcAddr + 1));
			return result;
		}
#endif

		// Derive SSN by counting Zw* exports with lower RVA (x86_64/aarch64)
		result.Ssn = 0;
		for (UINT32 j = 0; j < numberOfNames; j++)
		{
			const CHAR* n = (const CHAR*)(base + nameRvaTable[j]);
			// Check for "Zw" prefix (0x5A='Z', 0x77='w')
			if (n[0] != 'Z' || n[1] != 'w')
				continue;

			UINT32 rva = funcRvaTable[ordinalTable[j]];
			if (rva >= exportDirRva && rva < (exportDirRva + exportDirSize))
				continue;

			if (rva < funcRva)
				result.Ssn++;
		}

		return result;
	}

	return result;
}

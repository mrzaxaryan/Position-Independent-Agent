/**
 * @file dyld.cc
 * @brief macOS dynamic library loader implementation
 *
 * @details Resolves framework functions in a position-independent context
 * (no libSystem) by:
 *
 * 1. Calling task_self_trap() and mach_reply_port() Mach traps
 * 2. Sending a task_info(TASK_DYLD_INFO) Mach IPC message to get
 *    the dyld_all_image_infos address
 * 3. Reading dyldImageLoadAddress from dyld_all_image_infos (version >= 2)
 * 4. Parsing dyld's Mach-O LC_SYMTAB to find _dlopen and _dlsym symbols
 * 5. Calling dlopen() to load frameworks, dlsym() to resolve functions
 *
 * This is the macOS equivalent of the Windows PEB module resolution
 * (src/platform/kernel/windows/peb.cc).
 *
 * @see XNU osfmk/mach/task.defs — task_info MIG definition
 * @see Apple Mach-O Reference — Symbol Table format
 */

#include "platform/kernel/macos/dyld.h"
#include "platform/kernel/macos/mach.h"
#include "platform/kernel/macos/syscall.h"
#include "platform/kernel/macos/system.h"
#include "core/memory/memory.h"
#include "core/string/string.h"

// =============================================================================
// MIG message structures for task_info
// =============================================================================

/// @brief MIG request message for task_info
struct TaskInfoRequest
{
	MachMsgHeader Header;
	NdrRecord NDR;
	UINT32 Flavor;
	UINT32 TaskInfoOutCnt;
};

/// @brief MIG reply message for task_info(TASK_DYLD_INFO)
struct TaskInfoReply
{
	MachMsgHeader Header;
	NdrRecord NDR;
	UINT32 RetCode;
	UINT32 TaskInfoOutCnt;
	UINT32 TaskInfoOut[TASK_DYLD_INFO_COUNT];
};

// =============================================================================
// dlopen / dlsym function pointer types
// =============================================================================

/// @brief dlopen function signature
typedef PVOID (*DlOpenFn)(const CHAR *path, INT32 mode);

/// @brief dlsym function signature
typedef PVOID (*DlSymFn)(PVOID handle, const CHAR *symbol);

/// @brief RTLD_LAZY — defer symbol resolution until first use
constexpr INT32 RTLD_LAZY = 1;

// =============================================================================
// Internal: Get dyld_all_image_infos via Mach IPC
// =============================================================================

/// @brief Query task_info(TASK_DYLD_INFO) to get the dyld_all_image_infos address
/// @return Pointer to DyldAllImageInfos, or nullptr on failure
static const DyldAllImageInfos *GetDyldAllImageInfos()
{
	UINT32 taskPort = MachTaskSelf();
	UINT32 replyPort = MachReplyPort();

	if (taskPort == 0 || replyPort == 0)
		return nullptr;

	// Build the task_info request message
	TaskInfoRequest req;
	Memory::Zero(&req, sizeof(req));

	req.Header.Bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_MAKE_SEND_ONCE);
	req.Header.Size = sizeof(TaskInfoRequest);
	req.Header.RemotePort = taskPort;
	req.Header.LocalPort = replyPort;
	req.Header.Id = TASK_INFO_MSG_ID;

	// NDR: little-endian, ASCII, IEEE float
	req.NDR.IntRep = 1;

	req.Flavor = TASK_DYLD_INFO;
	req.TaskInfoOutCnt = TASK_DYLD_INFO_COUNT;

	// Use a union so send and receive share the same buffer
	// (reply is larger than request)
	union
	{
		TaskInfoRequest Request;
		TaskInfoReply Reply;
	} msg;
	msg.Request = req;

	// Send request and receive reply
	SSIZE ret = MachMsgTrap(
		&msg,
		MACH_SEND_MSG | MACH_RCV_MSG,
		sizeof(TaskInfoRequest),
		sizeof(TaskInfoReply),
		replyPort,
		MACH_MSG_TIMEOUT_NONE,
		MACH_PORT_NULL);

	if (ret != 0)
		return nullptr;

	// Verify reply
	if (msg.Reply.RetCode != KERN_SUCCESS)
		return nullptr;

	// Extract the dyld_all_image_infos address from the reply
	TaskDyldInfo dyldInfo;
	Memory::Copy(&dyldInfo, msg.Reply.TaskInfoOut, sizeof(TaskDyldInfo));

	if (dyldInfo.AllImageInfoAddr == 0)
		return nullptr;

	return (const DyldAllImageInfos *)(USIZE)dyldInfo.AllImageInfoAddr;
}

// =============================================================================
// Internal: Compare segment names (16-byte fixed fields)
// =============================================================================

static BOOL SegNameEquals(const CHAR *segName, const CHAR *target)
{
	for (INT32 i = 0; i < 16; i++)
	{
		if (segName[i] != target[i])
			return false;
		if (segName[i] == '\0')
			return true;
	}
	return true;
}

// =============================================================================
// Internal: Find a symbol in a Mach-O image by name
// =============================================================================

/// @brief Search a Mach-O image's LC_SYMTAB for a named symbol
/// @param header Pointer to the Mach-O header in memory
/// @param symbolName Symbol name to find (e.g. "_dlopen")
/// @return Runtime address of the symbol, or nullptr if not found
static PVOID FindSymbolInMachO(const MachHeader64 *header, const CHAR *symbolName)
{
	if (header == nullptr || header->Magic != MH_MAGIC_64)
		return nullptr;

	// Walk load commands to find LC_SEGMENT_64(__TEXT), LC_SEGMENT_64(__LINKEDIT), LC_SYMTAB
	const UINT8 *cmdPtr = (const UINT8 *)header + sizeof(MachHeader64);
	const SegmentCommand64 *textSeg = nullptr;
	const SegmentCommand64 *linkeditSeg = nullptr;
	const SymtabCommand *symtab = nullptr;

	auto textName = "__TEXT";
	auto linkeditName = "__LINKEDIT";

	for (UINT32 i = 0; i < header->NCmds; i++)
	{
		const LoadCommand *lc = (const LoadCommand *)cmdPtr;

		if (lc->Cmd == LC_SEGMENT_64)
		{
			const SegmentCommand64 *seg = (const SegmentCommand64 *)lc;
			if (SegNameEquals(seg->SegName, (const CHAR *)textName))
				textSeg = seg;
			else if (SegNameEquals(seg->SegName, (const CHAR *)linkeditName))
				linkeditSeg = seg;
		}
		else if (lc->Cmd == LC_SYMTAB)
		{
			symtab = (const SymtabCommand *)lc;
		}

		cmdPtr += lc->CmdSize;
	}

	if (textSeg == nullptr || linkeditSeg == nullptr || symtab == nullptr)
		return nullptr;

	// Compute the ASLR slide
	USIZE slide = (USIZE)header - (USIZE)textSeg->VmAddr;

	// Compute memory addresses of the symbol and string tables
	// File offsets within __LINKEDIT map to:
	//   slide + linkedit.VmAddr + (fileOffset - linkedit.FileOff)
	USIZE linkeditBase = slide + (USIZE)linkeditSeg->VmAddr - (USIZE)linkeditSeg->FileOff;
	const Nlist64 *symbols = (const Nlist64 *)(linkeditBase + symtab->SymOff);
	const CHAR *strings = (const CHAR *)(linkeditBase + symtab->StrOff);

	USIZE nameLen = StringUtils::Length(symbolName);

	for (UINT32 i = 0; i < symtab->NSyms; i++)
	{
		if (symbols[i].StrIndex >= symtab->StrSize)
			continue;

		const CHAR *name = strings + symbols[i].StrIndex;

		// Compare symbol name
		BOOL match = true;
		for (USIZE j = 0; j <= nameLen; j++)
		{
			if (name[j] != symbolName[j])
			{
				match = false;
				break;
			}
		}

		if (match && symbols[i].Value != 0)
			return (PVOID)(symbols[i].Value + slide);
	}

	return nullptr;
}

// =============================================================================
// Internal: Resolve dlopen and dlsym from dyld
// =============================================================================

/// @brief Cached dlopen/dlsym function pointers (resolved once, reused)
struct DyldFunctions
{
	DlOpenFn DlOpen;
	DlSymFn DlSym;
	BOOL Resolved;
	BOOL Failed;
};

/// @brief Resolve dlopen and dlsym from dyld's Mach-O symbol table
/// @param[out] fns Output structure for the resolved function pointers
/// @return true on success, false on failure
static BOOL ResolveDyldFunctions(DyldFunctions &fns)
{
	if (fns.Resolved)
		return !fns.Failed;

	fns.Resolved = true;
	fns.Failed = true;

	const DyldAllImageInfos *allInfo = GetDyldAllImageInfos();
	if (allInfo == nullptr || allInfo->Version < 2)
		return false;

	const MachHeader64 *dyldHeader = allInfo->DyldImageLoadAddress;
	if (dyldHeader == nullptr)
		return false;

	auto dlOpenName = "_dlopen";
	auto dlSymName = "_dlsym";

	fns.DlOpen = (DlOpenFn)FindSymbolInMachO(dyldHeader, (const CHAR *)dlOpenName);
	fns.DlSym = (DlSymFn)FindSymbolInMachO(dyldHeader, (const CHAR *)dlSymName);

	if (fns.DlOpen == nullptr || fns.DlSym == nullptr)
		return false;

	fns.Failed = false;
	return true;
}

// =============================================================================
// Public API: ResolveFrameworkFunction
// =============================================================================

PVOID ResolveFrameworkFunction(const CHAR *frameworkPath, const CHAR *functionName)
{
	// Stack-local cache for dyld functions (no global/static variables)
	// Re-resolved each call — acceptable overhead for infrequent screen ops.
	DyldFunctions fns;
	Memory::Zero(&fns, sizeof(fns));

	if (!ResolveDyldFunctions(fns))
		return nullptr;

	// Load the framework
	PVOID handle = fns.DlOpen(frameworkPath, RTLD_LAZY);
	if (handle == nullptr)
		return nullptr;

	// Resolve the function
	return fns.DlSym(handle, functionName);
}

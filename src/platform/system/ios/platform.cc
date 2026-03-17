#include "platform/platform.h"
#include "platform/kernel/ios/syscall.h"
#include "platform/kernel/ios/system.h"

// ARM64 iOS cannot use -static (kernel requires dyld). The linker adds
// dyld_stub_binder to the initial undefined symbols list for all dynamic
// executables. Normally libSystem provides it, but -nostdlib prevents linking
// libSystem. This no-op stub satisfies the linker. The explicit
// visibility("default") is required because the global -fvisibility=hidden
// would otherwise hide the symbol, preventing the linker from resolving the
// default-visibility initial-undefine reference. The stub is never called
// because -fvisibility=hidden eliminates all lazy-binding stubs.
extern "C" __attribute__((visibility("default"))) VOID dyld_stub_binder() {}

// iOS process exit implementation
NO_RETURN VOID ExitProcess(USIZE code)
{
	System::Call(SYS_EXIT, code);
	__builtin_unreachable();
}

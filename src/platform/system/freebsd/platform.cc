#include "platform/platform.h"
#include "platform/kernel/freebsd/syscall.h"
#include "platform/kernel/freebsd/system.h"

// FreeBSD process exit implementation
NO_RETURN VOID ExitProcess(USIZE code)
{
	System::Call(SYS_EXIT, code);
	__builtin_unreachable();
}

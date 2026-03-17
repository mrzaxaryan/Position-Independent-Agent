#include "platform/platform.h"
#include "platform/kernel/windows/ntdll.h"

NO_RETURN VOID ExitProcess(USIZE code)
{
	(void)NTDLL::ZwTerminateProcess(NTDLL::NtCurrentProcess(), (NTSTATUS)(code));
	__builtin_unreachable();
}

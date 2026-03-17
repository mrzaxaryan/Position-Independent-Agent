#include "platform/system/random.h"
#include "platform/system/date_time.h"
#include "platform/platform.h"

static FORCE_INLINE UINT64 GetHardwareTimestamp()
{
#if defined(ARCHITECTURE_X86_64) || defined(ARCHITECTURE_I386)
	// x86/x64: Read the Time Stamp Counter
	UINT32 lo, hi;
	__asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
	return ((UINT64)hi << 32) | lo;

#elif defined(ARCHITECTURE_AARCH64)
	// ARM64: Standard 64-bit system counter access
	UINT64 virtualTimerValue;
	__asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(virtualTimerValue));
	return virtualTimerValue;

#elif defined(ARCHITECTURE_RISCV64)
	// RISC-V 64-bit: Read the time CSR
	UINT64 val;
	__asm__ __volatile__("rdtime %0" : "=r"(val));
	return val;

#elif defined(ARCHITECTURE_MIPS64) || defined(ARCHITECTURE_ARMV7A) || defined(ARCHITECTURE_RISCV32)
	// 32-bit ARM/RISC-V: Use syscall-based monotonic timestamp
	// This uses clock_gettime(CLOCK_MONOTONIC) via PLATFORM
	return DateTime::GetMonotonicNanoseconds();

#else
#error "GetHardwareTimestamp not implemented for this architecture"
#endif
}

VOID Random::EnsureSeeded()
{
	if (!prng.IsSeeded())
		prng.Seed(GetHardwareTimestamp());
}

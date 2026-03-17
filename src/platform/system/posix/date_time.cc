#include "platform/system/date_time.h"
#if defined(PLATFORM_LINUX)
#include "platform/kernel/linux/syscall.h"
#include "platform/kernel/linux/system.h"
#elif defined(PLATFORM_ANDROID)
#include "platform/kernel/android/syscall.h"
#include "platform/kernel/android/system.h"
#elif defined(PLATFORM_MACOS)
#include "platform/kernel/macos/syscall.h"
#include "platform/kernel/macos/system.h"
#elif defined(PLATFORM_IOS)
#include "platform/kernel/ios/syscall.h"
#include "platform/kernel/ios/system.h"
#elif defined(PLATFORM_SOLARIS)
#include "platform/kernel/solaris/syscall.h"
#include "platform/kernel/solaris/system.h"
#elif defined(PLATFORM_FREEBSD)
#include "platform/kernel/freebsd/syscall.h"
#include "platform/kernel/freebsd/system.h"
#endif

DateTime DateTime::Now()
{
	DateTime dt;

#if defined(PLATFORM_MACOS) || defined(PLATFORM_IOS)
	Timeval tv;

	// Get current time using gettimeofday syscall
	// Note: macOS gettimeofday returns seconds in RAX (retval[0]) on success,
	// not 0. On error, System::Call returns negative (carry flag convention).
	SSIZE result = System::Call(SYS_GETTIMEOFDAY, (USIZE)&tv, (USIZE)0);
	if (result < 0)
	{
		// If syscall fails, return epoch time (1970-01-01 00:00:00)
		dt.Years = 1970;
		dt.Months = 1;
		dt.Days = 1;
		return dt;
	}

	UINT64 totalSeconds = (UINT64)tv.Sec;
	UINT64 nanoseconds = (UINT64)tv.Usec * 1000ULL;
#else
	Timespec ts;

	// Get current time using clock_gettime syscall
	SSIZE result = System::Call(SYS_CLOCK_GETTIME, CLOCK_REALTIME, (USIZE)&ts);
	if (result != 0)
	{
		// If syscall fails, return epoch time (1970-01-01 00:00:00)
		dt.Years = 1970;
		dt.Months = 1;
		dt.Days = 1;
		return dt;
	}

	UINT64 totalSeconds = (UINT64)ts.Sec;
	UINT64 nanoseconds = (UINT64)ts.Nsec;
#endif

	// Convert Unix timestamp (seconds since 1970-01-01) to date/time
	constexpr UINT64 SECONDS_PER_DAY = 86400;

	UINT64 days = totalSeconds / SECONDS_PER_DAY;
	UINT64 timeOfDay = totalSeconds % SECONDS_PER_DAY;

	DateTime::FromDaysAndTime(dt, days, 1970, timeOfDay, nanoseconds);
	return dt;
}

UINT64 DateTime::GetMonotonicNanoseconds()
{
#if defined(PLATFORM_MACOS) || defined(PLATFORM_IOS)
	// macOS has no clock_gettime BSD syscall — it's userspace-only via commpage.
	// Use gettimeofday instead (not truly monotonic, but functional).
	Timeval tv;
	SSIZE result = System::Call(SYS_GETTIMEOFDAY, (USIZE)&tv, (USIZE)0);
	if (result < 0)
		return 0;
	return ((UINT64)tv.Sec * 1000000000ULL) + ((UINT64)tv.Usec * 1000ULL);
#else
	Timespec ts;

	// Get monotonic time (not affected by system clock changes)
	SSIZE result = System::Call(SYS_CLOCK_GETTIME, CLOCK_MONOTONIC, (USIZE)&ts);
	if (result != 0)
		return 0;

	return ((UINT64)ts.Sec * 1000000000ULL) + (UINT64)ts.Nsec;
#endif
}

/**
 * @file environment.cc
 * @brief Shared POSIX environment variable and platform implementation
 *
 * @details Linux reads environment variables from /proc/self/environ.
 * macOS, FreeBSD, and Solaris return 0 (not found) as they lack a simple
 * procfs-based mechanism in freestanding mode.
 *
 * Platform identification:
 * - GetAgentPlatform(): compile-time OS target from PLATFORM_* defines
 * - GetOSVersion(): runtime OS version via uname syscall (Linux/Android),
 *   sysctl (macOS/iOS/FreeBSD), utssys then /etc/release (Solaris), or 0 on failure
 * - GetArchitecture(): runtime host CPU architecture via uname machine field
 *   (Linux/Android), sysctl hw.machine (macOS/FreeBSD — iOS reports hardware
 *   model identifiers, so it uses the compile-time fallback), or sysinfo
 *   SI_ARCHITECTURE_64/32 (Solaris), falling back to the compile-time
 *   ARCHITECTURE_* target
 * - GetHostname(): HOSTNAME env var (Linux/Android), /etc/hostname (Linux/Android),
 *   sysctl kern.hostname (macOS/iOS/FreeBSD), utssys then /etc/nodename (Solaris)
 *
 * Future enhancements:
 * - macOS: use sysctl(kern.procargs2) to read process environment
 * - FreeBSD: use sysctl(kern.proc.env) to read process environment
 * - Solaris: use /proc/self/psinfo + /proc/self/as or walk the initial stack
 */

#include "platform/system/environment.h"
#include "core/string/string.h"

// Platform-specific kernel headers
#if defined(PLATFORM_ANDROID)
#include "platform/kernel/android/syscall.h"
#include "platform/kernel/android/system.h"
#elif defined(PLATFORM_LINUX)
#include "platform/kernel/linux/syscall.h"
#include "platform/kernel/linux/system.h"
#elif defined(PLATFORM_MACOS)
#include "platform/kernel/macos/syscall.h"
#include "platform/kernel/macos/system.h"
#elif defined(PLATFORM_IOS)
#include "platform/kernel/ios/syscall.h"
#include "platform/kernel/ios/system.h"
#elif defined(PLATFORM_FREEBSD)
#include "platform/kernel/freebsd/syscall.h"
#include "platform/kernel/freebsd/system.h"
#elif defined(PLATFORM_SOLARIS)
#include "platform/kernel/solaris/syscall.h"
#include "platform/kernel/solaris/system.h"
#endif

#include "core/memory/memory.h"

#if defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)

// Helper to compare strings (case-sensitive for Linux)
static BOOL CompareEnvName(const CHAR *envEntry, const CHAR *name) noexcept
{
	while (*name != '\0')
	{
		if (*envEntry != *name)
		{
			return false;
		}
		envEntry++;
		name++;
	}

	// After name, should be '='
	return *envEntry == '=';
}

USIZE Environment::GetVariable(const CHAR *name, Span<CHAR> buffer) noexcept
{
	if (name == nullptr || buffer.Size() == 0)
	{
		return 0;
	}

	// Open /proc/self/environ
	const CHAR *procEnvPath = "/proc/self/environ";
#if defined(ARCHITECTURE_AARCH64) || defined(ARCHITECTURE_RISCV64) || defined(ARCHITECTURE_RISCV32)
	// aarch64/riscv only has openat syscall
	SSIZE fd = System::Call(SYS_OPENAT, (USIZE)-100, (USIZE)procEnvPath, 0, 0);
	if (fd < 0)
	{
		buffer[0] = '\0';
		return 0;
	}
#else
	SSIZE fd = System::Call(SYS_OPEN, (USIZE)procEnvPath, 0 /* O_RDONLY */, 0);
	if (fd < 0)
	{
		// Try openat with AT_FDCWD (-100) for newer kernels
		fd = System::Call(SYS_OPENAT, (USIZE)-100, (USIZE)procEnvPath, 0, 0);
		if (fd < 0)
		{
			buffer[0] = '\0';
			return 0;
		}
	}
#endif

	// Read environment block (entries separated by null bytes)
	CHAR envBuf[4096];
	SSIZE bytesRead = System::Call(SYS_READ, (USIZE)fd, (USIZE)envBuf, sizeof(envBuf) - 1);
	System::Call(SYS_CLOSE, (USIZE)fd);

	if (bytesRead <= 0)
	{
		buffer[0] = '\0';
		return 0;
	}

	envBuf[bytesRead] = '\0';

	// Search for the variable
	const CHAR *ptr = envBuf;
	const CHAR *end = envBuf + bytesRead;

	while (ptr < end && *ptr != '\0')
	{
		if (CompareEnvName(ptr, name))
		{
			// Find the '=' and skip past it
			const CHAR *value = ptr;
			while (*value != '=' && *value != '\0')
			{
				value++;
			}
			if (*value == '=')
			{
				value++; // Skip the '='

				// Copy value to buffer
				USIZE len = 0;
				while (*value != '\0' && len < buffer.Size() - 1)
				{
					buffer[len++] = *value++;
				}
				buffer[len] = '\0';
				return len;
			}
		}

		// Skip to next entry (after null terminator)
		while (ptr < end && *ptr != '\0')
		{
			ptr++;
		}
		ptr++; // Skip the null terminator
	}

	// Variable not found
	buffer[0] = '\0';
	return 0;
}

#else // macOS, FreeBSD, Solaris — stub implementation

USIZE Environment::GetVariable(const CHAR *name, Span<CHAR> buffer) noexcept
{
	if (name == nullptr || buffer.Size() == 0)
	{
		return 0;
	}

	// No /proc filesystem or equivalent available in freestanding mode.
	// Return empty result.
	buffer[0] = '\0';
	return 0;
}

#endif

// =============================================================================
// Platform identification (shared across all POSIX platforms)
// =============================================================================

USIZE Environment::GetAgentPlatform(Span<CHAR> buffer) noexcept
{
#if defined(PLATFORM_LINUX)
	StringUtils::Copy(buffer, Span<const CHAR>("linux"));
#elif defined(PLATFORM_MACOS)
	StringUtils::Copy(buffer, Span<const CHAR>("macos"));
#elif defined(PLATFORM_ANDROID)
	StringUtils::Copy(buffer, Span<const CHAR>("android"));
#elif defined(PLATFORM_IOS)
	StringUtils::Copy(buffer, Span<const CHAR>("ios"));
#elif defined(PLATFORM_FREEBSD)
	StringUtils::Copy(buffer, Span<const CHAR>("freebsd"));
#elif defined(PLATFORM_SOLARIS)
	StringUtils::Copy(buffer, Span<const CHAR>("solaris"));
#else
	buffer.Data()[0] = '\0';
	return 0;
#endif
	return StringUtils::Length(buffer.Data());
}

Result<USIZE, Error> Environment::GetOSVersion(Span<CHAR> buffer) noexcept
{
	if (buffer.Size() == 0)
		return Result<USIZE, Error>::Err(Error(Error::None));

#if defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)
	// Use the uname syscall to get kernel release info
	Utsname uts;
	Memory::Zero(&uts, sizeof(Utsname));
	SSIZE ret = System::Call(SYS_UNAME, (USIZE)&uts);
	if (ret == 0)
	{
		// Format: "{sysname} {release}" e.g. "Linux 6.1.0"
		USIZE sysLen = StringUtils::Length(uts.Sysname);
		USIZE relLen = StringUtils::Length(uts.Release);
		USIZE pos = 0;

		StringUtils::Copy(Span<CHAR>(buffer.Data() + pos, buffer.Size() - pos), Span<const CHAR>(uts.Sysname, sysLen + 1));
		pos += sysLen;

		buffer.Data()[pos++] = ' ';

		StringUtils::Copy(Span<CHAR>(buffer.Data() + pos, buffer.Size() - pos), Span<const CHAR>(uts.Release, relLen + 1));
		pos += relLen;
		return Result<USIZE, Error>::Ok(pos);
	}

	// Fallback: try reading /proc/version via raw syscalls
	{
		const CHAR *path = "/proc/version";
#if defined(ARCHITECTURE_AARCH64) || defined(ARCHITECTURE_RISCV64) || defined(ARCHITECTURE_RISCV32)
		SSIZE fd = System::Call(SYS_OPENAT, (USIZE)-100, (USIZE)path, 0, 0);
#else
		SSIZE fd = System::Call(SYS_OPEN, (USIZE)path, 0, 0);
#endif
		if (fd >= 0)
		{
			SSIZE bytesRead = System::Call(SYS_READ, (USIZE)fd, (USIZE)buffer.Data(), buffer.Size() - 1);
			System::Call(SYS_CLOSE, (USIZE)fd);
			if (bytesRead > 0)
			{
				// Trim trailing newline
				if (buffer.Data()[bytesRead - 1] == '\n')
					buffer.Data()[bytesRead - 1] = '\0';
				else
					buffer.Data()[bytesRead] = '\0';
				return Result<USIZE, Error>::Ok(StringUtils::Length(buffer.Data()));
			}
		}
	}
#elif defined(PLATFORM_MACOS) || defined(PLATFORM_IOS) || defined(PLATFORM_FREEBSD)
	// Use sysctl to query kern.ostype and kern.osrelease
	// sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	{
		INT32 mib[2];
		CHAR ostype[128];
		CHAR osrelease[128];
		USIZE len;

		// CTL_KERN=1, KERN_OSTYPE=1 → e.g. "Darwin" or "FreeBSD"
		mib[0] = 1;
		mib[1] = 1;
		len = sizeof(ostype) - 1;
		Memory::Zero(ostype, sizeof(ostype));
		SSIZE ret = System::Call(SYS_SYSCTL, (USIZE)mib, 2, (USIZE)ostype, (USIZE)&len, 0, 0);
		if (ret < 0)
			return Result<USIZE, Error>::Err(Error(Error::None));

		// CTL_KERN=1, KERN_OSRELEASE=2 → e.g. "23.1.0" or "14.0-RELEASE"
		mib[1] = 2;
		len = sizeof(osrelease) - 1;
		Memory::Zero(osrelease, sizeof(osrelease));
		ret = System::Call(SYS_SYSCTL, (USIZE)mib, 2, (USIZE)osrelease, (USIZE)&len, 0, 0);
		if (ret < 0)
			return Result<USIZE, Error>::Err(Error(Error::None));

		// Format: "{ostype} {osrelease}" e.g. "Darwin 23.1.0"
		USIZE sysLen = StringUtils::Length(ostype);
		USIZE relLen = StringUtils::Length(osrelease);
		USIZE pos = 0;

		StringUtils::Copy(Span<CHAR>(buffer.Data() + pos, buffer.Size() - pos), Span<const CHAR>(ostype, sysLen + 1));
		pos += sysLen;

		buffer.Data()[pos++] = ' ';

		StringUtils::Copy(Span<CHAR>(buffer.Data() + pos, buffer.Size() - pos), Span<const CHAR>(osrelease, relLen + 1));
		pos += relLen;
		return Result<USIZE, Error>::Ok(pos);
	}
#elif defined(PLATFORM_SOLARIS)
	// Try utssys syscall first (works on illumos/OpenIndiana)
	// utssys(buf, 0, UTS_UNAME=0)
	{
		SolarisUtsname uts;
		Memory::Zero(&uts, sizeof(SolarisUtsname));
		SSIZE ret = System::Call(SYS_UTSSYS, (USIZE)&uts, 0, 0);
		if (ret == 0 && uts.Sysname[0] != '\0')
		{
			// Format: "{sysname} {release}" e.g. "SunOS 5.11"
			USIZE sysLen = StringUtils::Length(uts.Sysname);
			USIZE relLen = StringUtils::Length(uts.Release);
			USIZE pos = 0;

			StringUtils::Copy(Span<CHAR>(buffer.Data() + pos, buffer.Size() - pos), Span<const CHAR>(uts.Sysname, sysLen + 1));
			pos += sysLen;

			buffer.Data()[pos++] = ' ';

			StringUtils::Copy(Span<CHAR>(buffer.Data() + pos, buffer.Size() - pos), Span<const CHAR>(uts.Release, relLen + 1));
			pos += relLen;
			return Result<USIZE, Error>::Ok(pos);
		}
	}

	// Fallback: read /etc/release (always present on Oracle Solaris 11.4
	// where utssys may be removed/repurposed)
	{
		const CHAR *path = "/etc/release";
		SSIZE fd = System::Call(SYS_OPEN, (USIZE)path, 0 /* O_RDONLY */, 0);
		if (fd < 0)
			fd = System::Call(SYS_OPENAT, (USIZE)AT_FDCWD, (USIZE)path, 0, 0);
		if (fd >= 0)
		{
			CHAR tmpBuf[256];
			SSIZE bytesRead = System::Call(SYS_READ, (USIZE)fd, (USIZE)tmpBuf, sizeof(tmpBuf) - 1);
			System::Call(SYS_CLOSE, (USIZE)fd);
			if (bytesRead > 0)
			{
				tmpBuf[bytesRead] = '\0';

				// Skip leading whitespace on first line
				const CHAR *p = tmpBuf;
				while (*p == ' ' || *p == '\t')
					p++;

				// Copy until newline or end
				USIZE pos = 0;
				while (*p != '\0' && *p != '\n' && pos < buffer.Size() - 1)
					buffer.Data()[pos++] = *p++;

				// Trim trailing whitespace
				while (pos > 0 && (buffer.Data()[pos - 1] == ' ' || buffer.Data()[pos - 1] == '\t'))
					pos--;

				buffer.Data()[pos] = '\0';
				if (pos > 0)
					return Result<USIZE, Error>::Ok(pos);
			}
		}
	}
#endif

	return Result<USIZE, Error>::Err(Error(Error::None));
}

// Resolve a uid to a username via /etc/passwd, falling back to the uid formatted
// as a decimal string when no passwd entry matches. passwd is authoritative on
// Linux/FreeBSD/Solaris (returns the real name); on macOS/iOS the login user
// lives in Directory Service rather than /etc/passwd, and Android has no passwd
// file, so those fall through to the numeric uid. Platform-independent: uses
// only file I/O syscalls + UIntToStr.
static Result<USIZE, Error> LookupUsernameByUid(UINT32 uid, Span<CHAR> buffer) noexcept
{
	if (buffer.Size() == 0)
		return Result<USIZE, Error>::Err(Error(Error::None));

	const CHAR *path = "/etc/passwd";
#if defined(ARCHITECTURE_AARCH64) || defined(ARCHITECTURE_RISCV64) || defined(ARCHITECTURE_RISCV32)
	SSIZE fd = System::Call(SYS_OPENAT, (USIZE)AT_FDCWD, (USIZE)path, 0 /*O_RDONLY*/, 0);
#else
	SSIZE fd = System::Call(SYS_OPEN, (USIZE)path, 0, 0);
	if (fd < 0)
		fd = System::Call(SYS_OPENAT, (USIZE)AT_FDCWD, (USIZE)path, 0, 0);
#endif

	if (fd >= 0)
	{
		// Read the whole file into a stack buffer.
		CHAR pwd[8192];
		USIZE total = 0;
		while (total < sizeof(pwd) - 1)
		{
			SSIZE n = System::Call(SYS_READ, (USIZE)fd, (USIZE)(pwd + total), sizeof(pwd) - 1 - total);
			if (n <= 0)
				break;
			total += (USIZE)n;
		}
		System::Call(SYS_CLOSE, (USIZE)fd);
		pwd[total] = '\0';

		// Walk lines of the form "name:passwd:uid:gid:...".
		CHAR *line = pwd;
		CHAR *end = pwd + total;
		while (line < end)
		{
			CHAR *eol = line;
			while (eol < end && *eol != '\n')
				eol++;

			CHAR *p = line;
			// Field 0: name
			CHAR *nameStart = p;
			while (p < eol && *p != ':')
				p++;
			CHAR *nameEnd = p;
			// Skip field 1 (passwd)
			if (p < eol && *p == ':')
				p++;
			while (p < eol && *p != ':')
				p++;
			// Field 2: uid
			if (p < eol && *p == ':')
				p++;
			CHAR *uidStart = p;
			while (p < eol && *p != ':')
				p++;
			CHAR *uidEnd = p;

			// Parse the uid field as decimal and compare.
			BOOL valid = (uidEnd > uidStart);
			UINT32 lineUid = 0;
			for (CHAR *d = uidStart; d < uidEnd && valid; d++)
			{
				if (*d < '0' || *d > '9')
					valid = false;
				else
					lineUid = lineUid * 10 + (UINT32)(*d - '0');
			}

			if (valid && lineUid == uid && nameEnd > nameStart)
			{
				USIZE nameLen = (USIZE)(nameEnd - nameStart);
				if (nameLen >= buffer.Size())
					nameLen = buffer.Size() - 1;
				Memory::Copy(buffer.Data(), nameStart, nameLen);
				buffer.Data()[nameLen] = '\0';
				return Result<USIZE, Error>::Ok(nameLen);
			}

			line = (eol < end) ? eol + 1 : eol;
		}
	}

	// No matching passwd entry (macOS Directory Service, Android, unmatched):
	// report the numeric uid (always non-empty).
	return Result<USIZE, Error>::Ok(StringUtils::UIntToStr(uid, buffer));
}

#if defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)
// Read the real uid from /proc/self/status (the "Uid:" line). Avoids needing a
// getuid syscall number per Linux architecture (including MIPS).
static Result<UINT32, Error> ReadUidFromProcStatus() noexcept
{
	const CHAR *path = "/proc/self/status";
#if defined(ARCHITECTURE_AARCH64) || defined(ARCHITECTURE_RISCV64) || defined(ARCHITECTURE_RISCV32)
	SSIZE fd = System::Call(SYS_OPENAT, (USIZE)-100, (USIZE)path, 0, 0);
#else
	SSIZE fd = System::Call(SYS_OPEN, (USIZE)path, 0, 0);
	if (fd < 0)
		fd = System::Call(SYS_OPENAT, (USIZE)-100, (USIZE)path, 0, 0);
#endif
	if (fd < 0)
		return Result<UINT32, Error>::Err(Error(Error::None));

	CHAR status[4096];
	SSIZE n = System::Call(SYS_READ, (USIZE)fd, (USIZE)status, sizeof(status) - 1);
	System::Call(SYS_CLOSE, (USIZE)fd);
	if (n <= 0)
		return Result<UINT32, Error>::Err(Error(Error::None));
	status[n] = '\0';

	// Find the line starting with "Uid:" and parse the first (real) uid.
	CHAR *p = status;
	CHAR *end = status + n;
	while (p < end)
	{
		if (p + 4 <= end && p[0] == 'U' && p[1] == 'i' && p[2] == 'd' && p[3] == ':')
		{
			CHAR *q = p + 4;
			while (q < end && (*q == '\t' || *q == ' '))
				q++;
			UINT32 uid = 0;
			BOOL haveDigit = false;
			while (q < end && *q >= '0' && *q <= '9')
			{
				haveDigit = true;
				uid = uid * 10 + (UINT32)(*q - '0');
				q++;
			}
			if (!haveDigit)
				return Result<UINT32, Error>::Err(Error(Error::None));
			return Result<UINT32, Error>::Ok(uid);
		}
		while (p < end && *p != '\n')
			p++;
		if (p < end)
			p++;
	}
	return Result<UINT32, Error>::Err(Error(Error::None));
}
#endif

Result<USIZE, Error> Environment::GetUsername(Span<CHAR> buffer) noexcept
{
	// 1. USER / LOGNAME from the environment (login sessions on Linux/Android).
	USIZE len = Environment::GetVariable("USER", buffer);
	if (len > 0)
		return Result<USIZE, Error>::Ok(len);
	len = Environment::GetVariable("LOGNAME", buffer);
	if (len > 0)
		return Result<USIZE, Error>::Ok(len);

	// 2. Resolve by uid: /etc/passwd lookup, falling back to the uid as a string.
#if defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)
	// Read the uid from /proc (no getuid syscall needed; works on every arch).
	auto uidResult = ReadUidFromProcStatus();
	if (!uidResult)
		return Result<USIZE, Error>::Err(Error(Error::None));
	UINT32 uid = uidResult.Value();
#else
	// macOS/iOS/FreeBSD/Solaris: getuid() (no /proc; env layer is stubbed here).
	UINT32 uid = (UINT32)System::Call(SYS_GETUID);
#endif
	return LookupUsernameByUid(uid, buffer);
}

Result<USIZE, Error> Environment::GetHostname(Span<CHAR> buffer) noexcept
{
	// Try HOSTNAME environment variable first (works on Linux/Android)
	USIZE len = Environment::GetVariable("HOSTNAME", buffer);
	if (len > 0)
		return Result<USIZE, Error>::Ok(len);

#if defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)
	// Fallback: read /etc/hostname
	{
		const CHAR *path = "/etc/hostname";
#if defined(ARCHITECTURE_AARCH64) || defined(ARCHITECTURE_RISCV64) || defined(ARCHITECTURE_RISCV32)
		SSIZE fd = System::Call(SYS_OPENAT, (USIZE)-100, (USIZE)path, 0, 0);
#else
		SSIZE fd = System::Call(SYS_OPEN, (USIZE)path, 0, 0);
#endif
		if (fd >= 0)
		{
			SSIZE bytesRead = System::Call(SYS_READ, (USIZE)fd, (USIZE)buffer.Data(), buffer.Size() - 1);
			System::Call(SYS_CLOSE, (USIZE)fd);
			if (bytesRead > 0)
			{
				// Trim trailing newline
				if (buffer.Data()[bytesRead - 1] == '\n')
					buffer.Data()[bytesRead - 1] = '\0';
				else
					buffer.Data()[bytesRead] = '\0';
				return Result<USIZE, Error>::Ok(StringUtils::Length(buffer.Data()));
			}
		}
	}
#elif defined(PLATFORM_MACOS) || defined(PLATFORM_IOS) || defined(PLATFORM_FREEBSD)
	// Use sysctl to query kern.hostname
	// sysctl({CTL_KERN=1, KERN_HOSTNAME=10}, ...)
	{
		INT32 mib[2];
		mib[0] = 1;   // CTL_KERN
		mib[1] = 10;  // KERN_HOSTNAME
		USIZE slen = buffer.Size() - 1;
		Memory::Zero(buffer.Data(), buffer.Size());
		SSIZE ret = System::Call(SYS_SYSCTL, (USIZE)mib, 2, (USIZE)buffer.Data(), (USIZE)&slen, 0, 0);
		if (ret == 0 && buffer.Data()[0] != '\0')
			return Result<USIZE, Error>::Ok(StringUtils::Length(buffer.Data()));
	}
#elif defined(PLATFORM_SOLARIS)
	// Try utssys nodename field (works on illumos)
	{
		SolarisUtsname uts;
		Memory::Zero(&uts, sizeof(SolarisUtsname));
		SSIZE ret = System::Call(SYS_UTSSYS, (USIZE)&uts, 0, 0);
		if (ret == 0 && uts.Nodename[0] != '\0')
		{
			USIZE nodeLen = StringUtils::Length(uts.Nodename);
			StringUtils::Copy(buffer, Span<const CHAR>(uts.Nodename, nodeLen + 1));
			return Result<USIZE, Error>::Ok(nodeLen);
		}
	}

	// Fallback: read /etc/nodename (present on Oracle Solaris 11.4)
	// then /etc/hostname (present on some illumos distributions)
	{
		const CHAR *paths[] = { "/etc/nodename", "/etc/hostname" };
		for (USIZE p = 0; p < 2; p++)
		{
			SSIZE fd = System::Call(SYS_OPEN, (USIZE)paths[p], 0 /* O_RDONLY */, 0);
			if (fd < 0)
				fd = System::Call(SYS_OPENAT, (USIZE)AT_FDCWD, (USIZE)paths[p], 0, 0);
			if (fd >= 0)
			{
				SSIZE bytesRead = System::Call(SYS_READ, (USIZE)fd, (USIZE)buffer.Data(), buffer.Size() - 1);
				System::Call(SYS_CLOSE, (USIZE)fd);
				if (bytesRead > 0)
				{
					// Trim trailing newline
					if (buffer.Data()[bytesRead - 1] == '\n')
						buffer.Data()[bytesRead - 1] = '\0';
					else
						buffer.Data()[bytesRead] = '\0';
					if (buffer.Data()[0] != '\0')
						return Result<USIZE, Error>::Ok(StringUtils::Length(buffer.Data()));
				}
			}
		}
	}
#endif

	return Result<USIZE, Error>::Err(Error(Error::None));
}

// Copy the kernel-provided machine string (uname Machine / sysctl hw.machine /
// sysinfo SI_ARCHITECTURE) verbatim — the OS-standard name ("x86_64",
// "armv7l", "amd64", "i686", ...) is reported unmodified. Clamps to the
// output buffer. Returns the copied length, or 0 if empty (caller falls back
// to the compile-time target). Unused on iOS, which skips runtime detection
// (hw.machine reports device models there).
[[maybe_unused]] static USIZE CopyKernelMachine(const CHAR *machine, Span<CHAR> buffer) noexcept
{
	if (buffer.Size() == 0)
		return 0;

	USIZE len = StringUtils::Length(machine);
	if (len == 0)
		return 0;

	if (len >= buffer.Size())
		len = buffer.Size() - 1;

	Memory::Copy(buffer.Data(), machine, len);
	buffer.Data()[len] = '\0';
	return len;
}

USIZE Environment::GetArchitecture(Span<CHAR> buffer) noexcept
{
#if defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)
	// uname reports the host machine architecture, not the build target
	// (e.g. "x86_64" when an i386 process runs on an x86_64 kernel).
	// Exception: a process with a 32-bit personality (setarch/linux32) or in
	// some containers sees the personality's machine name ("i686"), not the
	// physical CPU.
	Utsname uts;
	Memory::Zero(&uts, sizeof(Utsname));
	if (System::Call(SYS_UNAME, (USIZE)&uts) == 0)
	{
		USIZE len = CopyKernelMachine(uts.Machine, buffer);
		if (len > 0)
			return len;
	}
#elif defined(PLATFORM_MACOS) || defined(PLATFORM_FREEBSD)
	// sysctl hw.machine reports the host architecture ("x86_64", "arm64").
	// iOS deliberately does not use it: on iOS devices hw.machine returns a
	// hardware model identifier ("iPhone13,2"), not a CPU architecture, so
	// iOS falls through to the compile-time fallback below.
	// Note: a Rosetta 2 process on Apple Silicon reports "x86_64" here —
	// the emulated instruction set, not the physical CPU.
	{
		INT32 mib[2];
		mib[0] = 6; // CTL_HW
		mib[1] = 1; // HW_MACHINE
		CHAR machine[64];
		Memory::Zero(machine, sizeof(machine));
		USIZE len = sizeof(machine) - 1;
		if (System::Call(SYS_SYSCTL, (USIZE)mib, 2, (USIZE)machine, (USIZE)&len, 0, 0) == 0)
		{
			USIZE archLen = CopyKernelMachine(machine, buffer);
			if (archLen > 0)
				return archLen;
		}
	}
#elif defined(PLATFORM_SOLARIS)
	// sysinfo SI_ARCHITECTURE_64 reports the kernel instruction set ("amd64",
	// "aarch64"), so a 32-bit process on a 64-bit kernel reports the real host.
	// On 32-bit-only kernels SI_ARCHITECTURE_64 fails with EINVAL; fall back
	// to SI_ARCHITECTURE_32. System::Call normalizes Solaris carry-flag
	// errors to negative returns, so >= 0 means success.
	{
		CHAR machine[64];
		Memory::Zero(machine, sizeof(machine));
		SSIZE ret = System::Call(SYS_SYSINFO, SI_ARCHITECTURE_64, (USIZE)machine, sizeof(machine) - 1);
		if (ret < 0)
			ret = System::Call(SYS_SYSINFO, SI_ARCHITECTURE_32, (USIZE)machine, sizeof(machine) - 1);
		if (ret >= 0 && machine[0] != '\0')
		{
			machine[sizeof(machine) - 1] = '\0';
			USIZE archLen = CopyKernelMachine(machine, buffer);
			if (archLen > 0)
				return archLen;
		}
	}
#endif

	// Fallback: the architecture the agent was compiled for.
#if defined(ARCHITECTURE_X86_64)
	StringUtils::Copy(buffer, Span<const CHAR>("x86_64"));
#elif defined(ARCHITECTURE_I386)
	StringUtils::Copy(buffer, Span<const CHAR>("i386"));
#elif defined(ARCHITECTURE_AARCH64)
	StringUtils::Copy(buffer, Span<const CHAR>("aarch64"));
#elif defined(ARCHITECTURE_ARMV7A)
	StringUtils::Copy(buffer, Span<const CHAR>("armv7a"));
#elif defined(ARCHITECTURE_RISCV64)
	StringUtils::Copy(buffer, Span<const CHAR>("riscv64"));
#elif defined(ARCHITECTURE_RISCV32)
	StringUtils::Copy(buffer, Span<const CHAR>("riscv32"));
#elif defined(ARCHITECTURE_MIPS64)
	StringUtils::Copy(buffer, Span<const CHAR>("mips64"));
#elif defined(ARCHITECTURE_MIPS)
	StringUtils::Copy(buffer, Span<const CHAR>("mips"));
#else
	buffer.Data()[0] = '\0';
	return 0;
#endif
	return StringUtils::Length(buffer.Data());
}

USIZE Environment::GetProcessArchitecture(Span<CHAR> buffer) noexcept
{
	// The PROCESS arch is the arch this binary was compiled for — a 32-bit agent under
	// WOW64 on an x64 machine is an i386 PROCESS (GetArchitecture reports the host CPU).
	// Reported with the C2's tag vocabulary so it joins the identity-tag filter directly;
	// the C2 compiles delivered injectors for exactly this arch (they deserialize into
	// this process).
#if defined(ARCHITECTURE_X86_64)
	StringUtils::Copy(buffer, Span<const CHAR>("x86_64"));
#elif defined(ARCHITECTURE_AARCH64)
	StringUtils::Copy(buffer, Span<const CHAR>("aarch64"));
#elif defined(ARCHITECTURE_I386)
	StringUtils::Copy(buffer, Span<const CHAR>("i386"));
#elif defined(ARCHITECTURE_ARMV7A)
	StringUtils::Copy(buffer, Span<const CHAR>("armv7a"));
#else
	buffer.Data()[0] = '\0';
	return 0;
#endif
	return StringUtils::Length(buffer.Data());
}

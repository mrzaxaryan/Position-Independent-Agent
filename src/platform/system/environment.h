/**
 * @file environment.h
 * @brief Environment variable and platform information access
 *
 * @details Provides position-independent access to environment variables and
 * platform identification across all supported targets. On Windows, variables
 * are read from the PEB environment block. On Linux/Android, variables are read
 * from /proc/self/environ. On macOS/iOS/FreeBSD/Solaris the environment layer
 * is unavailable (GetVariable() returns 0). On UEFI, GetVariable() always
 * returns 0 as environment variables are not available. No .rdata dependencies.
 *
 * Platform and system identification:
 * - GetAgentPlatform(): compile-time OS target string (e.g. "windows", "linux")
 * - GetOSVersion(): runtime OS version string (e.g. "Windows 10.0 Build 19045")
 * - GetHostname(): machine hostname from OS environment
 * - GetUsername(): current user name (env, getuid()+/etc/passwd, or numeric uid)
 * - GetArchitecture(): runtime host CPU architecture string (e.g. "x86_64"),
 *   falling back to the compile-time target when OS detection is unavailable
 */

#pragma once

#include "core/core.h"
#include "core/types/result.h"

/**
 * @class Environment
 * @brief Static class for environment variable and platform information access.
 */
class Environment
{
public:
	/**
	 * @brief Retrieves the value of an environment variable.
	 *
	 * @param name Variable name (null-terminated).
	 * @param buffer Output buffer to receive the value.
	 * @return Length of the value written, or 0 if not found.
	 *
	 * @note On UEFI, this always returns 0 (no environment variables).
	 */
	static USIZE GetVariable(const CHAR* name, Span<CHAR> buffer) noexcept;

	/**
	 * @brief Retrieves the compile-time OS target name.
	 *
	 * @param buffer Output buffer to receive the platform string.
	 * @return Length of the string written (excluding null terminator).
	 *
	 * @details Returns a short identifier for the OS the agent was compiled
	 * for (e.g. "windows", "linux", "macos", "android", "ios", "freebsd",
	 * "solaris", "uefi"). Determined at compile time from PLATFORM_* defines.
	 */
	static USIZE GetAgentPlatform(Span<CHAR> buffer) noexcept;

	/**
	 * @brief Retrieves the runtime OS version string.
	 *
	 * @param buffer Output buffer to receive the version string.
	 * @return Length of the string written (excluding null terminator).
	 *
	 * @details Returns a human-readable OS version string detected at runtime:
	 * - Windows: "Windows {Major}.{Minor} Build {Build}" from PEB fields
	 * - Linux/Android: "{sysname} {release}" from the uname syscall
	 * - macOS/iOS/FreeBSD: "{ostype} {osrelease}" from sysctl
	 * - Solaris: "{sysname} {release}" from utssys syscall
	 * - UEFI: "uefi"
	 *
	 * @return Ok(length) on success, Err on failure
	 */
	[[nodiscard]] static Result<USIZE, Error> GetOSVersion(Span<CHAR> buffer) noexcept;

	/**
	 * @brief Retrieves the machine hostname.
	 *
	 * @param buffer Output buffer to receive the hostname string.
	 * @return Length of the string written (excluding null terminator).
	 *
	 * @details Retrieves the hostname using platform-specific methods:
	 * - Windows: COMPUTERNAME environment variable from PEB
	 * - Linux/Android: HOSTNAME environment variable, fallback to /etc/hostname
	 * - macOS/FreeBSD/Solaris/iOS: HOSTNAME environment variable, fallback
	 *   to /etc/hostname
	 * - UEFI: returns 0 (no hostname concept)
	 *
	 * @return Ok(length) on success, Err on failure
	 */
	[[nodiscard]] static Result<USIZE, Error> GetHostname(Span<CHAR> buffer) noexcept;

	/**
	 * @brief Retrieves the current user name.
	 *
	 * @param buffer Output buffer to receive the username string.
	 * @return Ok(length) on success, Err on failure.
	 *
	 * @details Retrieves the username using platform-specific methods:
	 * - Windows: USERNAME environment variable from PEB
	 * - Linux/Android: USER/LOGNAME environment variable
	 * - macOS/iOS/FreeBSD/Solaris: getuid() syscall + /etc/passwd lookup
	 *   (the environment layer is unavailable here)
	 * - UEFI: returns Err (no user concept)
	 */
	[[nodiscard]] static Result<USIZE, Error> GetUsername(Span<CHAR> buffer) noexcept;

	/**
	 * @brief Retrieves the runtime host CPU architecture string.
	 *
	 * @param buffer Output buffer to receive the architecture string.
	 * @return Length of the string written (excluding null terminator).
	 *
	 * @details Returns the OS-standard machine name for the host CPU,
	 * detected at runtime so an emulated process (e.g. an x86 agent under
	 * WOW64 on x64/ARM64, or an i386 agent on an x86_64 kernel) reports the
	 * real host architecture. The OS-provided name is reported verbatim:
	 * - Windows: IsWow64Process2 native machine (Windows 10 1511+) →
	 *   "amd64", "arm64", "i386", "arm"; pre-1511 x86-under-WOW64 falls
	 *   back to IsWow64Process → "amd64"
	 * - Linux/Android: uname machine field → "x86_64", "aarch64", "armv7l", ...
	 *   (a 32-bit personality via setarch/linux32 reports the personality
	 *   name, e.g. "i686", not the physical CPU)
	 * - macOS/FreeBSD: sysctl hw.machine → "arm64", "x86_64", ...
	 *   (a Rosetta 2 process reports the emulated "x86_64")
	 * - Solaris: sysinfo SI_ARCHITECTURE_64/32 → "amd64", "i386", ...
	 * - iOS / UEFI / detection failure: the compile-time ARCHITECTURE_* target
	 *   (e.g. "x86_64", "aarch64", "i386", "armv7a", "riscv64",
	 *   "riscv32", "mips64")
	 */
	static USIZE GetArchitecture(Span<CHAR> buffer) noexcept;

	/**
	 * @brief Retrieves the PROCESS architecture string.
	 *
	 * @param buffer Output buffer to receive the architecture string.
	 * @return Length of the string written (excluding null terminator).
	 *
	 * @details The arch of THIS binary — the process the agent runs in, not the
	 * host CPU (GetArchitecture): a 32-bit agent under WOW64 on an x64 machine is
	 * an i386 process. Reported with the C2's tag vocabulary (i386 / x86_64 /
	 * aarch64 / armv7a) so it joins the identity-tag filter directly; the C2
	 * compiles delivered injectors for exactly this arch (they deserialize into
	 * this process).
	 */
	static USIZE GetProcessArchitecture(Span<CHAR> buffer) noexcept;


	// Prevent instantiation
	VOID *operator new(USIZE) = delete;
	VOID operator delete(VOID *) = delete;
	VOID *operator new(USIZE, PVOID ptr) noexcept { return ptr; }
	VOID operator delete(VOID *, PVOID) noexcept {}
};

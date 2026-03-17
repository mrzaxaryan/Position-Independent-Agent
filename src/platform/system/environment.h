/**
 * @file environment.h
 * @brief Environment variable access
 *
 * @details Provides position-independent access to environment variables across
 * platforms. On Windows, variables are read from the PEB environment block. On
 * Linux, macOS, and Solaris, variables are retrieved from the process environ
 * pointer. On UEFI, GetVariable() always returns 0 as environment variables are
 * not available. No .rdata dependencies.
 */

#pragma once

#include "core/core.h"

/**
 * @class Environment
 * @brief Static class for environment variable access.
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

	// Prevent instantiation
	VOID *operator new(USIZE) = delete;
	VOID operator delete(VOID *) = delete;
	VOID *operator new(USIZE, PVOID ptr) noexcept { return ptr; }
	VOID operator delete(VOID *, PVOID) noexcept {}
};

/**
 * @file memory.h
 * @brief Platform-Independent Memory Operations
 *
 * @details Provides CRT-free memory manipulation functions (copy, set, compare, zero).
 * These are pure byte-by-byte operations with no platform dependencies, implemented
 * without relying on the C runtime library.
 *
 * The Memory class provides a clean C++ interface to low-level memory operations
 * that are essential for shellcode and position-independent code.
 *
 * These functions conform to the semantics defined by the ISO C standard:
 * - memset: ISO/IEC 9899:2018 (C17) Section 7.24.6.1
 * - memcpy: ISO/IEC 9899:2018 (C17) Section 7.24.2.1
 * - memmove: ISO/IEC 9899:2018 (C17) Section 7.24.2.2
 * - memcmp: ISO/IEC 9899:2018 (C17) Section 7.24.4.1
 * - bzero: IEEE Std 1003.1 (POSIX.1) — legacy, equivalent to memset(s, 0, n)
 *
 * @see ISO/IEC 9899:2018 — Programming languages — C (C17 standard)
 *      https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2310.pdf
 *
 * @note All functions are implemented in memory.cc without CRT dependencies.
 *
 * @ingroup core
 *
 * @defgroup memory Memory Operations
 * @ingroup core
 * @{
 */

#pragma once

#include "core/types/primitives.h"

// =============================================================================
// C LIBRARY MEMORY FUNCTIONS
// =============================================================================

/**
 * @brief Sets a block of memory to a specified value
 * @param dest Pointer to the destination memory block
 * @param ch Value to set (converted to unsigned char)
 * @param count Number of bytes to set
 * @return Pointer to the destination memory block
 *
 * @details Custom CRT-free implementation of the C standard library memset.
 * Conforms to the semantics defined in ISO/IEC 9899:2018 Section 7.24.6.1.
 *
 * @see ISO/IEC 9899:2018 Section 7.24.6.1 — The memset function
 *      https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2310.pdf
 */
extern "C" PVOID memset(PVOID dest, INT32 ch, USIZE count);

/**
 * @brief Copies a block of memory from source to destination
 * @param dest Pointer to the destination memory block
 * @param src Pointer to the source memory block
 * @param count Number of bytes to copy
 * @return Pointer to the destination memory block
 *
 * @details Custom CRT-free implementation of the C standard library memcpy.
 * Conforms to the semantics defined in ISO/IEC 9899:2018 Section 7.24.2.1.
 *
 * @warning Memory regions must not overlap; use memmove for overlapping regions.
 *
 * @see ISO/IEC 9899:2018 Section 7.24.2.1 — The memcpy function
 *      https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2310.pdf
 */
extern "C" PVOID memcpy(PVOID dest, PCVOID src, USIZE count);

/**
 * @brief Compares two blocks of memory byte by byte
 * @param ptr1 Pointer to the first memory block
 * @param ptr2 Pointer to the second memory block
 * @param num Number of bytes to compare
 * @return 0 if equal, negative if ptr1 < ptr2, positive if ptr1 > ptr2
 *
 * @details Custom CRT-free implementation of the C standard library memcmp.
 * Conforms to the semantics defined in ISO/IEC 9899:2018 Section 7.24.4.1.
 *
 * @see ISO/IEC 9899:2018 Section 7.24.4.1 — The memcmp function
 *      https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2310.pdf
 */
extern "C" INT32 memcmp(PCVOID ptr1, PCVOID ptr2, USIZE num);

/**
 * @brief Copies a block of memory, handling overlapping regions correctly
 * @param dest Pointer to the destination memory block
 * @param src Pointer to the source memory block
 * @param count Number of bytes to copy
 * @return Pointer to the destination memory block
 *
 * @details Custom CRT-free implementation of the C standard library memmove.
 * Unlike memcpy, this function correctly handles the case where the source
 * and destination memory regions overlap by choosing the appropriate copy
 * direction. Conforms to the semantics defined in ISO/IEC 9899:2018
 * Section 7.24.2.2.
 *
 * @see ISO/IEC 9899:2018 Section 7.24.2.2 — The memmove function
 *      https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2310.pdf
 */
extern "C" PVOID memmove(PVOID dest, PCVOID src, USIZE count);

/**
 * @brief Zeros a block of memory
 * @param dest Pointer to the destination memory block
 * @param count Number of bytes to zero
 *
 * @details Custom CRT-free implementation of the POSIX bzero function.
 * The LLVM aarch64 backend may emit bzero calls for zero-initialization
 * during LTO optimization, so a definition must be provided to avoid
 * unresolved external references that would generate data sections.
 *
 * @see IEEE Std 1003.1 (POSIX.1) — bzero (legacy)
 */
extern "C" VOID bzero(PVOID dest, USIZE count);

/**
 * @brief Apple/macOS-specific zero-fill function
 * @param dest Pointer to the destination memory block
 * @param count Number of bytes to zero
 *
 * @details The LLVM x86_64 backend on macOS may emit __bzero calls
 * (linker symbol ___bzero) for zero-initialization during LTO
 * optimization, distinct from the POSIX bzero symbol. A definition
 * must be provided to avoid unresolved external references when
 * linking with -nostdlib.
 */
extern "C" VOID __bzero(PVOID dest, USIZE count);

// =============================================================================
// MEMORY CLASS
// =============================================================================

/**
 * @class Memory
 * @brief Position-independent memory operations wrapper class
 *
 * @details Provides a clean C++ interface for memory operations without
 * CRT dependencies. All methods are force-inlined for maximum performance.
 *
 * @par Example Usage:
 * @code
 * CHAR buffer[256];
 * Memory::Zero(buffer, sizeof(buffer));           // Zero the buffer
 * Memory::Copy(buffer, "Hello", 5);               // Copy data
 * Memory::Set(buffer + 5, 'X', 10);               // Fill with 'X'
 * INT32 cmp = Memory::Compare(a, b, sizeof(a));   // Compare memory
 * @endcode
 */
class Memory
{
public:
	/**
	 * @brief Copies memory from source to destination
	 * @param dest Destination buffer
	 * @param src Source buffer
	 * @param count Number of bytes to copy
	 * @return Pointer to destination buffer
	 */
	FORCE_INLINE static PVOID Copy(PVOID dest, PCVOID src, USIZE count)
	{
		return memcpy(dest, src, count);
	}

	/**
	 * @brief Zeros a memory region
	 * @param dest Destination buffer to zero
	 * @param count Number of bytes to zero
	 * @return Pointer to destination buffer
	 */
	FORCE_INLINE static PVOID Zero(PVOID dest, USIZE count)
	{
		return memset(dest, 0, count);
	}

	/**
	 * @brief Sets memory to a specified byte value
	 * @param dest Destination buffer
	 * @param ch Byte value to set
	 * @param count Number of bytes to set
	 * @return Pointer to destination buffer
	 */
	FORCE_INLINE static PVOID Set(PVOID dest, INT32 ch, USIZE count)
	{
		return memset(dest, ch, count);
	}

	/**
	 * @brief Copies memory, handling overlapping regions correctly
	 * @param dest Destination buffer
	 * @param src Source buffer
	 * @param count Number of bytes to copy
	 * @return Pointer to destination buffer
	 */
	FORCE_INLINE static PVOID Move(PVOID dest, PCVOID src, USIZE count)
	{
		return memmove(dest, src, count);
	}

	/**
	 * @brief Compares two memory regions
	 * @param ptr1 First memory region
	 * @param ptr2 Second memory region
	 * @param num Number of bytes to compare
	 * @return 0 if equal, negative if ptr1 < ptr2, positive if ptr1 > ptr2
	 */
	FORCE_INLINE static INT32 Compare(PCVOID ptr1, PCVOID ptr2, USIZE num)
	{
		return memcmp(ptr1, ptr2, num);
	}
};

/** @} */ // end of memory group

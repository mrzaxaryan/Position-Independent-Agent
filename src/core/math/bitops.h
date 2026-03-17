/**
 * @file bitops.h
 * @brief Bit Manipulation Operations
 *
 * @details Provides bit rotation operations essential for cryptographic algorithms.
 * These templates offer consistent, efficient implementations that compile to
 * single CPU instructions on most architectures (ROL/ROR on x86, ROR on ARM).
 *
 * Used by:
 * - SHA-2 family (SHA-256, SHA-384, SHA-512)
 * - ChaCha20 stream cipher
 * - Other cryptographic primitives
 *
 * @ingroup core
 *
 * @defgroup bitops Bit Operations
 * @ingroup core
 * @{
 */

#pragma once

#include "core/types/primitives.h"
#include "core/compiler/compiler.h"

/**
 * @class BitOps
 * @brief Static class containing bit manipulation operations
 *
 * @details Provides templated bit rotation functions that work with any
 * unsigned integer type. The template implementations compile to optimal
 * machine instructions for the target architecture.
 */
class BitOps
{
public:
	/**
	 * @brief Rotate right (circular shift right)
	 * @tparam T Unsigned integer type (UINT32, UINT64, etc.)
	 * @param x Value to rotate
	 * @param n Number of bits to rotate by
	 * @return Value rotated right by n bits
	 *
	 * @details Performs a circular right shift where bits shifted out on the right
	 * are shifted back in on the left. Compiles to a single ROR instruction on
	 * x86/x64 and ARM architectures.
	 *
	 * @par Example:
	 * @code
	 * UINT32 val = 0x12345678;
	 * UINT32 rotated = BitOps::Rotr(val, 8);  // 0x78123456
	 * @endcode
	 */
	template<typename T>
	static constexpr FORCE_INLINE T Rotr(T x, UINT32 n)
	{
		constexpr UINT32 bits = sizeof(T) * 8;
		n &= (bits - 1);
		if (n == 0)
			return x;
		return (x >> n) | (x << (bits - n));
	}

	/**
	 * @brief Rotate left (circular shift left)
	 * @tparam T Unsigned integer type (UINT32, UINT64, etc.)
	 * @param x Value to rotate
	 * @param n Number of bits to rotate by
	 * @return Value rotated left by n bits
	 *
	 * @details Performs a circular left shift where bits shifted out on the left
	 * are shifted back in on the right. Compiles to a single ROL instruction on
	 * x86/x64 architectures.
	 *
	 * @par Example:
	 * @code
	 * UINT32 val = 0x12345678;
	 * UINT32 rotated = BitOps::Rotl(val, 8);  // 0x34567812
	 * @endcode
	 */
	template<typename T>
	static constexpr FORCE_INLINE T Rotl(T x, UINT32 n)
	{
		constexpr UINT32 bits = sizeof(T) * 8;
		n &= (bits - 1);
		if (n == 0)
			return x;
		return (x << n) | (x >> (bits - n));
	}

	/**
	 * @brief Rotate 32-bit value right
	 * @param x Value to rotate
	 * @param n Number of bits to rotate by
	 * @return Value rotated right by n bits
	 */
	static constexpr FORCE_INLINE UINT32 Rotr32(UINT32 x, UINT32 n) { return Rotr(x, n); }

	/**
	 * @brief Rotate 32-bit value left
	 * @param x Value to rotate
	 * @param n Number of bits to rotate by
	 * @return Value rotated left by n bits
	 */
	static constexpr FORCE_INLINE UINT32 Rotl32(UINT32 x, UINT32 n) { return Rotl(x, n); }

	/**
	 * @brief Rotate 64-bit value right
	 * @param x Value to rotate
	 * @param n Number of bits to rotate by
	 * @return Value rotated right by n bits
	 */
	static constexpr FORCE_INLINE UINT64 Rotr64(UINT64 x, UINT32 n) { return Rotr(x, n); }

	/**
	 * @brief Rotate 64-bit value left
	 * @param x Value to rotate
	 * @param n Number of bits to rotate by
	 * @return Value rotated left by n bits
	 */
	static constexpr FORCE_INLINE UINT64 Rotl64(UINT64 x, UINT32 n) { return Rotl(x, n); }
};

/** @} */ // end of bitops group

/**
 * @file djb2.h
 * @brief DJB2 Hash Algorithm Implementation
 *
 * @details Implements the DJB2 hash algorithm, originally created by
 * Daniel J. Bernstein and first published in comp.lang.c (1991). The algorithm
 * computes a non-cryptographic hash using the recurrence hash(i) = hash(i-1) * 33 + c,
 * where 33 is chosen for its favorable distribution properties. DJB2 is the hash
 * function used in Bernstein's CDB (constant database) package.
 *
 * This implementation extends the original algorithm with:
 * - **Build-unique seeding:** The initial hash value is derived from __DATE__ using
 *   FNV-1a, producing different hash outputs per compilation. This defeats static
 *   signature matching against known hash constants.
 * - **Case-insensitive hashing:** Characters are lowercased before hashing, enabling
 *   Windows API name resolution where export names are case-insensitive.
 * - **Dual evaluation modes:** Both runtime (`Hash`) and compile-time (`HashCompileTime`)
 *   variants are provided, allowing compile-time hash precomputation for constant
 *   expressions used in API resolution comparisons.
 * - **Wide string support:** Templated on character type to support both CHAR and WCHAR
 *   inputs with a single implementation.
 *
 * @see DJB2 Hash Function — Daniel J. Bernstein, comp.lang.c (1991)
 *      https://cr.yp.to/cdb.html
 * @see IETF Draft — The FNV Non-Cryptographic Hash Algorithm (seed generation)
 *      https://datatracker.ietf.org/doc/html/draft-eastlake-fnv
 *
 * @ingroup core
 *
 * @defgroup djb2 DJB2 Hash Algorithm
 * @ingroup core
 * @{
 */

#pragma once
#include "core/types/primitives.h"
#include "core/types/span.h"
#include "core/string/string.h"

/**
 * @brief Compile-time FNV-1a hash for DJB2 seed generation
 *
 * @details Computes a hash of the given string using the Fowler-Noll-Vo 1a (FNV-1a)
 * algorithm with 32-bit parameters extended to 64-bit arithmetic:
 * 1. Initialize hash to the FNV-1 32-bit offset basis (2166136261 / 0x811C9DC5)
 * 2. For each byte of input: XOR the byte into the hash, then multiply by the
 *    FNV-1 32-bit prime (16777619 / 0x01000193)
 * 3. Arithmetic is performed in 64-bit to allow natural overflow accumulation
 *
 * This function is invoked with __DATE__ to produce a build-unique seed for
 * the DJB2 hash, ensuring that hash values differ across compilations. The
 * consteval qualifier guarantees the computation is fully resolved at compile
 * time and never appears in the generated binary as a runtime call.
 *
 * @param s Null-terminated string to hash (typically __DATE__)
 * @return FNV-1a hash value computed entirely at compile time
 *
 * @see IETF Draft Section 6.2 — FNV-1a algorithm description
 *      https://datatracker.ietf.org/doc/html/draft-eastlake-fnv#section-6.2
 * @see IETF Draft Section 7.1 — FNV-1 32-bit parameters (offset basis and prime)
 *      https://datatracker.ietf.org/doc/html/draft-eastlake-fnv#section-7.1
 */
consteval UINT64 ct_hash_str_seed(const CHAR *s)
{
	UINT64 h = (UINT64)2166136261u; ///< FNV-1 32-bit offset basis (IETF Draft Section 7.1)
	for (UINT64 i = 0; s[i] != '\0'; ++i)
		h = (h ^ (UINT64)(UINT8)s[i]) * (UINT64)16777619u; ///< FNV-1 32-bit prime (IETF Draft Section 7.1)
	return h;
}

/**
 * @class Djb2
 * @brief DJB2 hash algorithm implementation for case-insensitive string hashing
 *
 * @details Provides both runtime and compile-time string hashing using the DJB2
 * algorithm by Daniel J. Bernstein. The core recurrence is:
 * @code
 * hash = hash * 33 + c
 * @endcode
 * which is computed as `(hash << 5) + hash + c` to avoid multiplication.
 * The multiplier 33 was chosen by Bernstein for its empirically favorable
 * distribution properties across typical string inputs.
 *
 * All hashing is case-insensitive: each character is converted to lowercase
 * via StringUtils::ToLowerCase() before being folded into the hash. This is
 * essential for Windows PE export name resolution, where DLL export names
 * may differ in casing across Windows versions.
 *
 * The initial hash value (seed) is not the traditional DJB2 constant 5381,
 * but is instead derived at compile time from __DATE__ via FNV-1a hashing.
 * This makes every build produce a unique set of hash values, preventing
 * signature-based detection of known API hash constants.
 *
 * @par Typical usage — API name resolution:
 * @code
 * // Precompute hash at compile time
 * constexpr UINT64 TARGET = Djb2::HashCompileTime("LoadLibraryA");
 *
 * // Walk PE export table, compute hash at runtime, compare
 * if (Djb2::Hash(exportName) == TARGET)
 * {
 *     // Resolved LoadLibraryA
 * }
 * @endcode
 *
 * @see DJB2 Hash Function — Daniel J. Bernstein, comp.lang.c (1991)
 *      https://cr.yp.to/cdb.html
 */
class Djb2
{
private:
	/**
	 * @brief Compile-time seed derived from __DATE__ via FNV-1a
	 *
	 * @details Replaces the traditional DJB2 initial value of 5381 with a
	 * build-unique value. Because the seed changes on every compilation,
	 * precomputed hash tables from one build cannot be reused to identify
	 * API names in a binary from a different build, defeating static
	 * signature-based detection of hash constants.
	 *
	 * @see ct_hash_str_seed — FNV-1a seed generation function
	 */
	static constexpr UINT64 Seed = ct_hash_str_seed(__DATE__);

public:
	/**
	 * @brief Computes a case-insensitive DJB2 hash at runtime
	 *
	 * @details Iterates over a null-terminated string, applying the DJB2 recurrence
	 * for each character:
	 * 1. Convert the character to lowercase via StringUtils::ToLowerCase()
	 * 2. Compute hash = (hash << 5) + hash + c, equivalent to hash * 33 + c
	 * 3. Repeat until the null terminator is reached
	 *
	 * The function is marked constexpr so the compiler may evaluate it at compile
	 * time when the input is a constant expression, but unlike HashCompileTime()
	 * it is not required to — it can also be called with runtime string pointers
	 * (e.g., PE export names discovered during module enumeration).
	 *
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param value Null-terminated string to hash
	 * @return 64-bit DJB2 hash of the lowercased input
	 *
	 * @see DJB2 Hash Function — Daniel J. Bernstein, comp.lang.c (1991)
	 *      https://cr.yp.to/cdb.html
	 */
	template <typename TChar>
	static constexpr FORCE_INLINE UINT64 Hash(const TChar *value)
	{
		UINT64 h = Seed;
		for (UINT64 i = 0; value[i] != (TChar)0; ++i)
		{
			TChar c = StringUtils::ToLowerCase(value[i]);
			h = ((h << 5) + h) + (UINT64)c;
		}
		return h;
	}

	/**
	 * @brief Computes a case-insensitive DJB2 hash over a bounded span
	 *
	 * @details Same algorithm as Hash(const TChar*), but operates on a
	 * size-bounded Span rather than relying on null termination. Suitable
	 * for hashing substrings or buffers that may not be null-terminated.
	 *
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param value Span of characters to hash
	 * @return 64-bit DJB2 hash of the lowercased input
	 *
	 * @see DJB2 Hash Function — Daniel J. Bernstein, comp.lang.c (1991)
	 *      https://cr.yp.to/cdb.html
	 */
	template <typename TChar>
	static constexpr FORCE_INLINE UINT64 Hash(Span<const TChar> value)
	{
		UINT64 h = Seed;
		for (USIZE i = 0; i < value.Size(); ++i)
		{
			TChar c = StringUtils::ToLowerCase(value[i]);
			h = ((h << 5) + h) + (UINT64)c;
		}
		return h;
	}

	/**
	 * @brief Computes a case-insensitive DJB2 hash at compile time
	 *
	 * @details Applies the same DJB2 recurrence as Hash(), but is declared
	 * consteval to guarantee the computation is fully resolved during
	 * compilation. The result is emitted as an immediate constant in the
	 * instruction stream, never appearing in a data section.
	 *
	 * The template parameter N is deduced from the string literal array
	 * size, restricting the input to compile-time-known string literals
	 * or constexpr arrays. Typical use is to precompute the expected hash
	 * of a target API name, which is then compared against runtime hashes
	 * computed by Hash() during PE export table enumeration:
	 *
	 * @code
	 * constexpr UINT64 ZW_CREATE_FILE = Djb2::HashCompileTime("ZwCreateFile");
	 * @endcode
	 *
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @tparam N Array size (deduced from the string literal)
	 * @param value String literal or constexpr array to hash
	 * @return 64-bit DJB2 hash computed entirely at compile time
	 *
	 * @see DJB2 Hash Function — Daniel J. Bernstein, comp.lang.c (1991)
	 *      https://cr.yp.to/cdb.html
	 */
	template <typename TChar, USIZE N>
	static consteval UINT64 HashCompileTime(const TChar (&value)[N])
	{
		UINT64 h = Seed;
		for (UINT64 i = 0; value[i] != (TChar)0; ++i)
		{
			TChar c = StringUtils::ToLowerCase(value[i]);
			h = ((h << 5) + h) + (UINT64)c;
		}
		return h;
	}
};

/** @} */ // end of djb2 group

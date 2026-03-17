/**
 * @file random.h
 * @brief Hardware-seeded pseudorandom number generator
 * @details Wraps the CORE layer's Prng (xorshift64) with automatic hardware-timestamp
 * seeding. Position-independent with no data section dependencies.
 * Part of the PLATFORM layer of the Position-Independent Runtime (PIR).
 */

#pragma once

#include "platform/platform.h"
#include "core/types/uuid.h"

/**
 * @class Random
 * @brief Hardware-seeded pseudorandom number generator
 *
 * @details Wraps the CORE layer's Prng (xorshift64) with automatic
 * hardware-timestamp seeding on first use. The seed is obtained from
 * architecture-specific counters (RDTSC on x86, CNTVCT on ARM64,
 * clock_gettime on ARMv7A).
 */
class Random
{
private:
	Prng prng;

	/// Lazy-seed from hardware timestamp (defined in random.cc)
	VOID EnsureSeeded();

public:
	VOID *operator new(USIZE) = delete;
	VOID operator delete(VOID *) = delete;
	VOID *operator new(USIZE, PVOID ptr) noexcept { return ptr; }
	VOID operator delete(VOID *, PVOID) noexcept {}

	/// The exclusive upper bound for values returned by Get()
	static constexpr INT32 Max = Prng::Max;

	/// Constructor — unseeded; Get() auto-seeds on first call
	Random() = default;

	/**
	 * @brief Generate the next pseudorandom number
	 * @return Pseudorandom INT32 in [0, MAX)
	 */
	INT32 Get()
	{
		EnsureSeeded();
		return prng.Get();
	}

	/**
	 * @brief Fill a buffer with pseudorandom bytes
	 * @param buffer Output buffer to fill
	 */
	VOID GetArray(Span<UINT8> buffer)
	{
		EnsureSeeded();
		prng.GetArray(buffer);
	}

	/**
	 * @brief Generate a random lowercase letter (a-z)
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @return Random character in ['a', 'z']
	 */
	template <typename TChar>
	TChar GetChar()
	{
		EnsureSeeded();
		return prng.GetChar<TChar>();
	}

	/**
	 * @brief Fill a span with random lowercase characters and null-terminate
	 * @tparam TChar Character type (CHAR or WCHAR)
	 * @param str Output span (must include space for null terminator)
	 * @return Number of random characters written (str.Size()-1), or 0 if empty
	 */
	template <typename TChar>
	UINT32 GetString(Span<TChar> str)
	{
		EnsureSeeded();
		return prng.GetString<TChar>(str);
	}

	/**
	 * @brief Generate a random (version 4) UUID
	 * @return Randomly generated UUID with version 4 and RFC 9562 variant bits set
	 *
	 * @see RFC 9562 Section 5.4 — UUID Version 4
	 *      https://datatracker.ietf.org/doc/html/rfc9562#section-5.4
	 */
	UUID RandomUUID()
	{
		UINT8 bytes[16];
		GetArray(Span<UINT8>(bytes));

		// Set version 4 (random): high nibble of byte 6 = 0100
		bytes[6] = (bytes[6] & 0x0F) | 0x40;
		// Set variant 10 (RFC 9562): high 2 bits of byte 8 = 10
		bytes[8] = (bytes[8] & 0x3F) | 0x80;

		return UUID(Span<const UINT8, 16>(bytes));
	}
};

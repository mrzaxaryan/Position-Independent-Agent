/**
 * @file bitset.h
 * @brief Heap-Backed Bitset
 *
 * @details Provides a compact, dynamically sized bit array over a heap
 * UINT8[] backing store. One bit per index gives an 8x memory reduction over
 * the heap BOOL[] grids it replaces (a 1080p tile grid drops from
 * tiles * 4 bytes to tiles / 8 bytes).
 *
 * Key properties:
 * - RAII: destructor frees the backing array
 * - Move-only: copy is deleted, move transfers ownership
 * - Fallible: Init() returns BOOL (false on allocation failure)
 * - Stack-only: heap allocation of the Bitset itself is deleted
 *
 * @ingroup core
 *
 * @defgroup bitset Bitset
 * @ingroup core
 * @{
 */

#pragma once

#include "core/memory/memory.h"
#include "core/types/primitives.h"

/**
 * @brief Dynamically sized bit array over a heap UINT8[] backing store
 *
 * @par Example Usage:
 * @code
 * Bitset visited;
 * if (!visited.Init(tileCount)) return;  // allocation failed
 * visited.Set(index);
 * if (visited.Test(index)) { ... }
 * // destructor frees automatically
 * @endcode
 */
struct Bitset
{
	UINT8 *Data;    ///< Backing byte array (bit i lives in Data[i / 8], bit i % 8)
	USIZE BitCount; ///< Logical number of bits

	// Stack-only: heap allocation of the Bitset itself is deleted
	VOID *operator new(USIZE) = delete;
	VOID *operator new[](USIZE) = delete;
	VOID operator delete(VOID *) = delete;
	VOID operator delete[](VOID *) = delete;
	// Placement new/delete required by Result<Bitset, Error>
	VOID *operator new(USIZE, PVOID ptr) noexcept { return ptr; }
	VOID operator delete(VOID *, PVOID) noexcept {}

	/**
	 * @brief Construct an empty bitset owning nothing
	 * @note All members are zeroed; call Init() before use
	 */
	constexpr Bitset() : Data(nullptr), BitCount(0) {}

	/**
	 * @brief Free the backing array if owned
	 */
	~Bitset()
	{
		if (Data)
			delete[] Data;
	}

	Bitset(const Bitset &) = delete;
	Bitset &operator=(const Bitset &) = delete;

	/**
	 * @brief Move-construct, stealing the source's backing array
	 * @param other Bitset to steal from (left empty)
	 */
	constexpr Bitset(Bitset &&other) : Data(other.Data), BitCount(other.BitCount)
	{
		other.Data = nullptr;
		other.BitCount = 0;
	}

	/**
	 * @brief Move-assign, freeing any array this bitset already owns
	 * @param other Bitset to steal from (left empty)
	 * @return Reference to this bitset
	 */
	Bitset &operator=(Bitset &&other)
	{
		if (this != &other)
		{
			if (Data)
				delete[] Data;
			Data = other.Data;
			BitCount = other.BitCount;
			other.Data = nullptr;
			other.BitCount = 0;
		}
		return *this;
	}

	/**
	 * @brief Allocate zeroed storage for a number of bits
	 * @param bitCount Number of bits the set must address
	 * @return true on success, false on allocation failure or re-init of an initialized set
	 * @note Allocates Ceil(bitCount / 8) bytes; all bits start cleared
	 */
	[[nodiscard]] BOOL Init(USIZE bitCount)
	{
		if (Data)
			return false;
		Data = new UINT8[(bitCount + 7) / 8];
		if (!Data)
			return false;
		Memory::Zero(Data, (bitCount + 7) / 8);
		BitCount = bitCount;
		return true;
	}

	/**
	 * @brief Set a bit to 1
	 * @param index Bit index (unchecked; must be < BitCount)
	 */
	VOID Set(USIZE index)
	{
		Data[index >> 3] |= (UINT8)(1u << (index & 7));
	}

	/**
	 * @brief Clear a bit to 0
	 * @param index Bit index (unchecked; must be < BitCount)
	 */
	VOID Clear(USIZE index)
	{
		Data[index >> 3] &= (UINT8)~(1u << (index & 7));
	}

	/**
	 * @brief Read a bit
	 * @param index Bit index (unchecked; must be < BitCount)
	 * @return true if the bit is set, false if clear
	 */
	BOOL Test(USIZE index) const
	{
		return (Data[index >> 3] & (1u << (index & 7))) != 0;
	}

	/**
	 * @brief Clear every bit without releasing the allocation
	 * @note Retaining the backing store makes reuse free — useful for per-frame grids
	 */
	VOID Reset()
	{
		if (Data && BitCount > 0)
			Memory::Zero(Data, (BitCount + 7) / 8);
	}
};

/** @} */ // end of bitset group

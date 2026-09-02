#pragma once

#include "lib/runtime.h"
#include "tests.h"

class BitsetTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running Bitset Tests...");

		RunTest(allPassed, &TestBasicsSuite, "Basics suite");
		RunTest(allPassed, &TestBoundaryBitsSuite, "Boundary bits suite");
		RunTest(allPassed, &TestMoveSemanticsSuite, "Move semantics suite");

		if (allPassed)
			LOG_INFO("All Bitset tests passed!");
		else
			LOG_ERROR("Some Bitset tests failed!");

		return allPassed;
	}

private:
	static BOOL TestBasicsSuite()
	{
		BOOL allPassed = true;

		// --- Default construction ---
		{
			Bitset b;
			BOOL passed = true;

			if (b.Data != nullptr)
			{
				LOG_ERROR("Default Data != nullptr");
				passed = false;
			}
			if (passed && b.BitCount != 0)
			{
				LOG_ERROR("Default BitCount != 0");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Default construction");
			else
			{
				LOG_ERROR("  FAILED: Default construction");
				allPassed = false;
			}
		}

		// --- Init zeroes all bits ---
		{
			Bitset b;
			BOOL passed = true;

			if (!b.Init(20))
			{
				LOG_ERROR("Init(20) returned false");
				passed = false;
			}
			if (passed && b.Data == nullptr)
			{
				LOG_ERROR("Data == nullptr after Init");
				passed = false;
			}
			if (passed && b.BitCount != 20)
			{
				LOG_ERROR("BitCount != 20 after Init");
				passed = false;
			}
			if (passed)
			{
				for (USIZE i = 0; i < 20; i++)
				{
					if (b.Test(i))
					{
						LOG_ERROR("Bit %u set after Init", (UINT32)i);
						passed = false;
						break;
					}
				}
			}
			// Re-init must fail so the existing allocation cannot be leaked
			if (passed && b.Init(4))
			{
				LOG_ERROR("Init() on initialized bitset returned true");
				passed = false;
			}
			// A bit count near the USIZE maximum must fail allocation, not wrap
			// the Ceil(bitCount / 8) computation down to zero and report Ok with
			// no backing bytes (any Set() would then write out of bounds)
			{
				Bitset huge;
				if (huge.Init(~(USIZE)0))
				{
					LOG_ERROR("Init(USIZE max) reported Ok — byte count wrapped");
					passed = false;
				}
				else if (huge.Data != nullptr || huge.BitCount != 0)
				{
					LOG_ERROR("Failed Init(USIZE max) left state dirty");
					passed = false;
				}
			}

			if (passed)
				LOG_INFO("  PASSED: Init allocates zeroed storage");
			else
			{
				LOG_ERROR("  FAILED: Init allocates zeroed storage");
				allPassed = false;
			}
		}

		// --- Set / Test / Clear ---
		{
			Bitset b;
			if (!b.Init(16)) return false;
			BOOL passed = true;

			b.Set(3);
			if (!b.Test(3))
			{
				LOG_ERROR("Test(3) false after Set(3)");
				passed = false;
			}
			// Neighbours must be untouched (no bleed within the byte)
			if (passed && (b.Test(2) || b.Test(4)))
			{
				LOG_ERROR("Neighbour bits affected by Set(3)");
				passed = false;
			}
			b.Clear(3);
			if (passed && b.Test(3))
			{
				LOG_ERROR("Test(3) true after Clear(3)");
				passed = false;
			}
			// Set, Clear, Set round-trip
			if (passed)
			{
				b.Set(3);
				if (!b.Test(3))
				{
					LOG_ERROR("Set after Clear failed");
					passed = false;
				}
			}

			if (passed)
				LOG_INFO("  PASSED: Set/Clear/Test round-trip");
			else
			{
				LOG_ERROR("  FAILED: Set/Clear/Test round-trip");
				allPassed = false;
			}
		}

		// --- Reset clears all bits ---
		{
			Bitset b;
			if (!b.Init(24)) return false;
			for (USIZE i = 0; i < 24; i += 3)
				b.Set(i);

			UINT8 *dataBefore = b.Data;
			b.Reset();

			BOOL passed = true;
			for (USIZE i = 0; i < 24; i++)
			{
				if (b.Test(i))
				{
					LOG_ERROR("Bit %u still set after Reset", (UINT32)i);
					passed = false;
					break;
				}
			}
			if (passed && (b.Data != dataBefore || b.BitCount != 24))
			{
				LOG_ERROR("Reset reallocated or changed BitCount");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Reset clears bits and keeps storage");
			else
			{
				LOG_ERROR("  FAILED: Reset clears bits and keeps storage");
				allPassed = false;
			}
		}

		return allPassed;
	}

	static BOOL TestBoundaryBitsSuite()
	{
		BOOL allPassed = true;

		// --- Byte-boundary indices ---
		{
			// 17 bits spans 3 bytes: indices 0, 7, 8, 15, 16 hit every boundary case
			Bitset b;
			if (!b.Init(17)) return false;
			BOOL passed = true;

			const USIZE boundaries[5] = {0, 7, 8, 15, 16};
			for (INT32 i = 0; i < 5; i++)
			{
				USIZE idx = boundaries[i];
				b.Set(idx);
				if (!b.Test(idx))
				{
					LOG_ERROR("Test(%u) false after Set", (UINT32)idx);
					passed = false;
				}
			}
			// All non-boundary bits must remain clear
			if (passed)
			{
				for (USIZE i = 0; i < 17; i++)
				{
					BOOL isBoundary = (i == 0 || i == 7 || i == 8 || i == 15 || i == 16);
					if (b.Test(i) != isBoundary)
					{
						LOG_ERROR("Bit %u state wrong after boundary sets", (UINT32)i);
						passed = false;
						break;
					}
				}
			}
			// Clear each boundary bit again
			if (passed)
			{
				for (INT32 i = 0; i < 5; i++)
				{
					b.Clear(boundaries[i]);
					if (b.Test(boundaries[i]))
					{
						LOG_ERROR("Test(%u) true after Clear", (UINT32)boundaries[i]);
						passed = false;
						break;
					}
				}
			}

			if (passed)
				LOG_INFO("  PASSED: Byte-boundary bit indices");
			else
			{
				LOG_ERROR("  FAILED: Byte-boundary bit indices");
				allPassed = false;
			}
		}

		// --- Non-multiple-of-8 bit count ---
		{
			// 13 bits needs 2 bytes; the final bit is index 12 (BitCount - 1)
			Bitset b;
			if (!b.Init(13)) return false;
			BOOL passed = true;

			b.Set(12);
			if (!b.Test(12))
			{
				LOG_ERROR("Test(BitCount-1) false after Set");
				passed = false;
			}
			// The padding bits in the last byte (13..15) must not disturb bit 12
			b.Set(12);
			if (passed && !b.Test(12))
			{
				LOG_ERROR("Repeated Set(BitCount-1) lost the bit");
				passed = false;
			}
			b.Clear(12);
			if (passed && b.Test(12))
			{
				LOG_ERROR("Test(BitCount-1) true after Clear");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Non-multiple-of-8 bit count");
			else
			{
				LOG_ERROR("  FAILED: Non-multiple-of-8 bit count");
				allPassed = false;
			}
		}

		// --- High and low bits in the same byte ---
		{
			Bitset b;
			if (!b.Init(8)) return false;
			BOOL passed = true;

			// Fully populated byte reads back as 0xFF
			for (USIZE i = 0; i < 8; i++)
				b.Set(i);
			if (passed && b.Data[0] != 0xFF)
			{
				LOG_ERROR("Data[0] = 0x%02X, expected 0xFF", b.Data[0]);
				passed = false;
			}
			// Clear every other bit -> 0xAA (bits 1,3,5,7)
			for (USIZE i = 0; i < 8; i += 2)
				b.Clear(i);
			if (passed && b.Data[0] != 0xAA)
			{
				LOG_ERROR("Data[0] = 0x%02X, expected 0xAA", b.Data[0]);
				passed = false;
			}
			if (passed)
			{
				for (USIZE i = 0; i < 8; i++)
				{
					if (b.Test(i) != (i % 2 == 1))
					{
						LOG_ERROR("Bit %u state wrong after alternating clear", (UINT32)i);
						passed = false;
						break;
					}
				}
			}

			if (passed)
				LOG_INFO("  PASSED: Bit pattern within a single byte");
			else
			{
				LOG_ERROR("  FAILED: Bit pattern within a single byte");
				allPassed = false;
			}
		}

		// --- Larger grid: set / sweep / clear all ---
		{
			// Mirrors the image-processor tile-grid use case (tilesX * tilesY)
			const USIZE tilesX = 30, tilesY = 20; // 600 bits = 75 bytes
			Bitset visited;
			if (!visited.Init(tilesX * tilesY)) return false;
			BOOL passed = true;

			for (USIZE ty = 0; ty < tilesY; ty++)
			{
				for (USIZE tx = 0; tx < tilesX; tx++)
				{
					if ((tx + ty) % 5 == 0)
						visited.Set(ty * tilesX + tx);
				}
			}
			USIZE setCount = 0;
			for (USIZE i = 0; i < tilesX * tilesY; i++)
			{
				if (visited.Test(i))
					setCount++;
			}
			// (tx + ty) % 5 == 0 hits 120 of the 600 cells
			if (setCount != 120)
			{
				LOG_ERROR("setCount = %u, expected 120", (UINT32)setCount);
				passed = false;
			}
			visited.Reset();
			if (passed && visited.Test(0))
			{
				LOG_ERROR("Bit 0 still set after Reset");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Tile-grid sized bitset");
			else
			{
				LOG_ERROR("  FAILED: Tile-grid sized bitset");
				allPassed = false;
			}
		}

		return allPassed;
	}

	static BOOL TestMoveSemanticsSuite()
	{
		BOOL allPassed = true;

		// --- Move construct ---
		{
			Bitset b;
			if (!b.Init(16)) return false;
			b.Set(4);
			b.Set(9);

			UINT8 *origData = b.Data;
			Bitset b2((Bitset &&)b);

			BOOL passed = true;
			if (b.Data != nullptr || b.BitCount != 0)
			{
				LOG_ERROR("Source not zeroed after move construct");
				passed = false;
			}
			if (passed && (b2.Data != origData || b2.BitCount != 16))
			{
				LOG_ERROR("Destination does not match original after move construct");
				passed = false;
			}
			if (passed && (!b2.Test(4) || !b2.Test(9) || b2.Test(0)))
			{
				LOG_ERROR("Bit states incorrect after move construct");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Move construction transfers ownership");
			else
			{
				LOG_ERROR("  FAILED: Move construction transfers ownership");
				allPassed = false;
			}
		}

		// --- Move assign ---
		{
			Bitset b;
			if (!b.Init(16)) return false;
			b.Set(2);

			UINT8 *origData = b.Data;

			Bitset b2;
			if (!b2.Init(8)) return false;
			b2 = (Bitset &&)b;

			BOOL passed = true;
			if (b.Data != nullptr || b.BitCount != 0)
			{
				LOG_ERROR("Source not zeroed after move assign");
				passed = false;
			}
			if (passed && (b2.Data != origData || b2.BitCount != 16))
			{
				LOG_ERROR("Destination incorrect after move assign");
				passed = false;
			}
			if (passed && !b2.Test(2))
			{
				LOG_ERROR("Bit state incorrect after move assign");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Move assignment frees destination and steals source");
			else
			{
				LOG_ERROR("  FAILED: Move assignment frees destination and steals source");
				allPassed = false;
			}
		}

		// --- Self-assign ---
		{
			Bitset b;
			if (!b.Init(8)) return false;
			b.Set(1);

			UINT8 *origData = b.Data;
			b = (Bitset &&)b;

			BOOL passed = b.Data == origData && b.BitCount == 8 && b.Test(1);

			if (passed)
				LOG_INFO("  PASSED: Move self-assignment is safe");
			else
			{
				LOG_ERROR("Self move-assign corrupted data");
				LOG_ERROR("  FAILED: Move self-assignment is safe");
				allPassed = false;
			}
		}

		return allPassed;
	}
};

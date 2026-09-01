#pragma once

#include "lib/runtime.h"
#include "tests.h"

class BufferTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running Buffer Tests...");

		RunTest(allPassed, &TestBasicsSuite, "Basics suite");
		RunTest(allPassed, &TestGrowthSuite, "Growth suite");
		RunTest(allPassed, &TestMoveSemanticsSuite, "Move semantics suite");
		RunTest(allPassed, &TestEdgeCasesSuite, "Edge cases suite");

		if (allPassed)
			LOG_INFO("All Buffer tests passed!");
		else
			LOG_ERROR("Some Buffer tests failed!");

		return allPassed;
	}

private:
	static BOOL TestBasicsSuite()
	{
		BOOL allPassed = true;

		// --- Default construction ---
		{
			Buffer<UINT8> b;
			BOOL passed = true;

			if (b.Data != nullptr)
			{
				LOG_ERROR("Default Data != nullptr");
				passed = false;
			}
			if (passed && (b.Capacity != 0 || b.Size != 0))
			{
				LOG_ERROR("Default Capacity/Size != 0");
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

		// --- Init ---
		{
			Buffer<UINT8> b;
			BOOL passed = true;

			if (!b.Init(64))
			{
				LOG_ERROR("Init(64) returned false");
				passed = false;
			}
			if (passed && b.Data == nullptr)
			{
				LOG_ERROR("Data == nullptr after Init");
				passed = false;
			}
			if (passed && b.Capacity != 64)
			{
				LOG_ERROR("Capacity != 64 after Init");
				passed = false;
			}
			if (passed && b.Size != 0)
			{
				LOG_ERROR("Size != 0 after Init");
				passed = false;
			}
			// Re-init of an initialized buffer must fail (no leak of the old array)
			if (passed && b.Init(32))
			{
				LOG_ERROR("Init() on initialized buffer returned true");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Init allocates and stays empty");
			else
			{
				LOG_ERROR("  FAILED: Init allocates and stays empty");
				allPassed = false;
			}
		}

		// --- Add single ---
		{
			Buffer<UINT8> b;
			if (!b.Init(4)) return false;
			BOOL passed = true;

			if (!b.Add(42))
			{
				LOG_ERROR("Add(42) returned false");
				passed = false;
			}
			if (passed && b.Size != 1)
			{
				LOG_ERROR("Size != 1 after Add");
				passed = false;
			}
			if (passed && b.Data[0] != 42)
			{
				LOG_ERROR("Data[0] != 42");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Add single element");
			else
			{
				LOG_ERROR("  FAILED: Add single element");
				allPassed = false;
			}
		}

		// --- Append span ---
		{
			Buffer<CHAR> b;
			if (!b.Init(8)) return false;
			BOOL passed = true;

			const CHAR *text = "hello";
			if (!b.Append(Span<const CHAR>(text, 5)))
			{
				LOG_ERROR("Append returned false");
				passed = false;
			}
			if (passed && b.Size != 5)
			{
				LOG_ERROR("Size != 5 after Append");
				passed = false;
			}
			if (passed && b.Data[0] != 'h' && b.Data[4] != 'o')
			{
				LOG_ERROR("Appended bytes incorrect");
				passed = false;
			}
			// AsSpan must be bounded by Size, not Capacity
			if (passed && b.AsSpan().Size() != 5)
			{
				LOG_ERROR("AsSpan().Size() != Size");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Append span and AsSpan bounds");
			else
			{
				LOG_ERROR("  FAILED: Append span and AsSpan bounds");
				allPassed = false;
			}
		}

		// --- Unused tail ---
		{
			Buffer<UINT8> b;
			if (!b.Init(16)) return false;
			if (!b.Append(Span<const UINT8>((const UINT8 *)"1234", 4))) return false;
			BOOL passed = true;

			if (b.Unused().Size() != 12)
			{
				LOG_ERROR("Unused().Size() = %u, expected 12", (UINT32)b.Unused().Size());
				passed = false;
			}
			if (passed && b.Unused().Data() != b.Data + 4)
			{
				LOG_ERROR("Unused().Data() != Data + Size");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Unused tail region");
			else
			{
				LOG_ERROR("  FAILED: Unused tail region");
				allPassed = false;
			}
		}

		return allPassed;
	}

	static BOOL TestGrowthSuite()
	{
		BOOL allPassed = true;

		// --- Reserve growth policy ---
		{
			Buffer<UINT8> b;
			BOOL passed = true;

			// Growth from empty uses the default initial capacity
			if (!b.Reserve(1))
			{
				LOG_ERROR("Reserve(1) returned false");
				passed = false;
			}
			if (passed && b.Capacity != BufferInitialCapacity)
			{
				LOG_ERROR("Capacity = %u, expected %u", (UINT32)b.Capacity, (UINT32)BufferInitialCapacity);
				passed = false;
			}
			if (passed && b.Size != 0)
			{
				LOG_ERROR("Size != 0 after Reserve");
				passed = false;
			}

			// Doubling until the request fits
			USIZE initialCap = b.Capacity;
			if (passed && !b.Reserve(initialCap * 4))
			{
				LOG_ERROR("Reserve(%u) returned false", (UINT32)(initialCap * 4));
				passed = false;
			}
			if (passed && b.Capacity < initialCap * 4)
			{
				LOG_ERROR("Capacity = %u, expected >= %u", (UINT32)b.Capacity, (UINT32)(initialCap * 4));
				passed = false;
			}

			// Reserve within capacity is a no-op
			USIZE grownCap = b.Capacity;
			if (passed && !b.Reserve(grownCap - 1))
			{
				LOG_ERROR("Reserve(within capacity) returned false");
				passed = false;
			}
			if (passed && b.Capacity != grownCap)
			{
				LOG_ERROR("Capacity changed on no-op Reserve");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Reserve growth policy");
			else
			{
				LOG_ERROR("  FAILED: Reserve growth policy");
				allPassed = false;
			}
		}

		// --- Add beyond capacity doubles ---
		{
			Buffer<UINT32> b;
			if (!b.Init(8)) return false;
			BOOL passed = true;

			// Filling exactly to capacity does not grow (Reserve only grows when
			// the request exceeds Capacity, unlike Vector's eager off-by-one).
			USIZE initialCap = b.Capacity;
			for (USIZE i = 0; i < initialCap; i++)
			{
				if (!b.Add((UINT32)(i * 7)))
				{
					LOG_ERROR("Add failed at index %u", (UINT32)i);
					passed = false;
					break;
				}
			}
			if (passed && b.Capacity != initialCap)
			{
				LOG_ERROR("Capacity = %u, expected %u (no growth at exactly full)", (UINT32)b.Capacity, (UINT32)initialCap);
				passed = false;
			}

			// One more element past capacity doubles the allocation
			if (passed && !b.Add((UINT32)(initialCap * 7)))
			{
				LOG_ERROR("Add past capacity returned false");
				passed = false;
			}
			if (passed && b.Capacity != initialCap * 2)
			{
				LOG_ERROR("Capacity = %u, expected %u (double)", (UINT32)b.Capacity, (UINT32)(initialCap * 2));
				passed = false;
			}
			if (passed && b.Size != initialCap + 1)
			{
				LOG_ERROR("Size = %u, expected %u", (UINT32)b.Size, (UINT32)(initialCap + 1));
				passed = false;
			}
			if (passed && (b.Data[0] != 0 || b.Data[initialCap] != (UINT32)(initialCap * 7)))
			{
				LOG_ERROR("Data values incorrect after growth");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Add beyond capacity triggers growth");
			else
			{
				LOG_ERROR("  FAILED: Add beyond capacity triggers growth");
				allPassed = false;
			}
		}

		// --- Growth preserves data ---
		{
			Buffer<UINT8> b;
			BOOL passed = true;

			// Append enough chunks to trigger several doublings, verifying contents each time
			UINT8 chunk[10];
			for (UINT8 i = 0; i < 10; i++)
				chunk[i] = (UINT8)(i + 1);

			USIZE total = 0;
			for (INT32 round = 0; round < 12; round++)
			{
				if (!b.Append(Span<const UINT8>(chunk, 10)))
				{
					LOG_ERROR("Append failed on round %d", round);
					passed = false;
					break;
				}
				total += 10;
			}
			if (passed && b.Size != total)
			{
				LOG_ERROR("Size = %u, expected %u", (UINT32)b.Size, (UINT32)total);
				passed = false;
			}
			if (passed)
			{
				for (USIZE i = 0; i < total; i++)
				{
					UINT8 expected = (UINT8)((i % 10) + 1);
					if (b.Data[i] != expected)
					{
						LOG_ERROR("Data[%u] = %u, expected %u", (UINT32)i, b.Data[i], expected);
						passed = false;
						break;
					}
				}
			}

			if (passed)
				LOG_INFO("  PASSED: Growth preserves existing data");
			else
			{
				LOG_ERROR("  FAILED: Growth preserves existing data");
				allPassed = false;
			}
		}

		// --- Resize ---
		{
			Buffer<UINT8> b;
			if (!b.Init(4)) return false;
			BOOL passed = true;

			// Grow via Resize
			if (!b.Resize(64))
			{
				LOG_ERROR("Resize(64) returned false");
				passed = false;
			}
			if (passed && b.Size != 64)
			{
				LOG_ERROR("Size != 64 after Resize");
				passed = false;
			}
			// Shrink is logical only — allocation is kept
			USIZE capBefore = b.Capacity;
			if (passed && !b.Resize(4))
			{
				LOG_ERROR("Resize(4) returned false");
				passed = false;
			}
			if (passed && b.Size != 4)
			{
				LOG_ERROR("Size != 4 after shrink Resize");
				passed = false;
			}
			if (passed && b.Capacity != capBefore)
			{
				LOG_ERROR("Capacity changed on shrink Resize");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Resize grows and never shrinks allocation");
			else
			{
				LOG_ERROR("  FAILED: Resize grows and never shrinks allocation");
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
			Buffer<INT32> b;
			if (!b.Init(8)) return false;
			if (!b.Add(1)) return false;
			if (!b.Add(2)) return false;

			INT32 *origData = b.Data;
			USIZE origSize = b.Size;
			USIZE origCap = b.Capacity;

			Buffer<INT32> b2((Buffer<INT32> &&)b);

			BOOL passed = true;

			if (b.Data != nullptr || b.Capacity != 0 || b.Size != 0)
			{
				LOG_ERROR("Source not zeroed after move construct");
				passed = false;
			}
			if (passed && (b2.Data != origData || b2.Size != origSize || b2.Capacity != origCap))
			{
				LOG_ERROR("Destination does not match original after move construct");
				passed = false;
			}
			if (passed && (b2.Data[0] != 1 || b2.Data[1] != 2))
			{
				LOG_ERROR("Data values incorrect after move construct");
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
			Buffer<INT32> b;
			if (!b.Init(8)) return false;
			if (!b.Add(10)) return false;
			if (!b.Add(20)) return false;

			INT32 *origData = b.Data;

			Buffer<INT32> b2;
			b2 = (Buffer<INT32> &&)b;

			BOOL passed = true;

			if (b.Data != nullptr || b.Capacity != 0 || b.Size != 0)
			{
				LOG_ERROR("Source not zeroed after move assign");
				passed = false;
			}
			if (passed && (b2.Data != origData || b2.Size != 2))
			{
				LOG_ERROR("Destination incorrect after move assign");
				passed = false;
			}
			if (passed && (b2.Data[0] != 10 || b2.Data[1] != 20))
			{
				LOG_ERROR("Data values incorrect after move assign");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Move assignment transfers ownership");
			else
			{
				LOG_ERROR("  FAILED: Move assignment transfers ownership");
				allPassed = false;
			}
		}

		// --- Self-assign ---
		{
			Buffer<INT32> b;
			if (!b.Init(8)) return false;
			if (!b.Add(99)) return false;

			INT32 *origData = b.Data;
			b = (Buffer<INT32> &&)b;

			BOOL passed = b.Data == origData && b.Size == 1 && b.Data[0] == 99;

			if (passed)
				LOG_INFO("  PASSED: Move self-assignment is safe");
			else
			{
				LOG_ERROR("Self move-assign corrupted data");
				LOG_ERROR("  FAILED: Move self-assignment is safe");
				allPassed = false;
			}
		}

		// --- Move assign into non-empty ---
		{
			Buffer<INT32> b1;
			if (!b1.Init(4)) return false;
			if (!b1.Add(100)) return false;

			Buffer<INT32> b2;
			if (!b2.Init(4)) return false;
			if (!b2.Add(200)) return false;
			if (!b2.Add(300)) return false;

			b2 = (Buffer<INT32> &&)b1;

			BOOL passed = true;

			if (b2.Size != 1 || b2.Data[0] != 100)
			{
				LOG_ERROR("b2 should have b1's data after move");
				passed = false;
			}
			if (passed && (b1.Data != nullptr || b1.Size != 0))
			{
				LOG_ERROR("b1 not zeroed after move");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Move assign into non-empty buffer");
			else
			{
				LOG_ERROR("  FAILED: Move assign into non-empty buffer");
				allPassed = false;
			}
		}

		return allPassed;
	}

	static BOOL TestEdgeCasesSuite()
	{
		BOOL allPassed = true;

		// --- Release ---
		{
			Buffer<UINT8> b;
			if (!b.Init(8)) return false;
			if (!b.Add(7)) return false;
			if (!b.Add(8)) return false;

			UINT8 *released = b.Release();
			BOOL passed = true;

			if (released == nullptr)
			{
				LOG_ERROR("Release() returned nullptr");
				passed = false;
			}
			if (passed && (released[0] != 7 || released[1] != 8))
			{
				LOG_ERROR("Released data values incorrect");
				passed = false;
			}
			if (passed && (b.Data != nullptr || b.Capacity != 0 || b.Size != 0))
			{
				LOG_ERROR("Buffer not reset after Release()");
				passed = false;
			}

			if (released)
				delete[] released;

			if (passed)
				LOG_INFO("  PASSED: Release returns pointer and resets");
			else
			{
				LOG_ERROR("  FAILED: Release returns pointer and resets");
				allPassed = false;
			}
		}

		// --- Release of an empty buffer ---
		{
			Buffer<UINT8> b;
			BOOL passed = b.Release() == nullptr && b.Data == nullptr && b.Size == 0;

			if (passed)
				LOG_INFO("  PASSED: Release on empty buffer returns nullptr");
			else
			{
				LOG_ERROR("  FAILED: Release on empty buffer returns nullptr");
				allPassed = false;
			}
		}

		// --- Reset then reuse ---
		{
			Buffer<UINT8> b;
			if (!b.Init(8)) return false;
			for (UINT8 i = 0; i < 8; i++)
				if (!b.Add(i)) return false;

			USIZE capBefore = b.Capacity;
			b.Reset();

			BOOL passed = true;
			if (b.Size != 0)
			{
				LOG_ERROR("Size != 0 after Reset");
				passed = false;
			}
			if (passed && b.Capacity != capBefore)
			{
				LOG_ERROR("Capacity not retained after Reset");
				passed = false;
			}
			// Reuse: appending within the retained capacity must not reallocate
			UINT8 *dataBefore = b.Data;
			if (passed && !b.Append(Span<const UINT8>((const UINT8 *)"abc", 3)))
			{
				LOG_ERROR("Append after Reset returned false");
				passed = false;
			}
			if (passed && b.Data != dataBefore)
			{
				LOG_ERROR("Backing array changed on reuse within retained capacity");
				passed = false;
			}
			if (passed && (b.Data[0] != 'a' || b.Data[2] != 'c' || b.Size != 3))
			{
				LOG_ERROR("Reused region contents incorrect");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Reset keeps capacity for reuse");
			else
			{
				LOG_ERROR("  FAILED: Reset keeps capacity for reuse");
				allPassed = false;
			}
		}

		// --- Zero-length Append ---
		{
			Buffer<UINT8> b;
			if (!b.Init(4)) return false;
			BOOL passed = b.Append(Span<const UINT8>()) && b.Size == 0;

			if (passed)
				LOG_INFO("  PASSED: Zero-length Append is a no-op");
			else
			{
				LOG_ERROR("  FAILED: Zero-length Append is a no-op");
				allPassed = false;
			}
		}

		return allPassed;
	}
};

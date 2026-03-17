#pragma once

#include "lib/runtime.h"
#include "tests.h"

class VectorTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running Vector Tests...");

		RunTest(allPassed, &TestBasicsSuite, "Basics suite");
		RunTest(allPassed, &TestMoveSemanticsSuite, "Move semantics suite");
		RunTest(allPassed, &TestEdgeCasesSuite, "Edge cases suite");

		if (allPassed)
			LOG_INFO("All Vector tests passed!");
		else
			LOG_ERROR("Some Vector tests failed!");

		return allPassed;
	}

private:
	static BOOL TestBasicsSuite()
	{
		BOOL allPassed = true;

		// --- Default construction ---
		{
			Vector<INT32> v;
			BOOL passed = true;

			if (v.Data != nullptr)
			{
				LOG_ERROR("Default Data != nullptr");
				passed = false;
			}
			if (passed && (v.Capacity != 0 || v.Count != 0))
			{
				LOG_ERROR("Default Capacity/Count != 0");
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
			Vector<INT32> v;
			BOOL passed = true;

			if (!v.Init())
			{
				LOG_ERROR("Init() returned false");
				passed = false;
			}
			if (passed && v.Data == nullptr)
			{
				LOG_ERROR("Data == nullptr after Init");
				passed = false;
			}
			if (passed && v.Capacity != VectorInitialCapacity)
			{
				LOG_ERROR("Capacity != VectorInitialCapacity after Init");
				passed = false;
			}
			if (passed && v.Count != 0)
			{
				LOG_ERROR("Count != 0 after Init");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Init allocates backing array");
			else
			{
				LOG_ERROR("  FAILED: Init allocates backing array");
				allPassed = false;
			}
		}

		// --- Add single ---
		{
			Vector<INT32> v;
			if (!v.Init()) return false;
			BOOL passed = true;

			if (!v.Add(42))
			{
				LOG_ERROR("Add(42) returned false");
				passed = false;
			}
			if (passed && v.Count != 1)
			{
				LOG_ERROR("Count != 1 after Add");
				passed = false;
			}
			if (passed && v.Data[0] != 42)
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

		// --- Add multiple ---
		{
			Vector<INT32> v;
			if (!v.Init()) return false;
			BOOL passed = true;

			for (INT32 i = 0; i < 5; i++)
			{
				if (!v.Add(i * 10))
				{
					LOG_ERROR("Add failed at index %d", i);
					passed = false;
					break;
				}
			}
			if (passed && v.Count != 5)
			{
				LOG_ERROR("Count != 5");
				passed = false;
			}
			if (passed)
			{
				for (INT32 i = 0; i < 5; i++)
				{
					if (v.Data[i] != i * 10)
					{
						LOG_ERROR("Data[%d] != %d", i, i * 10);
						passed = false;
						break;
					}
				}
			}

			if (passed)
				LOG_INFO("  PASSED: Add multiple elements");
			else
			{
				LOG_ERROR("  FAILED: Add multiple elements");
				allPassed = false;
			}
		}

		// --- Add grow ---
		{
			Vector<INT32> v;
			if (!v.Init()) return false;
			BOOL passed = true;

			INT32 initialCap = v.Capacity;

			// Fill to capacity (Add triggers growth when Count + 1 >= Capacity)
			for (INT32 i = 0; i < initialCap; i++)
			{
				if (!v.Add(i))
				{
					LOG_ERROR("Add failed at index %d", i);
					passed = false;
					break;
				}
			}

			if (passed && v.Capacity <= initialCap)
			{
				LOG_ERROR("Capacity did not grow: still %d", v.Capacity);
				passed = false;
			}
			if (passed && v.Capacity != initialCap * 2)
			{
				LOG_ERROR("Capacity != %d (expected double)", initialCap * 2);
				passed = false;
			}
			if (passed && v.Count != initialCap)
			{
				LOG_ERROR("Count != %d after filling", initialCap);
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Add beyond initial capacity triggers growth");
			else
			{
				LOG_ERROR("  FAILED: Add beyond initial capacity triggers growth");
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
			Vector<INT32> v;
			if (!v.Init()) return false;
			if (!v.Add(1)) return false;
			if (!v.Add(2)) return false;

			INT32 *origData = v.Data;
			INT32 origCount = v.Count;
			INT32 origCap = v.Capacity;

			Vector<INT32> v2((Vector<INT32> &&)v);

			BOOL passed = true;

			// Source should be empty
			if (v.Data != nullptr || v.Capacity != 0 || v.Count != 0)
			{
				LOG_ERROR("Source not zeroed after move construct");
				passed = false;
			}
			// Destination should have the data
			if (passed && (v2.Data != origData || v2.Count != origCount || v2.Capacity != origCap))
			{
				LOG_ERROR("Destination does not match original after move construct");
				passed = false;
			}
			if (passed && (v2.Data[0] != 1 || v2.Data[1] != 2))
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
			Vector<INT32> v;
			if (!v.Init()) return false;
			if (!v.Add(10)) return false;
			if (!v.Add(20)) return false;

			INT32 *origData = v.Data;

			Vector<INT32> v2;
			v2 = (Vector<INT32> &&)v;

			BOOL passed = true;

			if (v.Data != nullptr || v.Capacity != 0 || v.Count != 0)
			{
				LOG_ERROR("Source not zeroed after move assign");
				passed = false;
			}
			if (passed && (v2.Data != origData || v2.Count != 2))
			{
				LOG_ERROR("Destination incorrect after move assign");
				passed = false;
			}
			if (passed && (v2.Data[0] != 10 || v2.Data[1] != 20))
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
			Vector<INT32> v;
			if (!v.Init()) return false;
			if (!v.Add(99)) return false;

			INT32 *origData = v.Data;
			v = (Vector<INT32> &&)v;

			BOOL passed = v.Data == origData && v.Count == 1 && v.Data[0] == 99;

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
			Vector<INT32> v1;
			if (!v1.Init()) return false;
			if (!v1.Add(100)) return false;

			Vector<INT32> v2;
			if (!v2.Init()) return false;
			if (!v2.Add(200)) return false;
			if (!v2.Add(300)) return false;

			// Move v1 into v2 — v2's old data should be freed
			v2 = (Vector<INT32> &&)v1;

			BOOL passed = true;

			if (v2.Count != 1 || v2.Data[0] != 100)
			{
				LOG_ERROR("v2 should have v1's data after move");
				passed = false;
			}
			if (passed && (v1.Data != nullptr || v1.Count != 0))
			{
				LOG_ERROR("v1 not zeroed after move");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Move assign into non-empty vector");
			else
			{
				LOG_ERROR("  FAILED: Move assign into non-empty vector");
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
			Vector<INT32> v;
			if (!v.Init()) return false;
			if (!v.Add(7)) return false;
			if (!v.Add(8)) return false;

			INT32 *released = v.Release();
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
			if (passed && (v.Data != nullptr || v.Capacity != 0 || v.Count != 0))
			{
				LOG_ERROR("Vector not reset after Release()");
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

		// --- Growth preserves data ---
		{
			Vector<INT32> v;
			if (!v.Init()) return false;
			BOOL passed = true;

			// Fill past initial capacity
			for (INT32 i = 0; i < VectorInitialCapacity + 5; i++)
			{
				if (!v.Add(i * 3))
				{
					LOG_ERROR("Add failed at index %d", i);
					passed = false;
					break;
				}
			}

			// Verify all data is intact
			if (passed)
			{
				for (INT32 i = 0; i < VectorInitialCapacity + 5; i++)
				{
					if (v.Data[i] != i * 3)
					{
						LOG_ERROR("Data[%d] = %d, expected %d", i, v.Data[i], i * 3);
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

		// --- Multiple growths ---
		{
			Vector<INT32> v;
			if (!v.Init()) return false;
			BOOL passed = true;

			// Add enough elements to trigger multiple doublings
			// Initial = 10, then 20, 40, 80 => need > 40 elements for 3 growths
			INT32 target = VectorInitialCapacity * 4 + 1;
			for (INT32 i = 0; i < target; i++)
			{
				if (!v.Add(i))
				{
					LOG_ERROR("Add failed at index %d", i);
					passed = false;
					break;
				}
			}

			if (passed && v.Count != target)
			{
				LOG_ERROR("Count = %d, expected %d", v.Count, target);
				passed = false;
			}

			// Spot check some values
			if (passed && (v.Data[0] != 0 || v.Data[target - 1] != target - 1))
			{
				LOG_ERROR("Data mismatch after multiple growths");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Multiple capacity doublings");
			else
			{
				LOG_ERROR("  FAILED: Multiple capacity doublings");
				allPassed = false;
			}
		}

		return allPassed;
	}
};

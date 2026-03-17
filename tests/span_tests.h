#pragma once

#include "lib/runtime.h"
#include "tests.h"

class SpanTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running Span Tests...");

		RunTest(allPassed, &TestDynamicExtentSuite, "Dynamic extent suite");
		RunTest(allPassed, &TestStaticExtentSuite, "Static extent suite");
		RunTest(allPassed, &TestCrossConversionSuite, "Cross-conversion suite");
		RunTest(allPassed, &TestCompileTimeSlicingSuite, "Compile-time slicing suite");
		RunTest(allPassed, &TestEdgeCasesSuite, "Edge cases suite");

		if (allPassed)
			LOG_INFO("All Span tests passed!");
		else
			LOG_ERROR("Some Span tests failed!");

		return allPassed;
	}

private:
	// Helper: accepts Span<const UINT8> (dynamic) to verify implicit conversions
	static USIZE SumBytes(Span<const UINT8> data)
	{
		USIZE sum = 0;
		for (USIZE i = 0; i < data.Size(); i++)
			sum += data[i];
		return sum;
	}

	// =====================================================================
	// Dynamic extent suite
	// =====================================================================

	static BOOL TestDynamicExtentSuite()
	{
		BOOL allPassed = true;

		// Default construction
		{
			Span<UINT8> s;
			if (s.Data() != nullptr)
			{
				LOG_ERROR("Default Data() != nullptr");
				allPassed = false;
			}
			else if (s.Size() != 0 || !s.IsEmpty())
			{
				LOG_ERROR("Default Size() != 0 or not empty");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Dynamic default construction");
			}
		}

		// Pointer+size construction
		{
			UINT8 buf[4];
			buf[0] = 1; buf[1] = 2; buf[2] = 3; buf[3] = 4;
			Span<UINT8> s(buf, 4);
			if (s.Data() != buf)
			{
				LOG_ERROR("  FAILED: Dynamic pointer+size construction (Data() != buf)");
				allPassed = false;
			}
			else if (s.Size() != 4 || s.IsEmpty())
			{
				LOG_ERROR("  FAILED: Dynamic pointer+size construction (Size() != 4 or empty)");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Dynamic pointer+size construction");
			}
		}

		// Array construction
		{
			UINT8 buf[8];
			buf[0] = 42;
			Span<UINT8> s(buf);
			if (s.Data() != buf || s.Size() != 8)
			{
				LOG_ERROR("  FAILED: Dynamic array construction (Data or Size mismatch)");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Dynamic array construction");
			}
		}

		// Accessors
		{
			UINT8 buf[3];
			buf[0] = 10; buf[1] = 20; buf[2] = 30;
			Span<UINT8> s(buf, 3);
			if (s[0] != 10 || s[1] != 20 || s[2] != 30)
			{
				LOG_ERROR("  FAILED: Dynamic accessors (operator[] mismatch)");
				allPassed = false;
			}
			else if (s.SizeBytes() != 3 * sizeof(UINT8))
			{
				LOG_ERROR("  FAILED: Dynamic accessors (SizeBytes() mismatch)");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Dynamic accessors");
			}
		}

		// Subspan
		{
			UINT8 buf[5];
			buf[0] = 1; buf[1] = 2; buf[2] = 3; buf[3] = 4; buf[4] = 5;
			Span<UINT8> s(buf, 5);

			Span<UINT8> sub = s.Subspan(2);
			if (sub.Size() != 3 || sub[0] != 3)
			{
				LOG_ERROR("  FAILED: Dynamic Subspan (Subspan(offset) failed)");
				allPassed = false;
			}
			else
			{
				Span<UINT8> sub2 = s.Subspan(1, 2);
				if (sub2.Size() != 2 || sub2[0] != 2 || sub2[1] != 3)
				{
					LOG_ERROR("  FAILED: Dynamic Subspan (Subspan(offset, count) failed)");
					allPassed = false;
				}
				else
				{
					LOG_INFO("  PASSED: Dynamic Subspan");
				}
			}
		}

		// First/Last
		{
			UINT8 buf[4];
			buf[0] = 1; buf[1] = 2; buf[2] = 3; buf[3] = 4;
			Span<UINT8> s(buf, 4);

			Span<UINT8> first = s.First(2);
			if (first.Size() != 2 || first[0] != 1 || first[1] != 2)
			{
				LOG_ERROR("  FAILED: Dynamic First/Last (First(2) failed)");
				allPassed = false;
			}
			else
			{
				Span<UINT8> last = s.Last(2);
				if (last.Size() != 2 || last[0] != 3 || last[1] != 4)
				{
					LOG_ERROR("  FAILED: Dynamic First/Last (Last(2) failed)");
					allPassed = false;
				}
				else
				{
					LOG_INFO("  PASSED: Dynamic First/Last");
				}
			}
		}

		// Range-for iteration
		{
			UINT8 buf[3];
			buf[0] = 10; buf[1] = 20; buf[2] = 30;
			Span<UINT8> s(buf, 3);
			USIZE sum = 0;
			for (UINT8 v : s)
				sum += v;
			if (sum != 60)
			{
				LOG_ERROR("  FAILED: Dynamic range-for iteration (sum mismatch: expected 60)");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Dynamic range-for iteration");
			}
		}

		// Const conversion
		{
			UINT8 buf[4];
			buf[0] = 5; buf[1] = 10; buf[2] = 15; buf[3] = 20;
			Span<UINT8> writable(buf, 4);
			Span<const UINT8> readable = writable;
			if (readable.Size() != 4 || readable[0] != 5)
			{
				LOG_ERROR("  FAILED: Dynamic Span<T> to Span<const T>");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Dynamic Span<T> to Span<const T>");
			}
		}

		return allPassed;
	}

	// =====================================================================
	// Static extent suite
	// =====================================================================

	static BOOL TestStaticExtentSuite()
	{
		BOOL allPassed = true;

		// Array construction
		{
			UINT8 buf[4];
			buf[0] = 1; buf[1] = 2; buf[2] = 3; buf[3] = 4;
			Span<UINT8, 4> s(buf);
			if (s.Data() != buf || s.Size() != 4 || s.IsEmpty())
			{
				LOG_ERROR("  FAILED: Static array construction");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Static array construction");
			}
		}

		// Accessors
		{
			UINT8 buf[3];
			buf[0] = 10; buf[1] = 20; buf[2] = 30;
			Span<UINT8, 3> s(buf);
			if (s.Size() != 3 || s.SizeBytes() != 3 || s[1] != 20)
			{
				LOG_ERROR("  FAILED: Static accessors");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Static accessors");
			}
		}

		// Subspan returns dynamic
		{
			UINT8 buf[5];
			buf[0] = 1; buf[1] = 2; buf[2] = 3; buf[3] = 4; buf[4] = 5;
			Span<UINT8, 5> s(buf);

			Span<UINT8> sub = s.Subspan(2);
			if (sub.Size() != 3 || sub[0] != 3)
			{
				LOG_ERROR("  FAILED: Static Subspan (Subspan(offset) failed)");
				allPassed = false;
			}
			else
			{
				Span<UINT8> sub2 = s.Subspan(1, 2);
				if (sub2.Size() != 2 || sub2[0] != 2)
				{
					LOG_ERROR("  FAILED: Static Subspan (Subspan(offset, count) failed)");
					allPassed = false;
				}
				else
				{
					LOG_INFO("  PASSED: Static Subspan returns dynamic");
				}
			}
		}

		// First/Last return dynamic
		{
			UINT8 buf[4];
			buf[0] = 1; buf[1] = 2; buf[2] = 3; buf[3] = 4;
			Span<UINT8, 4> s(buf);

			Span<UINT8> first = s.First(2);
			if (first.Size() != 2 || first[0] != 1 || first[1] != 2)
			{
				LOG_ERROR("  FAILED: Static First/Last (First(2) failed)");
				allPassed = false;
			}
			else
			{
				Span<UINT8> last = s.Last(2);
				if (last.Size() != 2 || last[0] != 3 || last[1] != 4)
				{
					LOG_ERROR("  FAILED: Static First/Last (Last(2) failed)");
					allPassed = false;
				}
				else
				{
					LOG_INFO("  PASSED: Static First/Last return dynamic");
				}
			}
		}

		// Range-for iteration
		{
			UINT8 buf[3];
			buf[0] = 10; buf[1] = 20; buf[2] = 30;
			Span<UINT8, 3> s(buf);
			USIZE sum = 0;
			for (UINT8 v : s)
				sum += v;
			if (sum != 60)
			{
				LOG_ERROR("  FAILED: Static range-for iteration (sum mismatch: expected 60)");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Static range-for iteration");
			}
		}

		// Const conversion
		{
			UINT8 buf[4];
			buf[0] = 5;
			Span<UINT8, 4> writable(buf);
			Span<const UINT8, 4> readable = writable;
			if (readable.Size() != 4 || readable[0] != 5)
			{
				LOG_ERROR("  FAILED: Static Span<T,N> to Span<const T,N>");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Static Span<T,N> to Span<const T,N>");
			}
		}

		return allPassed;
	}

	// =====================================================================
	// Cross-conversion suite
	// =====================================================================

	static BOOL TestCrossConversionSuite()
	{
		BOOL allPassed = true;

		// Static-to-dynamic
		{
			UINT8 buf[4];
			buf[0] = 1; buf[1] = 2; buf[2] = 3; buf[3] = 4;
			Span<UINT8, 4> staticSpan(buf);
			Span<UINT8> dynamicSpan = staticSpan;
			if (dynamicSpan.Size() != 4 || dynamicSpan[0] != 1)
			{
				LOG_ERROR("  FAILED: Span<T,N> to Span<T> conversion");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Span<T,N> to Span<T> conversion");
			}
		}

		// Static-to-dynamic-const
		{
			UINT8 buf[4];
			buf[0] = 1;
			Span<UINT8, 4> staticSpan(buf);
			Span<const UINT8> readOnly = staticSpan;
			if (readOnly.Size() != 4 || readOnly[0] != 1)
			{
				LOG_ERROR("  FAILED: Span<T,N> to Span<const T> conversion");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Span<T,N> to Span<const T> conversion");
			}
		}

		// Pass to function
		{
			UINT8 buf[3];
			buf[0] = 10; buf[1] = 20; buf[2] = 30;
			Span<UINT8, 3> staticSpan(buf);
			// SumBytes takes Span<const UINT8> (dynamic) -- implicit conversion
			USIZE result = SumBytes(staticSpan);
			if (result != 60)
			{
				LOG_ERROR("  FAILED: Pass Span<T,N> to function taking Span<T> (expected 60, got %zu)", result);
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Pass Span<T,N> to function taking Span<T>");
			}
		}

		// Size optimization
		{
			// Static extent stores only pointer (no m_size member)
			static_assert(sizeof(Span<UINT8, 4>) == sizeof(UINT8 *));
			// Dynamic extent stores pointer + size
			static_assert(sizeof(Span<UINT8>) == sizeof(UINT8 *) + sizeof(USIZE));
			LOG_INFO("  PASSED: Static extent eliminates size member");
		}

		return allPassed;
	}

	// =====================================================================
	// Compile-time slicing suite
	// =====================================================================

	static BOOL TestCompileTimeSlicingSuite()
	{
		BOOL allPassed = true;

		// Static explicit pointer construction
		{
			UINT8 buf[4];
			buf[0] = 7; buf[1] = 8; buf[2] = 9; buf[3] = 10;
			UINT8 *ptr = buf;
			Span<UINT8, 4> s(ptr);  // explicit pointer constructor
			if (s.Data() != buf || s.Size() != 4 || s[0] != 7)
			{
				LOG_ERROR("  FAILED: Static explicit pointer construction");
				allPassed = false;
			}
			else
			{
				static_assert(sizeof(Span<UINT8, 4>) == sizeof(UINT8 *), "Static span from pointer must store only pointer");
				LOG_INFO("  PASSED: Static explicit pointer construction");
			}
		}

		// Static First<N>
		{
			UINT8 buf[8];
			buf[0] = 1; buf[1] = 2; buf[2] = 3; buf[3] = 4;
			buf[4] = 5; buf[5] = 6; buf[6] = 7; buf[7] = 8;
			Span<UINT8, 8> s(buf);

			Span<UINT8, 3> first = s.First<3>();
			static_assert(sizeof(first) == sizeof(UINT8 *), "First<N>() must return pointer-only span");
			if (first.Size() != 3 || first[0] != 1 || first[2] != 3)
			{
				LOG_ERROR("  FAILED: Static First<N>() returns Span<T,N>");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Static First<N>() returns Span<T,N>");
			}
		}

		// Static Last<N>
		{
			UINT8 buf[8];
			buf[0] = 1; buf[1] = 2; buf[2] = 3; buf[3] = 4;
			buf[4] = 5; buf[5] = 6; buf[6] = 7; buf[7] = 8;
			Span<UINT8, 8> s(buf);

			Span<UINT8, 3> last = s.Last<3>();
			static_assert(sizeof(last) == sizeof(UINT8 *), "Last<N>() must return pointer-only span");
			if (last.Size() != 3 || last[0] != 6 || last[2] != 8)
			{
				LOG_ERROR("  FAILED: Static Last<N>() returns Span<T,N>");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Static Last<N>() returns Span<T,N>");
			}
		}

		// Static Subspan<O,N>
		{
			UINT8 buf[8];
			buf[0] = 1; buf[1] = 2; buf[2] = 3; buf[3] = 4;
			buf[4] = 5; buf[5] = 6; buf[6] = 7; buf[7] = 8;
			Span<UINT8, 8> s(buf);

			Span<UINT8, 4> mid = s.Subspan<2, 4>();
			static_assert(sizeof(mid) == sizeof(UINT8 *), "Subspan<O,N>() must return pointer-only span");
			if (mid.Size() != 4 || mid[0] != 3 || mid[3] != 6)
			{
				LOG_ERROR("  FAILED: Static Subspan<O,N>() returns Span<T,N>");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Static Subspan<O,N>() returns Span<T,N>");
			}
		}

		// Static Subspan<O> deduces count
		{
			UINT8 buf[8];
			buf[0] = 1; buf[1] = 2; buf[2] = 3; buf[3] = 4;
			buf[4] = 5; buf[5] = 6; buf[6] = 7; buf[7] = 8;
			Span<UINT8, 8> s(buf);

			auto tail = s.Subspan<3>();  // Span<UINT8, 8-3> = Span<UINT8, 5>
			static_assert(__is_same_as(decltype(tail), Span<UINT8, 5>), "Subspan<O>() must deduce count from extent");
			static_assert(sizeof(tail) == sizeof(UINT8 *), "Subspan<O>() must return pointer-only span");
			if (tail.Size() != 5 || tail[0] != 4 || tail[4] != 8)
			{
				LOG_ERROR("  FAILED: Static Subspan<O>() deduces count from type");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Static Subspan<O>() deduces count from type");
			}
		}

		// Dynamic First<N>
		{
			UINT8 buf[8];
			buf[0] = 10; buf[1] = 20; buf[2] = 30; buf[3] = 40;
			buf[4] = 50; buf[5] = 60; buf[6] = 70; buf[7] = 80;
			Span<UINT8> s(buf, 8);

			Span<UINT8, 4> first = s.First<4>();
			static_assert(sizeof(first) == sizeof(UINT8 *), "Dynamic First<N>() must return pointer-only span");
			if (first.Size() != 4 || first[0] != 10 || first[3] != 40)
			{
				LOG_ERROR("  FAILED: Dynamic First<N>() returns Span<T,N>");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Dynamic First<N>() returns Span<T,N>");
			}
		}

		// Dynamic Last<N>
		{
			UINT8 buf[8];
			buf[0] = 10; buf[1] = 20; buf[2] = 30; buf[3] = 40;
			buf[4] = 50; buf[5] = 60; buf[6] = 70; buf[7] = 80;
			Span<UINT8> s(buf, 8);

			Span<UINT8, 4> last = s.Last<4>();
			static_assert(sizeof(last) == sizeof(UINT8 *), "Dynamic Last<N>() must return pointer-only span");
			if (last.Size() != 4 || last[0] != 50 || last[3] != 80)
			{
				LOG_ERROR("  FAILED: Dynamic Last<N>() returns Span<T,N>");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Dynamic Last<N>() returns Span<T,N>");
			}
		}

		// Dynamic Subspan<O,N>
		{
			UINT8 buf[8];
			buf[0] = 10; buf[1] = 20; buf[2] = 30; buf[3] = 40;
			buf[4] = 50; buf[5] = 60; buf[6] = 70; buf[7] = 80;
			Span<UINT8> s(buf, 8);

			Span<UINT8, 3> mid = s.Subspan<2, 3>();
			static_assert(sizeof(mid) == sizeof(UINT8 *), "Dynamic Subspan<O,N>() must return pointer-only span");
			if (mid.Size() != 3 || mid[0] != 30 || mid[2] != 50)
			{
				LOG_ERROR("  FAILED: Dynamic Subspan<O,N>() returns Span<T,N>");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Dynamic Subspan<O,N>() returns Span<T,N>");
			}
		}

		return allPassed;
	}

	// =====================================================================
	// Edge cases suite
	// =====================================================================

	static BOOL TestEdgeCasesSuite()
	{
		BOOL allPassed = true;

		// Empty dynamic span
		{
			Span<UINT8> empty;
			if (!empty.IsEmpty() || empty.Size() != 0 || empty.Data() != nullptr)
			{
				LOG_ERROR("  FAILED: Empty dynamic span");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Empty dynamic span");
			}
		}

		// Single element static span
		{
			UINT8 val[1];
			val[0] = 42;
			Span<UINT8, 1> s(val);
			if (s.Size() != 1 || s[0] != 42 || s.IsEmpty())
			{
				LOG_ERROR("  FAILED: Single element static span");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Single element static span");
			}
		}

		return allPassed;
	}
};

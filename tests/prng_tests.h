#pragma once

#include "lib/runtime.h"
#include "tests.h"

class PrngTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running Prng Tests...");

		RunTest(allPassed, &TestDeterministicSuite, "Deterministic suite");
		RunTest(allPassed, &TestOutputSuite, "Output suite");

		if (allPassed)
			LOG_INFO("All Prng tests passed!");
		else
			LOG_ERROR("Some Prng tests failed!");

		return allPassed;
	}

private:
	static BOOL TestDeterministicSuite()
	{
		BOOL allPassed = true;

		// --- Deterministic sequence ---
		{
			Prng prng(1);
			BOOL passed = true;

			// Expected xorshift64 output for seed=1
			const INT32 expected[] = {1082269761, 201397313, 1854285353, 1432191013, 274305637};

			for (INT32 i = 0; i < 5; i++)
			{
				INT32 val = prng.Get();
				if (val != expected[i])
				{
					LOG_ERROR("Seed 1, index %d: expected %d, got %d", i, expected[i], val);
					passed = false;
					break;
				}
			}

			if (passed)
				LOG_INFO("  PASSED: Deterministic sequence with known seed");
			else
			{
				LOG_ERROR("  FAILED: Deterministic sequence with known seed");
				allPassed = false;
			}
		}

		// --- Different seeds ---
		{
			Prng a(1);
			Prng b(42);

			// First values from different seeds must differ
			INT32 va = a.Get();
			INT32 vb = b.Get();

			if (va != vb)
				LOG_INFO("  PASSED: Different seeds produce different sequences");
			else
			{
				LOG_ERROR("Seeds 1 and 42 produced same first value: %d", va);
				LOG_ERROR("  FAILED: Different seeds produce different sequences");
				allPassed = false;
			}
		}

		// --- Value range ---
		{
			Prng prng(12345);
			BOOL passed = true;

			for (INT32 i = 0; i < 1000; i++)
			{
				INT32 val = prng.Get();
				if (val < 0 || val >= Prng::Max)
				{
					LOG_ERROR("Value out of range: %d (max: %d)", val, Prng::Max);
					passed = false;
					break;
				}
			}

			if (passed)
				LOG_INFO("  PASSED: Values within [0, MAX)");
			else
			{
				LOG_ERROR("  FAILED: Values within [0, MAX)");
				allPassed = false;
			}
		}

		// --- IsSeeded ---
		{
			Prng prng;
			BOOL passed = true;

			if (prng.IsSeeded())
			{
				LOG_ERROR("Default-constructed Prng reports seeded");
				passed = false;
			}

			if (passed)
			{
				prng.Seed(42);
				if (!prng.IsSeeded())
				{
					LOG_ERROR("Prng reports unseeded after Seed(42)");
					passed = false;
				}
			}

			if (passed)
				LOG_INFO("  PASSED: IsSeeded and Seed");
			else
			{
				LOG_ERROR("  FAILED: IsSeeded and Seed");
				allPassed = false;
			}
		}

		return allPassed;
	}

	static BOOL TestOutputSuite()
	{
		BOOL allPassed = true;

		// --- GetArray ---
		{
			Prng prng(99);
			UINT8 buffer[32];
			Memory::Zero(buffer, sizeof(buffer));

			prng.GetArray(Span<UINT8>(buffer, 32));

			// At least some bytes should be non-zero
			BOOL foundNonZero = false;
			for (USIZE i = 0; i < 32; i++)
			{
				if (buffer[i] != 0)
				{
					foundNonZero = true;
					break;
				}
			}

			if (foundNonZero)
				LOG_INFO("  PASSED: GetArray fills buffer");
			else
			{
				LOG_ERROR("All 32 bytes are zero after GetArray");
				LOG_ERROR("  FAILED: GetArray fills buffer");
				allPassed = false;
			}
		}

		// --- GetChar ---
		{
			Prng prng(777);
			BOOL passed = true;

			for (INT32 i = 0; i < 100; i++)
			{
				CHAR c = prng.GetChar<CHAR>();
				if (c < 'a' || c > 'z')
				{
					LOG_ERROR("Char out of range: 0x%02X", (UINT32)(UINT8)c);
					passed = false;
					break;
				}
			}

			if (passed)
			{
				for (INT32 i = 0; i < 100; i++)
				{
					WCHAR c = prng.GetChar<WCHAR>();
					if (c < L'a' || c > L'z')
					{
						LOG_ERROR("Wide char out of range: 0x%04X", (UINT32)c);
						passed = false;
						break;
					}
				}
			}

			if (passed)
				LOG_INFO("  PASSED: GetChar produces lowercase a-z");
			else
			{
				LOG_ERROR("  FAILED: GetChar produces lowercase a-z");
				allPassed = false;
			}
		}

		// --- GetString ---
		{
			Prng prng(555);
			CHAR buffer[16];
			BOOL passed = true;

			UINT32 len = prng.GetString<CHAR>(Span<CHAR>(buffer, 11));
			if (len != 10)
			{
				LOG_ERROR("String length: expected 10, got %u", len);
				passed = false;
			}

			if (passed && buffer[10] != '\0')
			{
				LOG_ERROR("String not null-terminated at position 10");
				passed = false;
			}

			if (passed)
			{
				for (UINT32 i = 0; i < len; i++)
				{
					if (buffer[i] < 'a' || buffer[i] > 'z')
					{
						LOG_ERROR("String char[%u] out of range: 0x%02X", i, (UINT32)(UINT8)buffer[i]);
						passed = false;
						break;
					}
				}
			}

			// Empty string
			if (passed)
			{
				CHAR empty[4];
				UINT32 emptyLen = prng.GetString<CHAR>(Span<CHAR>(empty, 1));
				if (emptyLen != 0 || empty[0] != '\0')
				{
					LOG_ERROR("Empty string: expected len=0 and null terminator");
					passed = false;
				}
			}

			if (passed)
				LOG_INFO("  PASSED: GetString fills and null-terminates");
			else
			{
				LOG_ERROR("  FAILED: GetString fills and null-terminates");
				allPassed = false;
			}
		}

		return allPassed;
	}
};

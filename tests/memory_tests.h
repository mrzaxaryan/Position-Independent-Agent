#pragma once

#include "lib/runtime.h"
#include "tests.h"

class MemoryTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running Memory Tests...");

		RunTest(allPassed, &TestCopySuite, "Memory copy suite");
		RunTest(allPassed, &TestSetZeroSuite, "Memory set/zero suite");
		RunTest(allPassed, &TestCompareSuite, "Memory compare suite");

		if (allPassed)
			LOG_INFO("All Memory tests passed!");
		else
			LOG_ERROR("Some Memory tests failed!");

		return allPassed;
	}

private:
	static BOOL TestCopySuite()
	{
		BOOL allPassed = true;

		// --- Copy basic ---
		{
			auto src = "Hello, World!";
			CHAR dest[16];

			Memory::Copy(dest, (const CHAR *)src, 14); // Include null terminator

			// Verify each byte was copied correctly
			BOOL passed = true;
			for (INT32 i = 0; i < 14; i++)
			{
				if (dest[i] != ((const CHAR *)src)[i])
				{
					LOG_ERROR("  FAILED: Copy basic - mismatch at index %d: got 0x%02X, expected 0x%02X", i, (UINT32)(UINT8)dest[i], (UINT32)(UINT8)((const CHAR *)src)[i]);
					passed = false;
					break;
				}
			}
			if (passed)
				LOG_INFO("  PASSED: Copy basic");
			else
				allPassed = false;
		}

		// --- Copy non-overlapping ---
		{
			UINT8 buffer[32];

			// Fill with pattern
			for (INT32 i = 0; i < 32; i++)
			{
				buffer[i] = (UINT8)i;
			}

			// Copy to different location (non-overlapping)
			UINT8 dest[16];
			Memory::Copy(dest, buffer, 16);

			BOOL passed = true;
			for (INT32 i = 0; i < 16; i++)
			{
				if (dest[i] != (UINT8)i)
				{
					LOG_ERROR("  FAILED: Copy non-overlapping - mismatch at index %d: got %u, expected %u", i, (UINT32)dest[i], (UINT32)i);
					passed = false;
					break;
				}
			}
			if (passed)
				LOG_INFO("  PASSED: Copy non-overlapping");
			else
				allPassed = false;
		}

		// --- Zero size operations ---
		{
			UINT8 src[8];
			src[0] = 1;
			src[1] = 2;
			src[2] = 3;
			src[3] = 4;
			src[4] = 5;
			src[5] = 6;
			src[6] = 7;
			src[7] = 8;

			UINT8 dest[8];
			Memory::Zero(dest, 8);

			// Copy zero bytes should not change dest
			Memory::Copy(dest, src, 0);
			BOOL passed = true;
			for (INT32 i = 0; i < 8; i++)
			{
				if (dest[i] != 0)
				{
					LOG_ERROR("  FAILED: Zero size - copy modified dest at index %d: got 0x%02X", i, (UINT32)dest[i]);
					passed = false;
					break;
				}
			}

			if (passed)
			{
				// Compare zero bytes should return 0 (equal)
				INT32 cmp = Memory::Compare(src, dest, 0);
				if (cmp != 0)
				{
					LOG_ERROR("  FAILED: Zero size - compare failed: expected 0, got %d", cmp);
					passed = false;
				}
			}

			if (passed)
				LOG_INFO("  PASSED: Zero size operations");
			else
				allPassed = false;
		}

		return allPassed;
	}

	static BOOL TestSetZeroSuite()
	{
		BOOL allPassed = true;

		// --- Zero ---
		{
			UINT8 buffer[16];

			// Fill with non-zero values
			for (INT32 i = 0; i < 16; i++)
			{
				buffer[i] = 0xFF;
			}

			// Zero the buffer
			Memory::Zero(buffer, 16);

			// Verify all bytes are zero
			BOOL passed = true;
			for (INT32 i = 0; i < 16; i++)
			{
				if (buffer[i] != 0)
				{
					LOG_ERROR("  FAILED: Zero - failed at index %d: got 0x%02X", i, (UINT32)buffer[i]);
					passed = false;
					break;
				}
			}
			if (passed)
				LOG_INFO("  PASSED: Zero");
			else
				allPassed = false;
		}

		// --- Set ---
		{
			UINT8 buffer[16];

			// Set all bytes to a specific value
			Memory::Set(buffer, 0xAB, 16);

			// Verify all bytes have the expected value
			BOOL passed = true;
			for (INT32 i = 0; i < 16; i++)
			{
				if (buffer[i] != 0xAB)
				{
					LOG_ERROR("  FAILED: Set - Set(0xAB) failed at index %d: got 0x%02X", i, (UINT32)buffer[i]);
					passed = false;
					break;
				}
			}

			if (passed)
			{
				// Test setting to different value
				Memory::Set(buffer, 0x42, 8);
				for (INT32 i = 0; i < 8; i++)
				{
					if (buffer[i] != 0x42)
					{
						LOG_ERROR("  FAILED: Set - Set(0x42) failed at index %d: got 0x%02X", i, (UINT32)buffer[i]);
						passed = false;
						break;
					}
				}
			}

			if (passed)
			{
				// Verify remaining bytes unchanged
				for (INT32 i = 8; i < 16; i++)
				{
					if (buffer[i] != 0xAB)
					{
						LOG_ERROR("  FAILED: Set - remaining byte changed at index %d: got 0x%02X, expected 0xAB", i, (UINT32)buffer[i]);
						passed = false;
						break;
					}
				}
			}

			if (passed)
				LOG_INFO("  PASSED: Set");
			else
				allPassed = false;
		}

		return allPassed;
	}

	static BOOL TestCompareSuite()
	{
		BOOL allPassed = true;

		// --- Compare equal ---
		{
			auto str1 = "Hello";
			auto str2 = "Hello";

			INT32 result = Memory::Compare((const CHAR *)str1, (const CHAR *)str2, 5);
			if (result != 0)
			{
				LOG_ERROR("  FAILED: Compare equal - expected 0, got %d", result);
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Compare equal");
			}
		}

		// --- Compare less than ---
		{
			auto str1 = "Apple";
			auto str2 = "Banana";

			INT32 result = Memory::Compare((const CHAR *)str1, (const CHAR *)str2, 5);
			if (result >= 0)
			{
				LOG_ERROR("  FAILED: Compare less than - expected < 0, got %d", result);
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Compare less than");
			}
		}

		// --- Compare greater than ---
		{
			auto str1 = "Zebra";
			auto str2 = "Apple";

			INT32 result = Memory::Compare((const CHAR *)str1, (const CHAR *)str2, 5);
			if (result <= 0)
			{
				LOG_ERROR("  FAILED: Compare greater than - expected > 0, got %d", result);
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Compare greater than");
			}
		}

		return allPassed;
	}
};

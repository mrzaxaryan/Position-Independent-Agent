#pragma once

#include "lib/runtime.h"
#include "tests.h"

class Djb2Tests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running DJB2 Hash Tests...");

		if (!TestDjb2Suite())
			allPassed = false;

		if (allPassed)
			LOG_INFO("All DJB2 tests passed!");
		else
			LOG_ERROR("Some DJB2 tests failed!");

		return allPassed;
	}

private:
	static BOOL TestDjb2Suite()
	{
		BOOL allPassed = true;

		// Basic hash consistency: same input should always produce same hash
		{
			auto testStr = "hello";
			UINT64 hash1 = Djb2::Hash((const CHAR *)testStr);
			UINT64 hash2 = Djb2::Hash((const CHAR *)testStr);
			if (hash1 != hash2)
			{
				LOG_ERROR("  FAILED: Hash consistency: same input produced different hashes");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Basic hash consistency");
			}
		}

		// Case insensitivity
		{
			auto lower = "hello";
			auto upper = "HELLO";
			auto mixed = "HeLLo";

			UINT64 hashLower = Djb2::Hash((const CHAR *)lower);
			UINT64 hashUpper = Djb2::Hash((const CHAR *)upper);
			UINT64 hashMixed = Djb2::Hash((const CHAR *)mixed);

			if (hashLower != hashUpper)
			{
				LOG_ERROR("  FAILED: Case insensitivity: lower != upper");
				allPassed = false;
			}
			else if (hashLower != hashMixed)
			{
				LOG_ERROR("  FAILED: Case insensitivity: lower != mixed");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Case insensitivity");
			}
		}

		// Empty string should return the seed value (non-zero)
		{
			auto empty = "";
			UINT64 hash = Djb2::Hash((const CHAR *)empty);
			if (hash == 0)
			{
				LOG_ERROR("  FAILED: Empty string hash is zero, expected non-zero seed");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Empty string");
			}
		}

		// Compile-time hash should match runtime hash
		{
			constexpr UINT64 compileTimeHash = Djb2::HashCompileTime("test");
			auto runtimeStr = "test";
			UINT64 runtimeHash = Djb2::Hash((const CHAR *)runtimeStr);
			if (compileTimeHash != runtimeHash)
			{
				LOG_ERROR("  FAILED: Compile-time hash does not match runtime hash");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Compile-time matches runtime");
			}
		}

		// Different strings should produce different hashes
		{
			auto str1 = "hello";
			auto str2 = "world";
			auto str3 = "test";

			UINT64 hash1 = Djb2::Hash((const CHAR *)str1);
			UINT64 hash2 = Djb2::Hash((const CHAR *)str2);
			UINT64 hash3 = Djb2::Hash((const CHAR *)str3);

			if (hash1 == hash2)
			{
				LOG_ERROR("  FAILED: Hash collision: 'hello' == 'world'");
				allPassed = false;
			}
			else if (hash2 == hash3)
			{
				LOG_ERROR("  FAILED: Hash collision: 'world' == 'test'");
				allPassed = false;
			}
			else if (hash1 == hash3)
			{
				LOG_ERROR("  FAILED: Hash collision: 'hello' == 'test'");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Different strings produce different hashes");
			}
		}

		// Wide character support
		{
			auto wideStr = L"hello";

			UINT64 hash1 = Djb2::Hash((const WCHAR *)wideStr);
			UINT64 hash2 = Djb2::Hash((const WCHAR *)wideStr);

			if (hash1 != hash2)
			{
				LOG_ERROR("  FAILED: Wide char hash consistency");
				allPassed = false;
			}
			else
			{
				auto wideLower = L"hello";
				auto wideUpper = L"HELLO";

				UINT64 hashLower = Djb2::Hash((const WCHAR *)wideLower);
				UINT64 hashUpper = Djb2::Hash((const WCHAR *)wideUpper);

				if (hashLower != hashUpper)
				{
					LOG_ERROR("  FAILED: Wide char case insensitivity: lower != upper");
					allPassed = false;
				}
				else
				{
					LOG_INFO("  PASSED: Wide character support");
				}
			}
		}

		return allPassed;
	}
};

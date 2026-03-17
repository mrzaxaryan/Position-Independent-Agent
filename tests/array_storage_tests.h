#pragma once

#include "lib/runtime.h"
#include "tests.h"

class ArrayStorageTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running ArrayStorage Tests...");

		RunTest(allPassed, &TestWideCharArrayStorage, "Wide char array storage");
		RunTest(allPassed, &TestUInt32ArrayStorage, "UINT32 array storage");
		RunTest(allPassed, &TestUInt64ArrayStorage, "UINT64 array storage");
		RunTest(allPassed, &TestArrayIndexing, "Array indexing");
		RunTest(allPassed, &TestPointerConversionAndCopy, "Pointer conversion and copy");
		RunTest(allPassed, &TestCompileTimeConstants, "Compile-time constants");

		if (allPassed)
			LOG_INFO("All ArrayStorage tests passed!");
		else
			LOG_ERROR("Some ArrayStorage tests failed!");

		return allPassed;
	}

private:
	static BOOL TestWideCharArrayStorage()
	{
		const WCHAR storage[] = L"Test";

		// Verify size
		constexpr USIZE count = 5; // "Test" + null terminator
		if (sizeof(storage) / sizeof(WCHAR) != count)
		{
			LOG_ERROR("Wide char Count: expected 5, got %u", (UINT32)(sizeof(storage) / sizeof(WCHAR)));
			return false;
		}

		// Verify data integrity using literals
		if (storage[0] != L'T' || storage[1] != L'e' || storage[2] != L's' ||
			storage[3] != L't' || storage[4] != L'\0')
		{
			LOG_ERROR("Wide char data integrity mismatch");
			return false;
		}

		return true;
	}

	static BOOL TestUInt32ArrayStorage()
	{
		const UINT32 storage[] = {1, 2, 3, 4};

		// Verify size
		if (sizeof(storage) / sizeof(UINT32) != 4)
		{
			LOG_ERROR("UINT32 Count: expected 4, got %u", (UINT32)(sizeof(storage) / sizeof(UINT32)));
			return false;
		}

		// Print values to console
		LOG_INFO("    UINT32 values:");
		for (USIZE i = 0; i < 4; i++)
		{
			LOG_INFO("      %u", storage[i]);
		}

		// Verify data integrity using literals
		if (storage[0] != 1 || storage[1] != 2 || storage[2] != 3 || storage[3] != 4)
		{
			LOG_ERROR("UINT32 data integrity mismatch");
			return false;
		}

		return true;
	}

	static BOOL TestUInt64ArrayStorage()
	{
		const UINT64 storage[] = {
			UINT64(0x123456789ABCDEF0ULL),
			UINT64(0xFEDCBA9876543210ULL),
			UINT64(0x0011223344556677ULL)};

		// Verify size
		if (sizeof(storage) / sizeof(UINT64) != 3)
		{
			LOG_ERROR("UINT64 Count: expected 3, got %u", (UINT32)(sizeof(storage) / sizeof(UINT64)));
			return false;
		}

		// Verify data integrity using literals
		if (storage[0] != UINT64(0x123456789ABCDEF0ULL) ||
			storage[1] != UINT64(0xFEDCBA9876543210ULL) ||
			storage[2] != UINT64(0x0011223344556677ULL))
		{
			LOG_ERROR("UINT64 data integrity mismatch");
			return false;
		}

		return true;
	}

	static BOOL TestArrayIndexing()
	{
		const UINT32 storage[] = {100, 200, 300, 400, 500};

		// Test indexing operator
		if (storage[0] != 100 || storage[1] != 200 || storage[2] != 300 ||
			storage[3] != 400 || storage[4] != 500)
		{
			LOG_ERROR("Array indexing mismatch");
			return false;
		}

		return true;
	}

	static BOOL TestPointerConversionAndCopy()
	{
		const UINT32 storage[] = {0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC};

		// Test pointer conversion and Memory::Copy
		UINT32 dest[3];
		Memory::Copy(dest, storage, 3 * sizeof(UINT32));

		if (dest[0] != 0xAAAAAAAA || dest[1] != 0xBBBBBBBB || dest[2] != 0xCCCCCCCC)
		{
			LOG_ERROR("Pointer conversion copy mismatch: got 0x%08X 0x%08X 0x%08X", dest[0], dest[1], dest[2]);
			return false;
		}

		return true;
	}

	static BOOL TestCompileTimeConstants()
	{
		const CHAR storage[] = "CompileTime";

		// Verify size properties
		constexpr USIZE expectedCount = 12; // "CompileTime" + null terminator
		constexpr USIZE expectedSizeBytes = 12;

		// Verify runtime behavior matches compile-time expectations
		if (sizeof(storage) != expectedCount)
		{
			LOG_ERROR("Runtime Count: expected 12, got %u", (UINT32)sizeof(storage));
			return false;
		}
		if (sizeof(storage) != expectedSizeBytes)
		{
			LOG_ERROR("Runtime SizeBytes: expected 12, got %u", (UINT32)sizeof(storage));
			return false;
		}

		return true;
	}
};

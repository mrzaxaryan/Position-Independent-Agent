#pragma once

#include "lib/runtime.h"
#include "tests.h"

class StringTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running String Tests...");

		RunTest(allPassed, &TestLengthSuite, "String length suite");
		RunTest(allPassed, &TestToLowerCaseSuite, "ToLowerCase suite");
		RunTest(allPassed, &TestUTF16Suite, "UTF16 suite");

		if (allPassed)
			LOG_INFO("All String tests passed!");
		else
			LOG_ERROR("Some String tests failed!");

		return allPassed;
	}

private:
	static BOOL TestLengthSuite()
	{
		BOOL allPassed = true;

		// --- Narrow string length ---
		{
			auto str1 = "Hello";
			auto str2 = "Hello, World!";
			auto str3 = "A";

			BOOL passed = true;
			if (StringUtils::Length((const CHAR *)str1) != 5)
			{
				LOG_ERROR("  FAILED: Narrow length - Length('Hello') != 5");
				passed = false;
			}
			if (passed && StringUtils::Length((const CHAR *)str2) != 13)
			{
				LOG_ERROR("  FAILED: Narrow length - Length('Hello, World!') != 13");
				passed = false;
			}
			if (passed && StringUtils::Length((const CHAR *)str3) != 1)
			{
				LOG_ERROR("  FAILED: Narrow length - Length('A') != 1");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Narrow string length");
			else
				allPassed = false;
		}

		// --- Wide string length ---
		{
			auto str1 = L"Hello";
			auto str2 = L"Hello, World!";
			auto str3 = L"A";

			BOOL passed = true;
			if (StringUtils::Length((const WCHAR *)str1) != 5)
			{
				LOG_ERROR("  FAILED: Wide length - Length('Hello') != 5");
				passed = false;
			}
			if (passed && StringUtils::Length((const WCHAR *)str2) != 13)
			{
				LOG_ERROR("  FAILED: Wide length - Length('Hello, World!') != 13");
				passed = false;
			}
			if (passed && StringUtils::Length((const WCHAR *)str3) != 1)
			{
				LOG_ERROR("  FAILED: Wide length - Length('A') != 1");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Wide string length");
			else
				allPassed = false;
		}

		// --- Empty string length ---
		{
			auto emptyNarrow = "";
			auto emptyWide = L"";

			BOOL passed = true;
			if (StringUtils::Length((const CHAR *)emptyNarrow) != 0)
			{
				LOG_ERROR("  FAILED: Empty length - Narrow empty string length != 0");
				passed = false;
			}
			if (passed && StringUtils::Length((const WCHAR *)emptyWide) != 0)
			{
				LOG_ERROR("  FAILED: Empty length - Wide empty string length != 0");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Empty string length");
			else
				allPassed = false;
		}

		return allPassed;
	}

	static BOOL TestToLowerCaseSuite()
	{
		BOOL allPassed = true;

		// --- ASCII ---
		{
			BOOL passed = true;

			// Test uppercase letters A-Z
			if (StringUtils::ToLowerCase<CHAR>('A') != 'a')
			{
				LOG_ERROR("  FAILED: ToLowerCase ASCII - ToLowerCase('A') != 'a'");
				passed = false;
			}
			if (passed && StringUtils::ToLowerCase<CHAR>('M') != 'm')
			{
				LOG_ERROR("  FAILED: ToLowerCase ASCII - ToLowerCase('M') != 'm'");
				passed = false;
			}
			if (passed && StringUtils::ToLowerCase<CHAR>('Z') != 'z')
			{
				LOG_ERROR("  FAILED: ToLowerCase ASCII - ToLowerCase('Z') != 'z'");
				passed = false;
			}

			// Test wide char version
			if (passed && StringUtils::ToLowerCase<WCHAR>(L'A') != L'a')
			{
				LOG_ERROR("  FAILED: ToLowerCase ASCII - Wide ToLowerCase('A') != 'a'");
				passed = false;
			}
			if (passed && StringUtils::ToLowerCase<WCHAR>(L'Z') != L'z')
			{
				LOG_ERROR("  FAILED: ToLowerCase ASCII - Wide ToLowerCase('Z') != 'z'");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: ToLowerCase ASCII");
			else
				allPassed = false;
		}

		// --- Preserves non-uppercase ---
		{
			BOOL passed = true;

			// Already lowercase should stay lowercase
			if (StringUtils::ToLowerCase<CHAR>('a') != 'a')
			{
				LOG_ERROR("  FAILED: ToLowerCase preserves - ToLowerCase('a') != 'a'");
				passed = false;
			}
			if (passed && StringUtils::ToLowerCase<CHAR>('z') != 'z')
			{
				LOG_ERROR("  FAILED: ToLowerCase preserves - ToLowerCase('z') != 'z'");
				passed = false;
			}

			// Numbers should be unchanged
			if (passed && StringUtils::ToLowerCase<CHAR>('0') != '0')
			{
				LOG_ERROR("  FAILED: ToLowerCase preserves - ToLowerCase('0') != '0'");
				passed = false;
			}
			if (passed && StringUtils::ToLowerCase<CHAR>('9') != '9')
			{
				LOG_ERROR("  FAILED: ToLowerCase preserves - ToLowerCase('9') != '9'");
				passed = false;
			}

			// Special characters should be unchanged
			if (passed && StringUtils::ToLowerCase<CHAR>('!') != '!')
			{
				LOG_ERROR("  FAILED: ToLowerCase preserves - ToLowerCase('!') != '!'");
				passed = false;
			}
			if (passed && StringUtils::ToLowerCase<CHAR>('@') != '@')
			{
				LOG_ERROR("  FAILED: ToLowerCase preserves - ToLowerCase('@') != '@'");
				passed = false;
			}
			if (passed && StringUtils::ToLowerCase<CHAR>(' ') != ' ')
			{
				LOG_ERROR("  FAILED: ToLowerCase preserves - ToLowerCase(' ') != ' '");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: ToLowerCase preserves non-uppercase");
			else
				allPassed = false;
		}

		return allPassed;
	}

	static BOOL TestUTF16Suite()
	{
		BOOL allPassed = true;

		// --- UTF16 basic ASCII ---
		{
			auto wide = L"Hello";
			CHAR utf8[16];

			USIZE wideLen = StringUtils::Length((const WCHAR *)wide);
			USIZE len = UTF16::ToUTF8(Span<const WCHAR>((const WCHAR *)wide, wideLen), Span<CHAR>(utf8, sizeof(utf8) - 1));
			utf8[len] = '\0';

			BOOL passed = true;
			// Should be 5 characters
			if (len != 5)
			{
				LOG_ERROR("  FAILED: UTF16 basic ASCII - length: expected 5, got %u", (UINT32)len);
				passed = false;
			}

			if (passed)
			{
				// Verify content
				auto expected_hello = "Hello";
				if (Memory::Compare(utf8, (const CHAR *)expected_hello, 6) != 0)
				{
					LOG_ERROR("  FAILED: UTF16 basic ASCII - content mismatch");
					passed = false;
				}
			}

			if (passed)
				LOG_INFO("  PASSED: UTF16 basic ASCII");
			else
				allPassed = false;
		}

		// --- UTF16 empty ---
		{
			auto wide = L"";
			CHAR utf8[16];

			USIZE wideLen = StringUtils::Length((const WCHAR *)wide);
			USIZE len = UTF16::ToUTF8(Span<const WCHAR>((const WCHAR *)wide, wideLen), Span<CHAR>(utf8, sizeof(utf8) - 1));
			utf8[len] = '\0';

			BOOL passed = true;
			// Should be 0 characters
			if (len != 0)
			{
				LOG_ERROR("  FAILED: UTF16 empty - length: expected 0, got %u", (UINT32)len);
				passed = false;
			}

			if (passed)
			{
				// Should be null terminated
				auto expected_empty = "";
				if (Memory::Compare(utf8, (const CHAR *)expected_empty, 1) != 0)
				{
					LOG_ERROR("  FAILED: UTF16 empty - content mismatch");
					passed = false;
				}
			}

			if (passed)
				LOG_INFO("  PASSED: UTF16 empty");
			else
				allPassed = false;
		}

		return allPassed;
	}
};

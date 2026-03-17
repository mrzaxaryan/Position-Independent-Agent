#pragma once

#include "lib/runtime.h"
#include "tests.h"

// =============================================================================
// Compile-Time (constexpr) Verification
// =============================================================================

// Default UUID is nil (all zeros)
static_assert(UUID().GetMostSignificantBits() == 0);
static_assert(UUID().GetLeastSignificantBits() == 0);

// UUID from known bytes: 550e8400-e29b-41d4-a716-446655440000
constexpr UINT8 kKnownUuidBytes[16] = {
	0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
	0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00
};
static_assert(UUID(Span<const UINT8, 16>(kKnownUuidBytes)).GetMostSignificantBits() == 0x550e8400e29b41d4ULL);
static_assert(UUID(Span<const UINT8, 16>(kKnownUuidBytes)).GetLeastSignificantBits() == 0xa716446655440000ULL);

// =============================================================================
// UuidTests — Runtime Validation
// =============================================================================

class UuidTests
{
private:
	static BOOL TestConstructionSuite()
	{
		BOOL allPassed = true;

		// --- Nil UUID ---
		{
			UUID nil;
			BOOL passed = nil.GetMostSignificantBits() == 0 && nil.GetLeastSignificantBits() == 0;

			if (passed)
				LOG_INFO("  PASSED: nil UUID default constructor");
			else
			{
				LOG_ERROR("  FAILED: nil UUID default constructor");
				allPassed = false;
			}
		}

		// --- From bytes ---
		{
			const UINT8 uuidBytes[] = {
				0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
				0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00
			};
			Span<const UINT8, 16> uuidSpan(uuidBytes);
			UUID uuid(uuidSpan);
			BOOL passed = uuid.GetMostSignificantBits() == 0x550e8400e29b41d4ULL &&
			              uuid.GetLeastSignificantBits() == 0xa716446655440000ULL;

			if (passed)
				LOG_INFO("  PASSED: UUID construction from bytes");
			else
			{
				LOG_ERROR("  FAILED: UUID construction from bytes");
				allPassed = false;
			}
		}

		// --- FromString lowercase ---
		{
			auto str = "550e8400-e29b-41d4-a716-446655440000";
			auto result = UUID::FromString((PCCHAR)str);
			BOOL passed = false;
			if (result)
			{
				auto &uuid = result.Value();
				passed = uuid.GetMostSignificantBits() == 0x550e8400e29b41d4ULL &&
				         uuid.GetLeastSignificantBits() == 0xa716446655440000ULL;
			}

			if (passed)
				LOG_INFO("  PASSED: FromString lowercase hex");
			else
			{
				LOG_ERROR("  FAILED: FromString lowercase hex");
				allPassed = false;
			}
		}

		// --- FromString uppercase ---
		{
			auto str = "550E8400-E29B-41D4-A716-446655440000";
			auto result = UUID::FromString((PCCHAR)str);
			BOOL passed = false;
			if (result)
			{
				auto &uuid = result.Value();
				passed = uuid.GetMostSignificantBits() == 0x550e8400e29b41d4ULL &&
				         uuid.GetLeastSignificantBits() == 0xa716446655440000ULL;
			}

			if (passed)
				LOG_INFO("  PASSED: FromString uppercase hex");
			else
			{
				LOG_ERROR("  FAILED: FromString uppercase hex");
				allPassed = false;
			}
		}

		// --- FromString no dashes ---
		{
			// 32 hex digits with no dashes — still valid (dashes are ignored)
			auto str = "550e8400e29b41d4a716446655440000";
			auto result = UUID::FromString((PCCHAR)str);
			BOOL passed = false;
			if (result)
			{
				auto &uuid = result.Value();
				passed = uuid.GetMostSignificantBits() == 0x550e8400e29b41d4ULL &&
				         uuid.GetLeastSignificantBits() == 0xa716446655440000ULL;
			}

			if (passed)
				LOG_INFO("  PASSED: FromString accepts no-dash format");
			else
			{
				LOG_ERROR("  FAILED: FromString accepts no-dash format");
				allPassed = false;
			}
		}

		// --- ToString round-trip ---
		{
			auto str = "550e8400-e29b-41d4-a716-446655440000";
			auto parseResult = UUID::FromString((PCCHAR)str);
			BOOL passed = false;
			if (parseResult)
			{
				CHAR buffer[37];
				auto toStrResult = parseResult.Value().ToString(Span<CHAR>(buffer));
				if (toStrResult)
				{
					auto expected = "550e8400-e29b-41d4-a716-446655440000";
					if (StringUtils::Equals(buffer, (PCCHAR)expected))
						passed = true;
					else
						LOG_ERROR("ToString mismatch: got '%s', expected '%s'", buffer, (PCCHAR)expected);
				}
			}

			if (passed)
				LOG_INFO("  PASSED: ToString round-trip");
			else
			{
				LOG_ERROR("  FAILED: ToString round-trip");
				allPassed = false;
			}
		}

		// --- Nil UUID ToString ---
		{
			UUID nil;
			CHAR buffer[37];
			auto result = nil.ToString(Span<CHAR>(buffer));
			BOOL passed = false;
			if (result)
			{
				auto expected = "00000000-0000-0000-0000-000000000000";
				if (StringUtils::Equals(buffer, (PCCHAR)expected))
					passed = true;
				else
					LOG_ERROR("Nil UUID ToString mismatch: got '%s'", buffer);
			}

			if (passed)
				LOG_INFO("  PASSED: nil UUID ToString");
			else
			{
				LOG_ERROR("  FAILED: nil UUID ToString");
				allPassed = false;
			}
		}

		return allPassed;
	}

	static BOOL TestErrorHandlingSuite()
	{
		BOOL allPassed = true;

		// --- Buffer too small ---
		{
			UUID nil;
			CHAR buffer[36]; // one byte too small (need 37)
			auto result = nil.ToString(Span<CHAR>(buffer));
			BOOL passed = !result;

			if (passed)
				LOG_INFO("  PASSED: ToString rejects buffer < 37 bytes");
			else
			{
				LOG_ERROR("  FAILED: ToString rejects buffer < 37 bytes");
				allPassed = false;
			}
		}

		// --- Invalid char ---
		{
			// 'g' is not a valid hex character
			auto str = "550e8400-e29b-41d4-a716-44665544000g";
			auto result = UUID::FromString((PCCHAR)str);
			BOOL passed = !result;

			if (passed)
				LOG_INFO("  PASSED: FromString rejects invalid hex char");
			else
			{
				LOG_ERROR("  FAILED: FromString rejects invalid hex char");
				allPassed = false;
			}
		}

		// --- Too short ---
		{
			// 31 hex digits instead of 32
			auto str = "550e8400-e29b-41d4-a716-44665544000";
			auto result = UUID::FromString((PCCHAR)str);
			BOOL passed = !result;

			if (passed)
				LOG_INFO("  PASSED: FromString rejects too few hex digits");
			else
			{
				LOG_ERROR("  FAILED: FromString rejects too few hex digits");
				allPassed = false;
			}
		}

		// --- Too long ---
		{
			// 33 hex digits instead of 32
			auto str = "550e8400-e29b-41d4-a716-4466554400001";
			auto result = UUID::FromString((PCCHAR)str);
			BOOL passed = !result;

			if (passed)
				LOG_INFO("  PASSED: FromString rejects too many hex digits");
			else
			{
				LOG_ERROR("  FAILED: FromString rejects too many hex digits");
				allPassed = false;
			}
		}

		return allPassed;
	}

public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running UUID Tests...");

		RunTest(allPassed, &TestConstructionSuite, "Construction suite");
		RunTest(allPassed, &TestErrorHandlingSuite, "Error handling suite");

		if (allPassed)
			LOG_INFO("All UUID tests passed!");
		else
			LOG_ERROR("Some UUID tests failed!");

		return allPassed;
	}
};

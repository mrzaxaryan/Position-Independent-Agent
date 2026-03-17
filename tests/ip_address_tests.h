#pragma once

#include "lib/runtime.h"
#include "tests.h"

// =============================================================================
// Compile-Time (constexpr) Verification
// =============================================================================
// These static_assert tests verify that IPAddress operations are fully evaluated
// at compile time, producing no .rdata or data section entries.

// Default constructor produces Invalid
static_assert(!IPAddress().IsValid());
static_assert(!IPAddress().IsIPv4());
static_assert(!IPAddress().IsIPv6());
static_assert(IPAddress().GetVersion() == IPVersion::Invalid);

// FromIPv4 factory
static_assert(IPAddress::FromIPv4(0x0100007F).IsIPv4());
static_assert(IPAddress::FromIPv4(0x0100007F).IsValid());
static_assert(!IPAddress::FromIPv4(0x0100007F).IsIPv6());
static_assert(IPAddress::FromIPv4(0x0100007F).ToIPv4() == 0x0100007F);

// FromIPv6 factory
constexpr UINT8 kTestIPv6[16] = {0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
static_assert(IPAddress::FromIPv6(kTestIPv6).IsIPv6());
static_assert(IPAddress::FromIPv6(kTestIPv6).IsValid());
static_assert(!IPAddress::FromIPv6(kTestIPv6).IsIPv4());
static_assert(IPAddress::FromIPv6(kTestIPv6).ToIPv4() == 0xFFFFFFFF);
static_assert(IPAddress::FromIPv6(kTestIPv6).ToIPv6() != nullptr);

// Invalid factory
static_assert(!IPAddress::Invalid().IsValid());
static_assert(IPAddress::Invalid().GetVersion() == IPVersion::Invalid);

// LocalHost IPv4
static_assert(IPAddress::LocalHost().IsIPv4());
static_assert(IPAddress::LocalHost().ToIPv4() == 0x0100007F);

// LocalHost IPv6
static_assert(IPAddress::LocalHost(true).IsIPv6());
static_assert(IPAddress::LocalHost(true).IsValid());

// Equality operator
static_assert(IPAddress::FromIPv4(0x01010101) == IPAddress::FromIPv4(0x01010101));
static_assert(!(IPAddress::FromIPv4(0x01010101) == IPAddress::FromIPv4(0x08080808)));
static_assert(IPAddress::FromIPv4(0x01010101) != IPAddress::FromIPv4(0x08080808));
static_assert(IPAddress::Invalid() == IPAddress::Invalid());
static_assert(IPAddress::FromIPv6(kTestIPv6) == IPAddress::FromIPv6(kTestIPv6));
static_assert(IPAddress::FromIPv4(0x01010101) != IPAddress::Invalid());

// Copy constructor
static_assert(IPAddress(IPAddress::FromIPv4(0xC0A80001)).ToIPv4() == 0xC0A80001);
static_assert(IPAddress(IPAddress::FromIPv6(kTestIPv6)).IsIPv6());

// Assignment (verified through constexpr lambda)
static_assert([]() constexpr
{
	IPAddress a = IPAddress::FromIPv4(0x01010101);
	IPAddress b;
	b = a;
	return b.ToIPv4() == 0x01010101 && b.IsIPv4();
}());

static_assert([]() constexpr
{
	IPAddress a = IPAddress::FromIPv6(kTestIPv6);
	IPAddress b;
	b = a;
	return b.IsIPv6() && b == a;
}());

// =============================================================================
// IPAddress Tests - Runtime Validation
// =============================================================================

class IPAddressTests
{
private:
	static BOOL TestConstexprSuite()
	{
		BOOL allPassed = true;

		// --- IPv4 ---
		{
			IPAddress ip = IPAddress::FromIPv4(0x0100007F);
			BOOL passed = ip.IsIPv4() && ip.ToIPv4() == 0x0100007F && !ip.IsIPv6();

			if (passed)
				LOG_INFO("  PASSED: constexpr IPv4 construction");
			else
			{
				LOG_ERROR("  FAILED: constexpr IPv4 construction");
				allPassed = false;
			}
		}

		// --- IPv6 ---
		{
			const UINT8 addr[] = {0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
			IPAddress ip = IPAddress::FromIPv6(addr);
			BOOL passed = ip.IsIPv6() && !ip.IsIPv4() && ip.IsValid();

			if (passed)
				LOG_INFO("  PASSED: constexpr IPv6 construction");
			else
			{
				LOG_ERROR("  FAILED: constexpr IPv6 construction");
				allPassed = false;
			}
		}

		// --- LocalHost ---
		{
			IPAddress v4 = IPAddress::LocalHost();
			IPAddress v6 = IPAddress::LocalHost(true);
			BOOL passed = v4.IsIPv4() && v4.ToIPv4() == 0x0100007F && v6.IsIPv6();

			if (passed)
				LOG_INFO("  PASSED: constexpr LocalHost");
			else
			{
				LOG_ERROR("  FAILED: constexpr LocalHost");
				allPassed = false;
			}
		}

		// --- Equality ---
		{
			IPAddress a = IPAddress::FromIPv4(0x01010101);
			IPAddress b = IPAddress::FromIPv4(0x01010101);
			IPAddress c = IPAddress::FromIPv4(0x08080808);
			BOOL passed = (a == b) && !(a == c) && (a != c);

			if (passed)
				LOG_INFO("  PASSED: constexpr equality operators");
			else
			{
				LOG_ERROR("  FAILED: constexpr equality operators");
				allPassed = false;
			}
		}

		// --- Copy ---
		{
			IPAddress original = IPAddress::FromIPv4(0xC0A80001);
			IPAddress copy(original);
			BOOL passed = copy.ToIPv4() == original.ToIPv4() && copy == original;

			if (passed)
				LOG_INFO("  PASSED: constexpr copy constructor");
			else
			{
				LOG_ERROR("  FAILED: constexpr copy constructor");
				allPassed = false;
			}
		}

		// --- Invalid ---
		{
			IPAddress inv = IPAddress::Invalid();
			IPAddress def;
			BOOL passed = !inv.IsValid() && !inv.IsIPv4() && !inv.IsIPv6() && !def.IsValid() && (inv == def);

			if (passed)
				LOG_INFO("  PASSED: constexpr Invalid factory");
			else
			{
				LOG_ERROR("  FAILED: constexpr Invalid factory");
				allPassed = false;
			}
		}

		return allPassed;
	}

	static BOOL TestFromStringSuite()
	{
		BOOL allPassed = true;

		// --- FromString IPv4 ---
		{
			BOOL passed = true;
			auto ipStr = "192.168.1.1";
			auto result = IPAddress::FromString((PCCHAR)ipStr);
			if (!result)
				passed = false;

			if (passed && !result.Value().IsIPv4())
				passed = false;

			if (passed)
			{
				// Verify round-trip through ToString
				CHAR buffer[64];
				auto toStrResult = result.Value().ToString(Span<CHAR>(buffer));
				if (!toStrResult)
					passed = false;

				if (passed)
				{
					// Verify the string matches
					auto expected = "192.168.1.1";
					if (!StringUtils::Equals((PCCHAR)buffer, (PCCHAR)expected))
					{
						LOG_ERROR("ToString mismatch: got '%s', expected '%s'", buffer, (PCCHAR)expected);
						passed = false;
					}
				}
			}

			if (passed)
				LOG_INFO("  PASSED: FromString IPv4 + ToString round-trip");
			else
			{
				LOG_ERROR("  FAILED: FromString IPv4 + ToString round-trip");
				allPassed = false;
			}
		}

		// --- FromString IPv6 ---
		{
			auto ipStr = "2001:db8::1";
			auto result = IPAddress::FromString((PCCHAR)ipStr);
			BOOL passed = result && result.Value().IsIPv6() && !result.Value().IsIPv4();

			if (passed)
				LOG_INFO("  PASSED: FromString IPv6");
			else
			{
				LOG_ERROR("  FAILED: FromString IPv6");
				allPassed = false;
			}
		}

		// --- FromString invalid ---
		{
			BOOL passed = true;

			auto inv1 = "256.1.1.1";
			if (IPAddress::FromString((PCCHAR)inv1))
				passed = false;

			if (passed)
			{
				auto inv2 = "192.168.1";
				if (IPAddress::FromString((PCCHAR)inv2))
					passed = false;
			}

			if (passed)
			{
				auto inv3 = "abc.def.ghi.jkl";
				if (IPAddress::FromString((PCCHAR)inv3))
					passed = false;
			}

			if (passed)
			{
				if (IPAddress::FromString(nullptr))
					passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: FromString rejects invalid input");
			else
			{
				LOG_ERROR("  FAILED: FromString rejects invalid input");
				allPassed = false;
			}
		}

		// --- IPv6 equality ---
		{
			auto str1 = "2001:db8::1";
			auto str2 = "2001:db8::1";
			auto str3 = "2001:db8::2";

			auto r1 = IPAddress::FromString((PCCHAR)str1);
			auto r2 = IPAddress::FromString((PCCHAR)str2);
			auto r3 = IPAddress::FromString((PCCHAR)str3);

			BOOL passed = r1 && r2 && r3 && (r1.Value() == r2.Value()) && !(r1.Value() == r3.Value());

			if (passed)
				LOG_INFO("  PASSED: IPv6 equality comparison");
			else
			{
				LOG_ERROR("  FAILED: IPv6 equality comparison");
				allPassed = false;
			}
		}

		return allPassed;
	}

public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running IPAddress Tests...");

		RunTest(allPassed, &TestConstexprSuite, "Constexpr suite");
		RunTest(allPassed, &TestFromStringSuite, "FromString suite");

		if (allPassed)
			LOG_INFO("All IPAddress tests passed!");
		else
			LOG_ERROR("Some IPAddress tests failed!");

		return allPassed;
	}
};

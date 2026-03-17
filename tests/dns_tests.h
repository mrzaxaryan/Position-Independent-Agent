#pragma once

#include "lib/runtime.h"
#include "tests.h"

// =============================================================================
// DNS Tests - DNS Resolution via DoH (binary wireformat)
//
// Tests are consolidated to reduce the number of DNS queries.
// =============================================================================

class DnsTests
{
private:
	// Cloudflare resolver: localhost (A + AAAA) and dns.google (A)
	static BOOL TestCloudflareResolution()
	{
		LOG_INFO("Test: Cloudflare DNS Resolution");

		BOOL allPassed = true;

		// --- Localhost A record ---
		{
			auto result = DnsClient::CloudflareResolve("localhost", DnsRecordType::A);
			if (!result)
			{
				LOG_ERROR("Localhost A resolution failed (error: %e)", result.Error());
				return false;
			}
			auto& ip = result.Value();

			if (ip.ToIPv4() != 0x0100007F)
			{
				LOG_ERROR("Localhost resolution failed: expected 0x0100007F, got 0x%08X", ip.ToIPv4());
				return false;
			}
			LOG_INFO("  PASSED: Localhost A -> 127.0.0.1");
		}

		// --- Localhost AAAA record ---
		{
			auto result6 = DnsClient::CloudflareResolve("localhost", DnsRecordType::AAAA);
			if (!result6)
			{
				LOG_ERROR("Localhost AAAA resolution failed (error: %e)", result6.Error());
				return false;
			}
			auto& ip6 = result6.Value();

			UINT8 expectedIPv6[16]{};
			expectedIPv6[15] = 1;
			if (ip6.IsIPv6() == false || Memory::Compare(ip6.ToIPv6(), expectedIPv6, 16) != 0)
			{
				LOG_ERROR("Localhost IPv6 resolution failed: expected ::1, got different address");
				return false;
			}
			LOG_INFO("  PASSED: Localhost AAAA -> ::1");
		}

		// --- dns.google A record ---
		{
			auto result = DnsClient::CloudflareResolve("dns.google", DnsRecordType::A);
			if (!result)
			{
				LOG_ERROR("Cloudflare DNS resolution failed (error: %e)", result.Error());
				return false;
			}
			auto& ip = result.Value();

			if (ip.ToIPv4() != 0x08080808 && ip.ToIPv4() != 0x04040808)
			{
				LOG_ERROR("Unexpected IP for dns.google: 0x%08X", ip.ToIPv4());
				return false;
			}
			LOG_INFO("  PASSED: dns.google -> 0x%08X", ip.ToIPv4());
		}

		return allPassed;
	}

	// Google resolver: one.one.one.one (A)
	static BOOL TestGoogleResolution()
	{
		LOG_INFO("Test: Google DNS Resolution (one.one.one.one)");

		auto result = DnsClient::GoogleResolve("one.one.one.one", DnsRecordType::A);
		if (!result)
		{
			LOG_ERROR("Google DNS resolution failed (error: %e)", result.Error());
			return false;
		}
		auto& ip = result.Value();

		if (ip.ToIPv4() != 0x01010101 && ip.ToIPv4() != 0x01000001)
		{
			LOG_ERROR("Unexpected IP for one.one.one.one: 0x%08X", ip.ToIPv4());
			return false;
		}

		LOG_INFO("Google resolved one.one.one.one to 0x%08X", ip.ToIPv4());
		return true;
	}

	// Main Resolve function (tries IPv6 first, falls back to IPv4)
	static BOOL TestMainResolve()
	{
		LOG_INFO("Test: Main DNS Resolve Function");

		auto result = DnsClient::Resolve("example.com");
		if (!result)
		{
			LOG_ERROR("Main DNS resolution failed for example.com (error: %e)", result.Error());
			return false;
		}
		LOG_INFO("  PASSED: Resolve example.com");

		return true;
	}

public:
	// Run all DNS tests
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running DNS Tests...");
		LOG_INFO("  Testing DNS resolution via DoH (binary wireformat)");

		RunTest(allPassed, &TestCloudflareResolution, "Cloudflare DNS resolution (localhost + dns.google)");
		RunTest(allPassed, &TestGoogleResolution, "Google DNS resolution");
		RunTest(allPassed, &TestMainResolve, "Main DNS resolve function");

		if (allPassed)
			LOG_INFO("All DNS tests passed!");
		else
			LOG_ERROR("Some DNS tests failed!");

		return allPassed;
	}
};

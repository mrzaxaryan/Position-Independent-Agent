#pragma once

#include "lib/runtime.h"
#include "lib/crypto/sha2.h"
#include "tests.h"

class ShaTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running SHA Tests...");

		RunTest(allPassed, &TestSHA256Suite, "SHA-256 suite");
		RunTest(allPassed, &TestSHA384Suite, "SHA-384 suite");
		RunTest(allPassed, &TestHMACSuite, "HMAC suite");

		if (allPassed)
			LOG_INFO("All SHA tests passed!");
		else
			LOG_ERROR("Some SHA tests failed!");

		return allPassed;
	}

private:
	// SHA-256 Suite: empty string, "abc", long message, incremental update
	static BOOL TestSHA256Suite()
	{
		BOOL allPassed = true;

		// SHA-256 Test: Empty string
		// Expected: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
		{
			UINT8 digest[SHA256_DIGEST_SIZE];
			const UINT8 expected[] = {
				0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14,
				0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
				0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c,
				0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55};

			auto message = "";
			SHA256::Hash(Span<const UINT8>(reinterpret_cast<const UINT8 *>(static_cast<PCCHAR>(message)), 0), digest);
			if (CompareBytes(Span<const UINT8>(digest), Span<const UINT8>(expected, SHA256_DIGEST_SIZE)))
				LOG_INFO("  PASSED: SHA-256 empty string");
			else
			{
				LOG_ERROR("  FAILED: SHA-256 empty string digest mismatch");
				allPassed = false;
			}
		}

		// SHA-256 Test: "abc"
		// Expected: ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
		{
			UINT8 digest[SHA256_DIGEST_SIZE];
			const UINT8 expected[] = {
				0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
				0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
				0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
				0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad};

			auto message = "abc";
			SHA256::Hash(Span<const UINT8>(reinterpret_cast<const UINT8 *>(static_cast<PCCHAR>(message)), 3), digest);
			if (CompareBytes(Span<const UINT8>(digest), Span<const UINT8>(expected, SHA256_DIGEST_SIZE)))
				LOG_INFO("  PASSED: SHA-256 'abc'");
			else
			{
				LOG_ERROR("  FAILED: SHA-256 'abc' digest mismatch");
				allPassed = false;
			}
		}

		// SHA-256 Test: "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
		// Expected: 248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1
		{
			UINT8 digest[SHA256_DIGEST_SIZE];
			const UINT8 expected[] = {
				0x24, 0x8d, 0x6a, 0x61, 0xd2, 0x06, 0x38, 0xb8,
				0xe5, 0xc0, 0x26, 0x93, 0x0c, 0x3e, 0x60, 0x39,
				0xa3, 0x3c, 0xe4, 0x59, 0x64, 0xff, 0x21, 0x67,
				0xf6, 0xec, 0xed, 0xd4, 0x19, 0xdb, 0x06, 0xc1};

			auto message = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
			SHA256::Hash(Span<const UINT8>(reinterpret_cast<const UINT8 *>(static_cast<PCCHAR>(message)), 56), digest);
			if (CompareBytes(Span<const UINT8>(digest), Span<const UINT8>(expected, SHA256_DIGEST_SIZE)))
				LOG_INFO("  PASSED: SHA-256 long message");
			else
			{
				LOG_ERROR("  FAILED: SHA-256 long message digest mismatch");
				allPassed = false;
			}
		}

		// SHA-256 Test: Incremental update (split "abc" into "ab" + "c")
		{
			UINT8 digest[SHA256_DIGEST_SIZE];
			const UINT8 expected[] = {
				0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
				0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
				0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
				0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad};

			SHA256 ctx;
			auto msg1 = "ab";
			auto msg2 = "c";
			ctx.Update(Span<const UINT8>(reinterpret_cast<const UINT8 *>(static_cast<PCCHAR>(msg1)), 2));
			ctx.Update(Span<const UINT8>(reinterpret_cast<const UINT8 *>(static_cast<PCCHAR>(msg2)), 1));
			ctx.Final(digest);

			if (CompareBytes(Span<const UINT8>(digest), Span<const UINT8>(expected, SHA256_DIGEST_SIZE)))
				LOG_INFO("  PASSED: SHA-256 incremental update");
			else
			{
				LOG_ERROR("  FAILED: SHA-256 incremental digest mismatch");
				allPassed = false;
			}
		}

		return allPassed;
	}

	// SHA-384 Suite: empty string, "abc", long message, incremental update
	static BOOL TestSHA384Suite()
	{
		BOOL allPassed = true;

		// SHA-384 Test: Empty string
		// Expected: 38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b
		{
			UINT8 digest[SHA384_DIGEST_SIZE];
			const UINT8 expected[] = {
				0x38, 0xb0, 0x60, 0xa7, 0x51, 0xac, 0x96, 0x38,
				0x4c, 0xd9, 0x32, 0x7e, 0xb1, 0xb1, 0xe3, 0x6a,
				0x21, 0xfd, 0xb7, 0x11, 0x14, 0xbe, 0x07, 0x43,
				0x4c, 0x0c, 0xc7, 0xbf, 0x63, 0xf6, 0xe1, 0xda,
				0x27, 0x4e, 0xde, 0xbf, 0xe7, 0x6f, 0x65, 0xfb,
				0xd5, 0x1a, 0xd2, 0xf1, 0x48, 0x98, 0xb9, 0x5b};

			auto message = "";
			SHA384::Hash(Span<const UINT8>(reinterpret_cast<const UINT8 *>(static_cast<PCCHAR>(message)), 0), digest);
			if (CompareBytes(Span<const UINT8>(digest), Span<const UINT8>(expected, SHA384_DIGEST_SIZE)))
				LOG_INFO("  PASSED: SHA-384 empty string");
			else
			{
				LOG_ERROR("  FAILED: SHA-384 empty string digest mismatch");
				allPassed = false;
			}
		}

		// SHA-384 Test: "abc"
		// Expected: cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7
		{
			UINT8 digest[SHA384_DIGEST_SIZE];
			const UINT8 expected[] = {
				0xcb, 0x00, 0x75, 0x3f, 0x45, 0xa3, 0x5e, 0x8b,
				0xb5, 0xa0, 0x3d, 0x69, 0x9a, 0xc6, 0x50, 0x07,
				0x27, 0x2c, 0x32, 0xab, 0x0e, 0xde, 0xd1, 0x63,
				0x1a, 0x8b, 0x60, 0x5a, 0x43, 0xff, 0x5b, 0xed,
				0x80, 0x86, 0x07, 0x2b, 0xa1, 0xe7, 0xcc, 0x23,
				0x58, 0xba, 0xec, 0xa1, 0x34, 0xc8, 0x25, 0xa7};

			auto message = "abc";
			SHA384::Hash(Span<const UINT8>(reinterpret_cast<const UINT8 *>(static_cast<PCCHAR>(message)), 3), digest);
			if (CompareBytes(Span<const UINT8>(digest), Span<const UINT8>(expected, SHA384_DIGEST_SIZE)))
				LOG_INFO("  PASSED: SHA-384 'abc'");
			else
			{
				LOG_ERROR("  FAILED: SHA-384 'abc' digest mismatch");
				allPassed = false;
			}
		}

		// SHA-384 Test: "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"
		// Expected: 09330c33f71147e83d192fc782cd1b4753111b173b3b05d22fa08086e3b0f712fcc7c71a557e2db966c3e9fa91746039
		{
			UINT8 digest[SHA384_DIGEST_SIZE];
			const UINT8 expected[] = {
				0x09, 0x33, 0x0c, 0x33, 0xf7, 0x11, 0x47, 0xe8,
				0x3d, 0x19, 0x2f, 0xc7, 0x82, 0xcd, 0x1b, 0x47,
				0x53, 0x11, 0x1b, 0x17, 0x3b, 0x3b, 0x05, 0xd2,
				0x2f, 0xa0, 0x80, 0x86, 0xe3, 0xb0, 0xf7, 0x12,
				0xfc, 0xc7, 0xc7, 0x1a, 0x55, 0x7e, 0x2d, 0xb9,
				0x66, 0xc3, 0xe9, 0xfa, 0x91, 0x74, 0x60, 0x39};

			auto message = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
			SHA384::Hash(Span<const UINT8>(reinterpret_cast<const UINT8 *>(static_cast<PCCHAR>(message)), 112), digest);
			if (CompareBytes(Span<const UINT8>(digest), Span<const UINT8>(expected, SHA384_DIGEST_SIZE)))
				LOG_INFO("  PASSED: SHA-384 long message");
			else
			{
				LOG_ERROR("  FAILED: SHA-384 long message digest mismatch");
				allPassed = false;
			}
		}

		// SHA-384 Test: Incremental update (split "abc" into "ab" + "c")
		{
			UINT8 digest[SHA384_DIGEST_SIZE];
			const UINT8 expected[] = {
				0xcb, 0x00, 0x75, 0x3f, 0x45, 0xa3, 0x5e, 0x8b,
				0xb5, 0xa0, 0x3d, 0x69, 0x9a, 0xc6, 0x50, 0x07,
				0x27, 0x2c, 0x32, 0xab, 0x0e, 0xde, 0xd1, 0x63,
				0x1a, 0x8b, 0x60, 0x5a, 0x43, 0xff, 0x5b, 0xed,
				0x80, 0x86, 0x07, 0x2b, 0xa1, 0xe7, 0xcc, 0x23,
				0x58, 0xba, 0xec, 0xa1, 0x34, 0xc8, 0x25, 0xa7};

			SHA384 ctx;
			auto msg1 = "ab";
			auto msg2 = "c";
			ctx.Update(Span<const UINT8>(reinterpret_cast<const UINT8 *>(static_cast<PCCHAR>(msg1)), 2));
			ctx.Update(Span<const UINT8>(reinterpret_cast<const UINT8 *>(static_cast<PCCHAR>(msg2)), 1));
			ctx.Final(digest);

			if (CompareBytes(Span<const UINT8>(digest), Span<const UINT8>(expected, SHA384_DIGEST_SIZE)))
				LOG_INFO("  PASSED: SHA-384 incremental update");
			else
			{
				LOG_ERROR("  FAILED: SHA-384 incremental digest mismatch");
				allPassed = false;
			}
		}

		return allPassed;
	}

	// HMAC Suite: HMAC-SHA256 and HMAC-SHA384 (RFC 4231 Test Case 1)
	static BOOL TestHMACSuite()
	{
		BOOL allPassed = true;

		// HMAC-SHA256 Test (RFC 4231 Test Case 1)
		// Key: "Jefe" (0x4a656665)
		// Data: "what do ya want for nothing?"
		// Expected: 5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843
		{
			UINT8 mac[SHA256_DIGEST_SIZE];
			const UINT8 expected[] = {
				0x5b, 0xdc, 0xc1, 0x46, 0xbf, 0x60, 0x75, 0x4e,
				0x6a, 0x04, 0x24, 0x26, 0x08, 0x95, 0x75, 0xc7,
				0x5a, 0x00, 0x3f, 0x08, 0x9d, 0x27, 0x39, 0x83,
				0x9d, 0xec, 0x58, 0xb9, 0x64, 0xec, 0x38, 0x43};

			auto key = "Jefe";
			auto message = "what do ya want for nothing?";

			HMAC_SHA256::Compute(Span<const UCHAR>(reinterpret_cast<const UCHAR *>(static_cast<PCCHAR>(key)), 4), Span<const UCHAR>(reinterpret_cast<const UCHAR *>(static_cast<PCCHAR>(message)), 28), Span<UCHAR>(mac));

			if (CompareBytes(Span<const UINT8>(mac), Span<const UINT8>(expected, SHA256_DIGEST_SIZE)))
				LOG_INFO("  PASSED: HMAC-SHA256");
			else
			{
				LOG_ERROR("  FAILED: HMAC-SHA256 MAC mismatch");
				allPassed = false;
			}
		}

		// HMAC-SHA384 Test (RFC 4231 Test Case 1)
		// Key: "Jefe" (0x4a656665)
		// Data: "what do ya want for nothing?"
		// Expected: af45d2e376484031617f78d2b58a6b1b9c7ef464f5a01b47e42ec3736322445e8e2240ca5e69e2c78b3239ecfab21649
		{
			UINT8 mac[SHA384_DIGEST_SIZE];
			const UINT8 expected[] = {
				0xaf, 0x45, 0xd2, 0xe3, 0x76, 0x48, 0x40, 0x31,
				0x61, 0x7f, 0x78, 0xd2, 0xb5, 0x8a, 0x6b, 0x1b,
				0x9c, 0x7e, 0xf4, 0x64, 0xf5, 0xa0, 0x1b, 0x47,
				0xe4, 0x2e, 0xc3, 0x73, 0x63, 0x22, 0x44, 0x5e,
				0x8e, 0x22, 0x40, 0xca, 0x5e, 0x69, 0xe2, 0xc7,
				0x8b, 0x32, 0x39, 0xec, 0xfa, 0xb2, 0x16, 0x49};

			auto key = "Jefe";
			auto message = "what do ya want for nothing?";

			HMAC_SHA384::Compute(Span<const UCHAR>(reinterpret_cast<const UCHAR *>(static_cast<PCCHAR>(key)), 4), Span<const UCHAR>(reinterpret_cast<const UCHAR *>(static_cast<PCCHAR>(message)), 28), Span<UCHAR>(mac));

			if (CompareBytes(Span<const UINT8>(mac), Span<const UINT8>(expected, SHA384_DIGEST_SIZE)))
				LOG_INFO("  PASSED: HMAC-SHA384");
			else
			{
				LOG_ERROR("  FAILED: HMAC-SHA384 MAC mismatch");
				allPassed = false;
			}
		}

		return allPassed;
	}
};

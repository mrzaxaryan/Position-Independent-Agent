#pragma once

#include "lib/runtime.h"
#include "tests.h"

class Base64Tests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running Base64 Tests...");

		if (!TestEncodeSuite())
			allPassed = false;
		if (!TestDecodeSuite())
			allPassed = false;
		if (!TestSizeCalculation())
			allPassed = false;

		if (allPassed)
			LOG_INFO("All Base64 tests passed!");
		else
			LOG_ERROR("Some Base64 tests failed!");

		return allPassed;
	}

private:
	// Suite: All encoding tests
	static BOOL TestEncodeSuite()
	{
		BOOL allPassed = true;

		// Encode empty string
		// Expected: ""
		{
			CHAR output[10];
			auto input = "";
			Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(input), 0), Span<CHAR>(output));
			if (!StringUtils::Compare<CHAR>(output, static_cast<PCCHAR>("")))
			{
				LOG_ERROR("  FAILED: Encode empty: got '%s'", output);
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Base64 encode empty string");
			}
		}

		// Encode single character "f"
		// Expected: "Zg=="
		{
			CHAR output[10];
			auto input = "f";
			Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(input), 1), Span<CHAR>(output));
			if (!StringUtils::Compare<CHAR>(output, static_cast<PCCHAR>("Zg==")))
			{
				LOG_ERROR("  FAILED: Encode 'f': got '%s'", output);
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Base64 encode single character");
			}
		}

		// Encode two characters "fo"
		// Expected: "Zm8="
		{
			CHAR output[10];
			auto input = "fo";
			Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(input), 2), Span<CHAR>(output));
			if (!StringUtils::Compare<CHAR>(output, static_cast<PCCHAR>("Zm8=")))
			{
				LOG_ERROR("  FAILED: Encode 'fo': got '%s'", output);
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Base64 encode two characters");
			}
		}

		// Encode three characters "foo"
		// Expected: "Zm9v"
		{
			CHAR output[10];
			auto input = "foo";
			Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(input), 3), Span<CHAR>(output));
			if (!StringUtils::Compare<CHAR>(output, static_cast<PCCHAR>("Zm9v")))
			{
				LOG_ERROR("  FAILED: Encode 'foo': got '%s'", output);
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Base64 encode three characters");
			}
		}

		// Encode standard text "Hello, World!"
		// Expected: "SGVsbG8sIFdvcmxkIQ=="
		{
			CHAR output[30];
			auto input = "Hello, World!";
			Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(input), 13), Span<CHAR>(output));
			if (!StringUtils::Compare<CHAR>(output, static_cast<PCCHAR>("SGVsbG8sIFdvcmxkIQ==")))
			{
				LOG_ERROR("  FAILED: Encode 'Hello, World!': got '%s'", output);
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Base64 encode standard text");
			}
		}

		// Encode binary data
		// Input: {0x00, 0x01, 0x02, 0x03, 0x04, 0x05}
		// Expected: "AAECAwQF"
		{
			CHAR output[20];
			const UINT8 input[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
			Base64::Encode(Span<const CHAR>(reinterpret_cast<const char *>(input), 6), Span<CHAR>(output));
			if (!StringUtils::Compare<CHAR>(output, static_cast<PCCHAR>("AAECAwQF")))
			{
				LOG_ERROR("  FAILED: Encode binary: got '%s'", output);
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Base64 encode binary data");
			}
		}

		// Encode strings of various lengths to test all padding cases
		{
			CHAR output[20];
			BOOL paddingPassed = true;

			auto input1 = "f";
			Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(input1), 1), Span<CHAR>(output));
			if (!StringUtils::Compare<CHAR>(output, static_cast<PCCHAR>("Zg==")))
			{
				LOG_ERROR("  FAILED: Padding 'f': got '%s'", output);
				paddingPassed = false;
			}

			auto input2 = "fo";
			Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(input2), 2), Span<CHAR>(output));
			if (!StringUtils::Compare<CHAR>(output, static_cast<PCCHAR>("Zm8=")))
			{
				LOG_ERROR("  FAILED: Padding 'fo': got '%s'", output);
				paddingPassed = false;
			}

			auto input3 = "foo";
			Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(input3), 3), Span<CHAR>(output));
			if (!StringUtils::Compare<CHAR>(output, static_cast<PCCHAR>("Zm9v")))
			{
				LOG_ERROR("  FAILED: Padding 'foo': got '%s'", output);
				paddingPassed = false;
			}

			auto input4 = "foob";
			Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(input4), 4), Span<CHAR>(output));
			if (!StringUtils::Compare<CHAR>(output, static_cast<PCCHAR>("Zm9vYg==")))
			{
				LOG_ERROR("  FAILED: Padding 'foob': got '%s'", output);
				paddingPassed = false;
			}

			auto input5 = "fooba";
			Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(input5), 5), Span<CHAR>(output));
			if (!StringUtils::Compare<CHAR>(output, static_cast<PCCHAR>("Zm9vYmE=")))
			{
				LOG_ERROR("  FAILED: Padding 'fooba': got '%s'", output);
				paddingPassed = false;
			}

			auto input6 = "foobar";
			Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(input6), 6), Span<CHAR>(output));
			if (!StringUtils::Compare<CHAR>(output, static_cast<PCCHAR>("Zm9vYmFy")))
			{
				LOG_ERROR("  FAILED: Padding 'foobar': got '%s'", output);
				paddingPassed = false;
			}

			if (paddingPassed)
			{
				LOG_INFO("  PASSED: Base64 encode all padding cases");
			}
			else
			{
				allPassed = false;
			}
		}

		return allPassed;
	}

	// Suite: All decoding tests + round-trip
	static BOOL TestDecodeSuite()
	{
		BOOL allPassed = true;

		// Decode empty string
		{
			CHAR output[10];
			auto input = "";
			auto r = Base64::Decode(Span<const CHAR>(static_cast<PCCHAR>(input), 0), Span<CHAR>(output));
			if (!r)
			{
				LOG_ERROR("  FAILED: Decode empty (error: %e)", r.Error());
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Base64 decode empty string");
			}
		}

		// Decode "Zg==" to "f"
		{
			CHAR output[10];
			auto input = "Zg==";
			auto r = Base64::Decode(Span<const CHAR>(static_cast<PCCHAR>(input), 4), Span<CHAR>(output));
			if (!r)
			{
				LOG_ERROR("  FAILED: Decode 'Zg==' (error: %e)", r.Error());
				allPassed = false;
			}
			else if (Memory::Compare(output, static_cast<PCCHAR>("f"), 1) != 0)
			{
				LOG_ERROR("  FAILED: Decode 'Zg==' content mismatch");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Base64 decode single character");
			}
		}

		// Decode "Zm8=" to "fo"
		{
			CHAR output[10];
			auto input = "Zm8=";
			auto r = Base64::Decode(Span<const CHAR>(static_cast<PCCHAR>(input), 4), Span<CHAR>(output));
			if (!r)
			{
				LOG_ERROR("  FAILED: Decode 'Zm8=' (error: %e)", r.Error());
				allPassed = false;
			}
			else if (Memory::Compare(output, static_cast<PCCHAR>("fo"), 2) != 0)
			{
				LOG_ERROR("  FAILED: Decode 'Zm8=' content mismatch");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Base64 decode two characters");
			}
		}

		// Decode "Zm9v" to "foo"
		{
			CHAR output[10];
			auto input = "Zm9v";
			auto r = Base64::Decode(Span<const CHAR>(static_cast<PCCHAR>(input), 4), Span<CHAR>(output));
			if (!r)
			{
				LOG_ERROR("  FAILED: Decode 'Zm9v' (error: %e)", r.Error());
				allPassed = false;
			}
			else if (Memory::Compare(output, static_cast<PCCHAR>("foo"), 3) != 0)
			{
				LOG_ERROR("  FAILED: Decode 'Zm9v' content mismatch");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Base64 decode three characters");
			}
		}

		// Decode "SGVsbG8sIFdvcmxkIQ==" to "Hello, World!"
		{
			CHAR output[30];
			auto input = "SGVsbG8sIFdvcmxkIQ==";
			auto r = Base64::Decode(Span<const CHAR>(static_cast<PCCHAR>(input), 20), Span<CHAR>(output));
			if (!r)
			{
				LOG_ERROR("  FAILED: Decode standard text (error: %e)", r.Error());
				allPassed = false;
			}
			else if (Memory::Compare(output, static_cast<PCCHAR>("Hello, World!"), 13) != 0)
			{
				LOG_ERROR("  FAILED: Decode standard text content mismatch");
				allPassed = false;
			}
			else
			{
				LOG_INFO("  PASSED: Base64 decode standard text");
			}
		}

		// Decode "AAECAwQF" to binary data {0x00, 0x01, 0x02, 0x03, 0x04, 0x05}
		{
			CHAR output[20];
			auto input = "AAECAwQF";
			auto r = Base64::Decode(Span<const CHAR>(static_cast<PCCHAR>(input), 8), Span<CHAR>(output));
			if (!r)
			{
				LOG_ERROR("  FAILED: Decode binary data (error: %e)", r.Error());
				allPassed = false;
			}
			else
			{
				const UINT8 expected[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
				if (Memory::Compare(output, expected, 6) != 0)
				{
					LOG_ERROR("  FAILED: Decode binary data content mismatch");
					allPassed = false;
				}
				else
				{
					LOG_INFO("  PASSED: Base64 decode binary data");
				}
			}
		}

		// Round-trip encoding and decoding
		{
			CHAR encoded[100];
			CHAR decoded[100];
			BOOL roundTripPassed = true;

			// Test various strings
			auto test1 = "The quick brown fox jumps over the lazy dog";
			UINT32 len1 = 44;
			Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(test1), len1), Span<CHAR>(encoded));
			auto r1 = Base64::Decode(Span<const CHAR>(encoded, Base64::GetEncodeOutSize(len1) - 1), Span<CHAR>(decoded));
			if (!r1)
			{
				LOG_ERROR("  FAILED: Round-trip decode failed for test1 (error: %e)", r1.Error());
				roundTripPassed = false;
			}
			else if (Memory::Compare(decoded, static_cast<PCCHAR>(test1), len1) != 0)
			{
				LOG_ERROR("  FAILED: Round-trip content mismatch for test1");
				roundTripPassed = false;
			}

			auto test2 = "1234567890";
			UINT32 len2 = 10;
			Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(test2), len2), Span<CHAR>(encoded));
			auto r2 = Base64::Decode(Span<const CHAR>(encoded, Base64::GetEncodeOutSize(len2) - 1), Span<CHAR>(decoded));
			if (!r2)
			{
				LOG_ERROR("  FAILED: Round-trip decode failed for test2 (error: %e)", r2.Error());
				roundTripPassed = false;
			}
			else if (Memory::Compare(decoded, static_cast<PCCHAR>(test2), len2) != 0)
			{
				LOG_ERROR("  FAILED: Round-trip content mismatch for test2");
				roundTripPassed = false;
			}

			auto test3 = "!@#$%^&*()_+-=[]{}|;:,.<>?";
			UINT32 len3 = 26;
			Base64::Encode(Span<const CHAR>(static_cast<PCCHAR>(test3), len3), Span<CHAR>(encoded));
			auto r3 = Base64::Decode(Span<const CHAR>(encoded, Base64::GetEncodeOutSize(len3) - 1), Span<CHAR>(decoded));
			if (!r3)
			{
				LOG_ERROR("  FAILED: Round-trip decode failed for test3 (error: %e)", r3.Error());
				roundTripPassed = false;
			}
			else if (Memory::Compare(decoded, static_cast<PCCHAR>(test3), len3) != 0)
			{
				LOG_ERROR("  FAILED: Round-trip content mismatch for test3");
				roundTripPassed = false;
			}

			if (roundTripPassed)
			{
				LOG_INFO("  PASSED: Base64 round-trip test");
			}
			else
			{
				allPassed = false;
			}
		}

		return allPassed;
	}

	// Suite: Encode and decode output size calculations
	static BOOL TestSizeCalculation()
	{
		BOOL allPassed = true;

		// GetEncodeOutSize
		{
			BOOL encodeSizePassed = true;

			// Empty: 0 -> 1 (null terminator)
			if (Base64::GetEncodeOutSize(0) != 1)
			{
				LOG_ERROR("  FAILED: GetEncodeOutSize(0): expected 1, got %u", Base64::GetEncodeOutSize(0));
				encodeSizePassed = false;
			}

			// 1 byte: 1 -> 5 (4 chars + null)
			if (Base64::GetEncodeOutSize(1) != 5)
			{
				LOG_ERROR("  FAILED: GetEncodeOutSize(1): expected 5, got %u", Base64::GetEncodeOutSize(1));
				encodeSizePassed = false;
			}

			// 2 bytes: 2 -> 5 (4 chars + null)
			if (Base64::GetEncodeOutSize(2) != 5)
			{
				LOG_ERROR("  FAILED: GetEncodeOutSize(2): expected 5, got %u", Base64::GetEncodeOutSize(2));
				encodeSizePassed = false;
			}

			// 3 bytes: 3 -> 5 (4 chars + null)
			if (Base64::GetEncodeOutSize(3) != 5)
			{
				LOG_ERROR("  FAILED: GetEncodeOutSize(3): expected 5, got %u", Base64::GetEncodeOutSize(3));
				encodeSizePassed = false;
			}

			// 4 bytes: 4 -> 9 (8 chars + null)
			if (Base64::GetEncodeOutSize(4) != 9)
			{
				LOG_ERROR("  FAILED: GetEncodeOutSize(4): expected 9, got %u", Base64::GetEncodeOutSize(4));
				encodeSizePassed = false;
			}

			// 6 bytes: 6 -> 9 (8 chars + null)
			if (Base64::GetEncodeOutSize(6) != 9)
			{
				LOG_ERROR("  FAILED: GetEncodeOutSize(6): expected 9, got %u", Base64::GetEncodeOutSize(6));
				encodeSizePassed = false;
			}

			if (encodeSizePassed)
			{
				LOG_INFO("  PASSED: Base64 encode output size calculation");
			}
			else
			{
				allPassed = false;
			}
		}

		// GetDecodeOutSize
		{
			BOOL decodeSizePassed = true;

			// 0 chars: 0 -> 0
			if (Base64::GetDecodeOutSize(0) != 0)
			{
				LOG_ERROR("  FAILED: GetDecodeOutSize(0): expected 0, got %u", Base64::GetDecodeOutSize(0));
				decodeSizePassed = false;
			}

			// 4 chars: 4 -> 3
			if (Base64::GetDecodeOutSize(4) != 3)
			{
				LOG_ERROR("  FAILED: GetDecodeOutSize(4): expected 3, got %u", Base64::GetDecodeOutSize(4));
				decodeSizePassed = false;
			}

			// 8 chars: 8 -> 6
			if (Base64::GetDecodeOutSize(8) != 6)
			{
				LOG_ERROR("  FAILED: GetDecodeOutSize(8): expected 6, got %u", Base64::GetDecodeOutSize(8));
				decodeSizePassed = false;
			}

			// 12 chars: 12 -> 9
			if (Base64::GetDecodeOutSize(12) != 9)
			{
				LOG_ERROR("  FAILED: GetDecodeOutSize(12): expected 9, got %u", Base64::GetDecodeOutSize(12));
				decodeSizePassed = false;
			}

			if (decodeSizePassed)
			{
				LOG_INFO("  PASSED: Base64 decode output size calculation");
			}
			else
			{
				allPassed = false;
			}
		}

		return allPassed;
	}
};

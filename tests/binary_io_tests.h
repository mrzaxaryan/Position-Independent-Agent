#pragma once

#include "lib/runtime.h"
#include "tests.h"

class BinaryIOTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;
		LOG_INFO("Running BinaryIO Tests...");

		RunTest(allPassed, &TestReaderSuite, "BinaryReader suite");
		RunTest(allPassed, &TestWriterSuite, "BinaryWriter suite");
		RunTest(allPassed, &TestRoundTrip, "BinaryReader/Writer round-trip");

		if (allPassed)
			LOG_INFO("All BinaryIO tests passed!");
		else
			LOG_ERROR("Some BinaryIO tests failed!");

		return allPassed;
	}

private:
	static BOOL TestReaderSuite()
	{
		BOOL allPassed = true;

		// --- ReadU8 ---
		{
			UINT8 data[3];
			data[0] = 0x42;
			data[1] = 0xFF;
			data[2] = 0x00;
			BinaryReader reader{Span<const UINT8>(data)};

			BOOL passed = reader.Read<UINT8>() == 0x42 &&
			              reader.Read<UINT8>() == 0xFF &&
			              reader.Read<UINT8>() == 0x00 &&
			              reader.GetOffset() == 3;

			if (passed)
				LOG_INFO("  PASSED: BinaryReader Read<UINT8>");
			else
			{
				LOG_ERROR("  FAILED: BinaryReader Read<UINT8>");
				allPassed = false;
			}
		}

		// --- ReadU16BE ---
		{
			// Big-endian: 0x1234 stored as [0x12, 0x34]
			UINT8 data[4];
			data[0] = 0x12;
			data[1] = 0x34;
			data[2] = 0xAB;
			data[3] = 0xCD;
			BinaryReader reader{Span<const UINT8>(data)};

			UINT16 val1 = reader.ReadU16BE();
			UINT16 val2 = reader.ReadU16BE();
			BOOL passed = val1 == 0x1234 && val2 == 0xABCD && reader.GetOffset() == 4;

			if (passed)
				LOG_INFO("  PASSED: BinaryReader ReadU16BE");
			else
			{
				LOG_ERROR("  FAILED: BinaryReader ReadU16BE");
				allPassed = false;
			}
		}

		// --- ReadU24BE ---
		{
			// Big-endian 24-bit: 0x123456 stored as [0x12, 0x34, 0x56]
			UINT8 data[3];
			data[0] = 0x12;
			data[1] = 0x34;
			data[2] = 0x56;
			BinaryReader reader{Span<const UINT8>(data)};

			UINT32 val = reader.ReadU24BE();
			BOOL passed = val == 0x123456 && reader.GetOffset() == 3;

			if (passed)
				LOG_INFO("  PASSED: BinaryReader ReadU24BE");
			else
			{
				LOG_ERROR("  FAILED: BinaryReader ReadU24BE");
				allPassed = false;
			}
		}

		// --- ReadU32BE ---
		{
			// Big-endian: 0x12345678 stored as [0x12, 0x34, 0x56, 0x78]
			UINT8 data[4];
			data[0] = 0x12;
			data[1] = 0x34;
			data[2] = 0x56;
			data[3] = 0x78;
			BinaryReader reader{Span<const UINT8>(data)};

			UINT32 val = reader.ReadU32BE();
			BOOL passed = val == 0x12345678 && reader.GetOffset() == 4;

			if (passed)
				LOG_INFO("  PASSED: BinaryReader ReadU32BE");
			else
			{
				LOG_ERROR("  FAILED: BinaryReader ReadU32BE");
				allPassed = false;
			}
		}

		// --- Skip and Remaining ---
		{
			UINT8 data[10];
			Memory::Zero(data, sizeof(data));
			BinaryReader reader{Span<const UINT8>(data)};

			BOOL passed = true;
			if (reader.Remaining() != 10)
				passed = false;

			if (passed && !reader.Skip(3))
				passed = false;
			if (passed && (reader.Remaining() != 7 || reader.GetOffset() != 3))
				passed = false;

			if (passed && !reader.Skip(7))
				passed = false;
			if (passed && reader.Remaining() != 0)
				passed = false;

			// Should fail: can't skip past end
			if (passed && reader.Skip(1))
				passed = false;

			if (passed)
				LOG_INFO("  PASSED: BinaryReader Skip and Remaining");
			else
			{
				LOG_ERROR("  FAILED: BinaryReader Skip and Remaining");
				allPassed = false;
			}
		}

		// --- SetOffset ---
		{
			UINT8 data[4];
			data[0] = 0xAA;
			data[1] = 0xBB;
			data[2] = 0xCC;
			data[3] = 0xDD;
			BinaryReader reader{Span<const UINT8>(data)};

			BOOL passed = true;
			reader.Skip(2);
			if (reader.Read<UINT8>() != 0xCC)
				passed = false;

			// Jump back to offset 0
			if (passed && !reader.SetOffset(0))
				passed = false;
			if (passed && reader.Read<UINT8>() != 0xAA)
				passed = false;

			// Jump to offset 3
			if (passed && !reader.SetOffset(3))
				passed = false;
			if (passed && reader.Read<UINT8>() != 0xDD)
				passed = false;

			// Should fail: past end
			if (passed && reader.SetOffset(5))
				passed = false;

			if (passed)
				LOG_INFO("  PASSED: BinaryReader SetOffset");
			else
			{
				LOG_ERROR("  FAILED: BinaryReader SetOffset");
				allPassed = false;
			}
		}

		// --- Bounds check ---
		{
			UINT8 data[2];
			data[0] = 0x12;
			data[1] = 0x34;
			BinaryReader reader{Span<const UINT8>(data)};

			BOOL passed = true;

			// ReadU32BE should fail (only 2 bytes)
			UINT32 val32 = reader.ReadU32BE();
			if (val32 != 0)
				passed = false;
			// Offset should not advance on failed read
			if (passed && reader.GetOffset() != 0)
				passed = false;

			// ReadU16BE should succeed
			if (passed)
			{
				UINT16 val16 = reader.ReadU16BE();
				if (val16 != 0x1234)
					passed = false;
			}

			// Now at end, another read should fail
			if (passed && reader.Read<UINT8>() != 0)
				passed = false;

			if (passed)
				LOG_INFO("  PASSED: BinaryReader bounds checking");
			else
			{
				LOG_ERROR("  FAILED: BinaryReader bounds checking");
				allPassed = false;
			}
		}

		return allPassed;
	}

	static BOOL TestWriterSuite()
	{
		BOOL allPassed = true;

		// --- WriteU8 ---
		{
			UINT8 buf[4];
			Memory::Zero(buf, sizeof(buf));
			BinaryWriter writer{Span<UINT8>(buf)};

			writer.WriteU8(0xAA);
			writer.WriteU8(0xBB);

			BOOL passed = buf[0] == 0xAA && buf[1] == 0xBB && writer.GetOffset() == 2;

			if (passed)
				LOG_INFO("  PASSED: BinaryWriter WriteU8");
			else
			{
				LOG_ERROR("  FAILED: BinaryWriter WriteU8");
				allPassed = false;
			}
		}

		// --- WriteU16BE ---
		{
			UINT8 buf[4];
			Memory::Zero(buf, sizeof(buf));
			BinaryWriter writer{Span<UINT8>(buf)};

			writer.WriteU16BE(0x1234);

			// Should be stored as [0x12, 0x34]
			BOOL passed = buf[0] == 0x12 && buf[1] == 0x34 && writer.GetOffset() == 2;

			if (passed)
				LOG_INFO("  PASSED: BinaryWriter WriteU16BE");
			else
			{
				LOG_ERROR("  FAILED: BinaryWriter WriteU16BE");
				allPassed = false;
			}
		}

		// --- WriteU32BE ---
		{
			UINT8 buf[4];
			Memory::Zero(buf, sizeof(buf));
			BinaryWriter writer{Span<UINT8>(buf)};

			writer.WriteU32BE(0x12345678);

			BOOL passed = buf[0] == 0x12 && buf[1] == 0x34 && buf[2] == 0x56 && buf[3] == 0x78 &&
			              writer.GetOffset() == 4;

			if (passed)
				LOG_INFO("  PASSED: BinaryWriter WriteU32BE");
			else
			{
				LOG_ERROR("  FAILED: BinaryWriter WriteU32BE");
				allPassed = false;
			}
		}

		// --- Bounds check ---
		{
			UINT8 buf[2];
			Memory::Zero(buf, sizeof(buf));
			BinaryWriter writer{Span<UINT8>(buf)};

			BOOL passed = true;

			// WriteU32BE should fail (only 2 bytes capacity)
			if (writer.WriteU32BE(0x12345678) != nullptr)
				passed = false;
			if (passed && writer.GetOffset() != 0)
				passed = false;

			// WriteU16BE should succeed
			if (passed && writer.WriteU16BE(0xABCD) == nullptr)
				passed = false;

			// Now full, another write should fail
			if (passed && writer.WriteU8(0xFF) != nullptr)
				passed = false;

			if (passed)
				LOG_INFO("  PASSED: BinaryWriter bounds checking");
			else
			{
				LOG_ERROR("  FAILED: BinaryWriter bounds checking");
				allPassed = false;
			}
		}

		return allPassed;
	}

	static BOOL TestRoundTrip()
	{
		UINT8 buf[16];
		Memory::Zero(buf, sizeof(buf));

		// Write big-endian values
		BinaryWriter writer{Span<UINT8>(buf)};
		writer.WriteU8(0x42);
		writer.WriteU16BE(0x1234);
		writer.WriteU24BE(0xABCDEF);
		writer.WriteU32BE(0xDEADBEEF);

		// Read them back
		BinaryReader reader{Span<const UINT8>(buf, writer.GetOffset())};

		if (reader.Read<UINT8>() != 0x42)
			return false;
		if (reader.ReadU16BE() != 0x1234)
			return false;
		if (reader.ReadU24BE() != 0xABCDEF)
			return false;
		if (reader.ReadU32BE() != 0xDEADBEEF)
			return false;

		if (reader.Remaining() != 0)
			return false;

		return true;
	}
};

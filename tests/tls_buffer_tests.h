#pragma once

#include "lib/runtime.h"
#include "tests.h"

// TlsBuffer read-cursor semantics: the cursor is an offset into the live
// region, so GetBuffer() + GetReadPosition() stays valid after Consume()
// grows the dead prefix, and CheckSize must compact before it grows.
class TlsBufferTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running TlsBuffer Tests...");

		RunTest(allPassed, &TestReadCursorAfterConsumeSuite, "Read cursor after consume suite");
		RunTest(allPassed, &TestExhaustedReadSuite, "Exhausted read suite");
		RunTest(allPassed, &TestAppendAfterConsumeSuite, "Append after consume suite");
		RunTest(allPassed, &TestCompactInsteadOfGrowSuite, "Compact instead of grow suite");
		RunTest(allPassed, &TestGrowPreservesLiveDataSuite, "Grow preserves live data suite");

		if (allPassed)
			LOG_INFO("All TlsBuffer tests passed!");
		else
			LOG_ERROR("Some TlsBuffer tests failed!");

		return allPassed;
	}

private:
	// Read<>, Read(span), ReadU24BE and the cursor accessors must all address
	// the live region once Consume() has dead-prefixed the front.
	static BOOL TestReadCursorAfterConsumeSuite()
	{
		BOOL allPassed = true;

		{
			TlsBuffer b;
			b.Append(Span<const CHAR>("ABCDEFGH", 8));
			b.Consume(3);

			if (b.GetSize() != 5)
			{
				LOG_ERROR("Consume(3) of 8 bytes: GetSize() = %d, expected 5", b.GetSize());
				allPassed = false;
			}
			if (allPassed && b.GetBuffer()[0] != 'D')
			{
				LOG_ERROR("GetBuffer()[0] = %c after Consume(3), expected 'D'", b.GetBuffer()[0]);
				allPassed = false;
			}
			CHAR c = 'X';
			if (allPassed)
			{
				c = b.Read<CHAR>();
				if (c != 'D')
				{
					LOG_ERROR("Read<CHAR>() = %c after Consume(3), expected 'D'", c);
					allPassed = false;
				}
			}
			// Read<CHAR>() above advanced the cursor; GetBuffer() + cursor must
			// point past it
			if (allPassed && b.GetBuffer()[b.GetReadPosition()] != 'E')
			{
				LOG_ERROR("GetBuffer()[GetReadPosition()] = %c, expected 'E'", b.GetBuffer()[b.GetReadPosition()]);
				allPassed = false;
			}
			if (allPassed)
			{
				b.AdvanceReadPosition(2);
				c = b.Read<CHAR>();
				if (c != 'G')
				{
					LOG_ERROR("Read<CHAR>() after AdvanceReadPosition(2) = %c, expected 'G'", c);
					allPassed = false;
				}
			}
			if (allPassed)
			{
				b.ResetReadPos();
				if (b.GetReadPosition() != 0)
				{
					LOG_ERROR("ResetReadPos(): GetReadPosition() = %d, expected 0", b.GetReadPosition());
					allPassed = false;
				}
			}
		}

		{
			// ReadU24BE reads from the live start, not the raw allocation
			TlsBuffer b;
			CHAR data[4] = {(CHAR)0x11, (CHAR)0x22, (CHAR)0x33, (CHAR)0x44};
			b.Append(Span<const CHAR>(data, 4));
			b.Consume(1);
			UINT32 v = b.ReadU24BE();
			if (v != 0x223344)
			{
				LOG_ERROR("ReadU24BE() = 0x%x after Consume(1), expected 0x223344", v);
				allPassed = false;
			}
		}

		{
			// Read(span) clamps to the live bytes behind the cursor
			TlsBuffer b;
			b.Append(Span<const CHAR>("ABCDEF", 6));
			b.Consume(2); // live "CDEF"
			CHAR out[8] = {0};
			b.Read(Span<CHAR>(out, 8));
			if (out[0] != 'C' || out[3] != 'F' || out[4] != 0)
			{
				LOG_ERROR("Read(span 8) over 4 live bytes = '%s', expected \"CDEF\"", (PCHAR)out);
				allPassed = false;
			}
		}

		return allPassed;
	}

	// Consuming everything (or more than everything) resets the buffer; reads
	// on an empty buffer must not touch memory and must leave the cursor at
	// the (zero) live size.
	static BOOL TestExhaustedReadSuite()
	{
		BOOL allPassed = true;

		{
			TlsBuffer b;
			b.Append(Span<const CHAR>("AB", 2));
			b.Consume(2);
			if (b.GetSize() != 0 || b.GetReadPosition() != 0)
			{
				LOG_ERROR("Consume(2) of 2 bytes: size=%d readPos=%d, expected 0/0", b.GetSize(), b.GetReadPosition());
				allPassed = false;
			}
			CHAR c = b.Read<CHAR>();
			if (c != 0 || b.GetReadPosition() != 0)
			{
				LOG_ERROR("Read<CHAR>() on empty buffer = %d readPos=%d, expected 0/0", (INT32)c, b.GetReadPosition());
				allPassed = false;
			}
			UINT32 v = b.ReadU24BE();
			if (v != 0)
			{
				LOG_ERROR("ReadU24BE() on empty buffer = 0x%x, expected 0", v);
				allPassed = false;
			}
			CHAR out[4] = {1, 1, 1, 1};
			b.Read(Span<CHAR>(out, 4));
			if (out[0] != 1)
			{
				LOG_ERROR("Read(span) on empty buffer wrote %d, expected no write", (INT32)out[0]);
				allPassed = false;
			}
		}

		{
			// Over-consume clamps to the empty state instead of wrapping
			TlsBuffer b;
			b.Append(Span<const CHAR>("ABCD", 4));
			b.Consume(1000);
			if (b.GetSize() != 0 || b.GetReadPosition() != 0)
			{
				LOG_ERROR("Consume(1000) of 4 bytes: size=%d readPos=%d, expected 0/0", b.GetSize(), b.GetReadPosition());
				allPassed = false;
			}
		}

		return allPassed;
	}

	// Appends go to the end of the live region and keep byte order intact
	// after a consume.
	static BOOL TestAppendAfterConsumeSuite()
	{
		BOOL allPassed = true;

		TlsBuffer b;
		b.Append(Span<const CHAR>("ABCDEFGH", 8));
		b.Consume(3);
		b.Append(Span<const CHAR>("XY", 2));

		if (b.GetSize() != 7)
		{
			LOG_ERROR("GetSize() = %d after append, expected 7", b.GetSize());
			allPassed = false;
		}
		if (allPassed)
		{
			CHAR out[8] = {0};
			b.Read(Span<CHAR>(out, 7));
			if (out[0] != 'D' || out[4] != 'H' || out[5] != 'X' || out[6] != 'Y' || out[7] != 0)
			{
				LOG_ERROR("live bytes after append = '%s', expected \"DEFGHXY\"", (PCHAR)out);
				allPassed = false;
			}
		}
		return allPassed;
	}

	// When the dead prefix alone blocks the fit, CheckSize must reclaim it
	// (same allocation, live bytes moved down) instead of growing.
	static BOOL TestCompactInsteadOfGrowSuite()
	{
		BOOL allPassed = true;

		TlsBuffer b;
		CHAR hundred[100];
		for (INT32 i = 0; i < 100; i++)
			hundred[i] = (CHAR)i;
		if (b.Append(Span<const CHAR>(hundred, 100)) < 0)
		{
			LOG_ERROR("initial Append(100) failed");
			return false;
		}
		// First append grew capacity to max(400, 256) = 400.
		b.Consume(60); // live bytes are 60..99 at rawBase + 60

		PCHAR liveBefore = b.GetBuffer();
		PCHAR rawBase = liveBefore - 60;

		// 100 + 350 > 400 fails the total-size check, but 40 + 350 <= 400
		// fits after compaction — no growth allowed.
		CHAR more[350];
		for (INT32 i = 0; i < 350; i++)
			more[i] = (CHAR)(100 + i);
		INT32 at = b.Append(Span<const CHAR>(more, 350));
		if (at < 0)
		{
			LOG_ERROR("Append(350) after Consume(60) failed");
			return false;
		}

		if (b.GetBuffer() != rawBase)
		{
			LOG_ERROR("Append that fits after compaction grew or moved the allocation");
			allPassed = false;
		}
		if (allPassed && b.GetSize() != 390)
		{
			LOG_ERROR("GetSize() = %d, expected 390", b.GetSize());
			allPassed = false;
		}
		if (allPassed)
		{
			// Live region = hundred[60..99] then more[0..349]
			for (INT32 i = 0; i < 390; i++)
			{
				CHAR expected = (i < 40) ? (CHAR)(60 + i) : (CHAR)(100 + (i - 40));
				if (b.GetBuffer()[i] != expected)
				{
					LOG_ERROR("live byte %d = %d, expected %d", i, (INT32)b.GetBuffer()[i], (INT32)expected);
					allPassed = false;
					break;
				}
			}
		}
		if (allPassed && at != 40)
		{
			// Append returns the offset relative to the live start
			LOG_ERROR("Append returned %d, expected 40", at);
			allPassed = false;
		}
		return allPassed;
	}

	// A grow (live data + append exceeds capacity even compacted) carries only
	// the live bytes over, in order, and keeps the read cursor live-relative.
	static BOOL TestGrowPreservesLiveDataSuite()
	{
		BOOL allPassed = true;

		TlsBuffer b;
		CHAR hundred[100];
		for (INT32 i = 0; i < 100; i++)
			hundred[i] = (CHAR)('A' + i % 26);
		if (b.Append(Span<const CHAR>(hundred, 100)) < 0)
		{
			LOG_ERROR("initial Append(100) failed");
			return false;
		}
		b.Consume(90); // live bytes are the last 10

		// 10 + 300 > 400: must grow to (310)*4 = 1240
		CHAR more[300];
		for (INT32 i = 0; i < 300; i++)
			more[i] = (CHAR)('a' + i % 26);
		if (b.Append(Span<const CHAR>(more, 300)) < 0)
		{
			LOG_ERROR("Append(300) after Consume(90) failed");
			return false;
		}

		if (b.GetSize() != 310)
		{
			LOG_ERROR("GetSize() = %d after grow, expected 310", b.GetSize());
			allPassed = false;
		}
		if (allPassed)
		{
			CHAR out[310];
			b.Read(Span<CHAR>(out, 310));
			BOOL ok = true;
			for (INT32 i = 0; i < 10; i++)
				if (out[i] != hundred[90 + i])
					ok = false;
			for (INT32 i = 0; i < 300; i++)
				if (out[10 + i] != more[i])
					ok = false;
			if (!ok)
			{
				LOG_ERROR("live bytes not preserved across the grow");
				allPassed = false;
			}
		}
		if (allPassed && b.GetReadPosition() != 310)
		{
			LOG_ERROR("GetReadPosition() = %d after reading all live bytes, expected 310", b.GetReadPosition());
			allPassed = false;
		}
		return allPassed;
	}
};

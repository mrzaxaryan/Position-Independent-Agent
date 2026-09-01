#pragma once

#include "lib/runtime.h"
#include "tests.h"

class ByteQueueTests
{
public:
	static BOOL RunAll()
	{
		BOOL allPassed = true;

		LOG_INFO("Running ByteQueue Tests...");

		RunTest(allPassed, &TestBasicsSuite, "Basics suite");
		RunTest(allPassed, &TestConsumeSuite, "Consume and compaction suite");
		RunTest(allPassed, &TestGrowthSuite, "Growth suite");
		RunTest(allPassed, &TestMoveSemanticsSuite, "Move semantics suite");

		if (allPassed)
			LOG_INFO("All ByteQueue tests passed!");
		else
			LOG_ERROR("Some ByteQueue tests failed!");

		return allPassed;
	}

private:
	static BOOL TestBasicsSuite()
	{
		BOOL allPassed = true;

		// --- Default construction ---
		{
			ByteQueue q;
			BOOL passed = true;

			if (q.Data != nullptr)
			{
				LOG_ERROR("Default Data != nullptr");
				passed = false;
			}
			if (passed && (q.Capacity != 0 || q.Size != 0 || q.ReadPos != 0))
			{
				LOG_ERROR("Default Capacity/Size/ReadPos != 0");
				passed = false;
			}
			if (passed && !q.OwnsMemory)
			{
				LOG_ERROR("Default OwnsMemory != true");
				passed = false;
			}
			if (passed && q.GetSize() != 0)
			{
				LOG_ERROR("Default GetSize() != 0");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Default construction");
			else
			{
				LOG_ERROR("  FAILED: Default construction");
				allPassed = false;
			}
		}

		// --- Init and Append ---
		{
			ByteQueue q;
			BOOL passed = true;

			if (!q.Init(64))
			{
				LOG_ERROR("Init(64) returned false");
				passed = false;
			}
			if (passed && (q.Capacity != 64 || q.Size != 0 || q.ReadPos != 0))
			{
				LOG_ERROR("Init did not produce an empty queue");
				passed = false;
			}
			if (passed && q.Init(32))
			{
				LOG_ERROR("Init() on initialized queue returned true");
				passed = false;
			}

			const CHAR *text = "abcdef";
			if (passed && !q.Append(Span<const CHAR>(text, 6)))
			{
				LOG_ERROR("Append returned false");
				passed = false;
			}
			if (passed && q.GetSize() != 6)
			{
				LOG_ERROR("GetSize() != 6 after Append");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Init and Append");
			else
			{
				LOG_ERROR("  FAILED: Init and Append");
				allPassed = false;
			}
		}

		// --- Live region contiguity ---
		{
			// The whole point of the linear design: GetLiveBuffer() must be one
			// contiguous run of exactly the appended bytes, safe to hand to a
			// BinaryReader without copying.
			ByteQueue q;
			if (!q.Init(32)) return false;
			BOOL passed = true;

			const CHAR *text = "hello world";
			if (!q.Append(Span<const CHAR>(text, 11)))
				return false;

			Span<CHAR> live = q.GetLiveBuffer();
			if (live.Size() != 11)
			{
				LOG_ERROR("live.Size() = %u, expected 11", (UINT32)live.Size());
				passed = false;
			}
			if (passed && live.Data() != q.Data)
			{
				LOG_ERROR("live.Data() != Data before any consume");
				passed = false;
			}
			if (passed)
			{
				for (USIZE i = 0; i < 11; i++)
				{
					if (live.Data()[i] != text[i])
					{
						LOG_ERROR("live byte %u = %c, expected %c", (UINT32)i, live.Data()[i], text[i]);
						passed = false;
						break;
					}
				}
			}
			// After consuming, the live region must start at Data + ReadPos and stay contiguous
			if (passed)
			{
				q.Consume(6);
				Span<CHAR> rest = q.GetLiveBuffer();
				if (rest.Size() != 5)
				{
					LOG_ERROR("rest.Size() = %u, expected 5", (UINT32)rest.Size());
					passed = false;
				}
				if (passed && rest.Data() != q.Data + 6)
				{
					LOG_ERROR("rest.Data() != Data + ReadPos");
					passed = false;
				}
				if (passed && (rest.Data()[0] != 'w' || rest.Data()[4] != 'd'))
				{
					LOG_ERROR("Post-consume live bytes incorrect");
					passed = false;
				}
			}

			if (passed)
				LOG_INFO("  PASSED: Live region stays contiguous after consume");
			else
			{
				LOG_ERROR("  FAILED: Live region stays contiguous after consume");
				allPassed = false;
			}
		}

		// --- BinaryReader over the live region ---
		{
			// The adoption case: parse records straight out of the queue.
			ByteQueue q;
			if (!q.Init(16)) return false;
			BOOL passed = true;

			UINT8 record[5];
			record[0] = 0x16;
			record[1] = 0x03;
			record[2] = 0x01;
			record[3] = 0x00;
			record[4] = 0x04;
			if (!q.Append(Span<const CHAR>((const CHAR *)record, 5)))
				return false;

			{
				Span<CHAR> live = q.GetLiveBuffer();
				BinaryReader reader{Span<const UINT8>((const UINT8 *)live.Data(), live.Size())};
				UINT8 type = reader.Read<UINT8>();
				UINT16 version = reader.ReadU16BE();
				UINT16 length = reader.ReadU16BE();
				if (type != 0x16 || version != 0x0301 || length != 0x0004)
				{
					LOG_ERROR("Parsed values wrong: %02X %04X %04X", type, version, length);
					passed = false;
				}
				if (passed)
					q.Consume(reader.GetOffset());
			}

			if (passed && q.GetSize() != 0)
			{
				LOG_ERROR("GetSize() != 0 after consuming the record");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: BinaryReader over live region");
			else
			{
				LOG_ERROR("  FAILED: BinaryReader over live region");
				allPassed = false;
			}
		}

		// --- Wrap (non-owning) mode ---
		{
			CHAR backing[8];
			for (INT32 i = 0; i < 8; i++)
				backing[i] = (CHAR)('a' + i);

			ByteQueue q;
			BOOL passed = true;

			if (!q.Wrap(Span<CHAR>(backing, 8)))
			{
				LOG_ERROR("Wrap returned false");
				passed = false;
			}
			if (passed && (q.OwnsMemory || q.Data != backing))
			{
				LOG_ERROR("Wrap did not adopt the caller storage non-owningly");
				passed = false;
			}
			if (passed && q.GetSize() != 8)
			{
				LOG_ERROR("GetSize() != 8 after Wrap");
				passed = false;
			}
			// A wrapped queue cannot grow
			if (passed && q.Append(Span<const CHAR>("!", 1)))
			{
				LOG_ERROR("Append on non-owning queue returned true");
				passed = false;
			}
			if (passed)
			{
				q.Consume(3);
				if (q.GetSize() != 5 || q.GetLiveBuffer().Data() != backing + 3)
				{
					LOG_ERROR("Consume on wrapped queue wrong");
					passed = false;
				}
			}
			// The queue is read-only storage; appending within capacity is allowed
			// only for owning queues, so nothing else to check here.

			if (passed)
				LOG_INFO("  PASSED: Non-owning wrap mode");
			else
			{
				LOG_ERROR("  FAILED: Non-owning wrap mode");
				allPassed = false;
			}
		}

		return allPassed;
	}

	static BOOL TestConsumeSuite()
	{
		BOOL allPassed = true;

		// --- Consume is O(1) until the watermark ---
		{
			ByteQueue q;
			if (!q.Init(16)) return false;
			BOOL passed = true;

			const CHAR *text = "0123456789ABCDEFGHIJ"; // 20 bytes
			if (!q.Append(Span<const CHAR>(text, 20)))
				return false;
			// Init(16) doubles to fit 20 bytes: 16 -> 32
			if (q.Capacity != 32)
			{
				LOG_ERROR("Capacity = %u, expected 32", (UINT32)q.Capacity);
				passed = false;
			}

			// Consume 8: below the Capacity/2 = 16 watermark, no compaction yet
			q.Consume(8);
			if (passed && (q.ReadPos != 8 || q.Size != 20))
			{
				LOG_ERROR("Consume below watermark moved data (ReadPos=%u Size=%u)", (UINT32)q.ReadPos, (UINT32)q.Size);
				passed = false;
			}
			if (passed && q.GetSize() != 12)
			{
				LOG_ERROR("GetSize() = %u, expected 12", (UINT32)q.GetSize());
				passed = false;
			}

			// Consume 8 more: ReadPos 16 equals Capacity/2 exactly — the watermark
			// is "exceeds", so compaction is still deferred
			if (passed)
			{
				q.Consume(8);
				if (q.ReadPos != 16 || q.Size != 20)
				{
					LOG_ERROR("Compaction ran at exactly Capacity/2 (ReadPos=%u Size=%u)", (UINT32)q.ReadPos, (UINT32)q.Size);
					passed = false;
				}
			}

			// One more byte pushes ReadPos past the watermark, so compaction runs
			if (passed)
			{
				q.Consume(1);
				if (q.ReadPos != 0 || q.Size != 3)
				{
					LOG_ERROR("Compaction did not run past watermark (ReadPos=%u Size=%u)", (UINT32)q.ReadPos, (UINT32)q.Size);
					passed = false;
				}
			}
			// The surviving bytes are the tail of the original append
			if (passed)
			{
				Span<CHAR> live = q.GetLiveBuffer();
				if (live.Size() != 3 || live.Data() != q.Data)
				{
					LOG_ERROR("Live region wrong after compaction");
					passed = false;
				}
				if (passed && live.Data()[0] != 'H')
				{
					LOG_ERROR("Post-compaction first byte = %c, expected H", live.Data()[0]);
					passed = false;
				}
			}

			if (passed)
				LOG_INFO("  PASSED: Consume defers compaction to the watermark");
			else
			{
				LOG_ERROR("  FAILED: Consume defers compaction to the watermark");
				allPassed = false;
			}
		}

		// --- Consume everything resets the queue ---
		{
			ByteQueue q;
			if (!q.Init(16)) return false;
			const CHAR *text = "hello";
			if (!q.Append(Span<const CHAR>(text, 5))) return false;

			BOOL passed = true;
			q.Consume(100); // over-consume clamps
			if (q.GetSize() != 0 || q.Size != 0 || q.ReadPos != 0)
			{
				LOG_ERROR("Over-consume did not reset the queue");
				passed = false;
			}
			if (passed && q.Capacity != 16)
			{
				LOG_ERROR("Over-consume changed capacity");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Over-consume clamps and resets");
			else
			{
				LOG_ERROR("  FAILED: Over-consume clamps and resets");
				allPassed = false;
			}
		}

		// --- Explicit Compact ---
		{
			ByteQueue q;
			if (!q.Init(64)) return false;
			const CHAR *text = "abcdefgh";
			if (!q.Append(Span<const CHAR>(text, 8))) return false;

			BOOL passed = true;
			q.Consume(3); // below watermark, no auto-compaction
			if (q.ReadPos != 3)
			{
				LOG_ERROR("ReadPos = %u, expected 3", (UINT32)q.ReadPos);
				passed = false;
			}

			q.Compact();
			if (passed && (q.ReadPos != 0 || q.Size != 5))
			{
				LOG_ERROR("Compact did not reclaim the dead prefix");
				passed = false;
			}
			if (passed && q.GetLiveBuffer().Data()[0] != 'd')
			{
				LOG_ERROR("Compact lost the live suffix");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Explicit Compact");
			else
			{
				LOG_ERROR("  FAILED: Explicit Compact");
				allPassed = false;
			}
		}

		return allPassed;
	}

	static BOOL TestGrowthSuite()
	{
		BOOL allPassed = true;

		// --- Growth from empty uses the 256 minimum ---
		{
			ByteQueue q;
			BOOL passed = true;

			if (!q.CheckSize(10))
			{
				LOG_ERROR("CheckSize(10) on empty queue returned false");
				passed = false;
			}
			if (passed && q.Capacity != ByteQueueMinCapacity)
			{
				LOG_ERROR("Capacity = %u, expected %u", (UINT32)q.Capacity, (UINT32)ByteQueueMinCapacity);
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Empty growth floors at the 256-byte minimum");
			else
			{
				LOG_ERROR("  FAILED: Empty growth floors at the 256-byte minimum");
				allPassed = false;
			}
		}

		// --- Doubling growth ---
		{
			ByteQueue q;
			if (!q.Init(64)) return false;
			BOOL passed = true;

			if (!q.CheckSize(65))
			{
				LOG_ERROR("CheckSize(65) returned false");
				passed = false;
			}
			if (passed && q.Capacity != 128)
			{
				LOG_ERROR("Capacity = %u, expected 128", (UINT32)q.Capacity);
				passed = false;
			}
			if (passed && !q.CheckSize(129))
			{
				LOG_ERROR("CheckSize(129) returned false");
				passed = false;
			}
			if (passed && q.Capacity != 256)
			{
				LOG_ERROR("Capacity = %u, expected 256", (UINT32)q.Capacity);
				passed = false;
			}
			// Within capacity: no growth
			if (passed && !q.CheckSize(1))
			{
				LOG_ERROR("CheckSize(1) within capacity returned false");
				passed = false;
			}
			if (passed && q.Capacity != 256)
			{
				LOG_ERROR("Capacity changed on no-op CheckSize");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Capacity doubles until the request fits");
			else
			{
				LOG_ERROR("  FAILED: Capacity doubles until the request fits");
				allPassed = false;
			}
		}

		// --- Grow after consume drops dead bytes ---
		{
			ByteQueue q;
			if (!q.Init(16)) return false;
			BOOL passed = true;

			// 20 bytes: capacity grows 16 -> 32
			const CHAR *text = "0123456789ABCDEFGHIJ";
			if (!q.Append(Span<const CHAR>(text, 20)))
				return false;

			// Consume 19 of them; the single surviving byte must be the last one
			q.Consume(19);
			if (q.GetSize() != 1)
			{
				LOG_ERROR("GetSize() = %u, expected 1", (UINT32)q.GetSize());
				passed = false;
			}
			if (passed && q.GetLiveBuffer().Data()[0] != 'J')
			{
				LOG_ERROR("Surviving byte wrong after consume");
				passed = false;
			}

			// Now force a grow: dead prefix is dropped first, only 1 live byte is carried
			if (passed && !q.Append(Span<const CHAR>("KLMNOPQRSTUV", 12)))
			{
				LOG_ERROR("Append after consume returned false");
				passed = false;
			}
			if (passed && q.GetSize() != 13)
			{
				LOG_ERROR("GetSize() = %u, expected 13", (UINT32)q.GetSize());
				passed = false;
			}
			if (passed)
			{
				Span<CHAR> live = q.GetLiveBuffer();
				if (live.Data()[0] != 'J' || live.Data()[1] != 'K' || live.Data()[12] != 'V')
				{
					LOG_ERROR("Live bytes wrong after grow-after-consume");
					passed = false;
				}
			}

			if (passed)
				LOG_INFO("  PASSED: Grow after consume drops the dead prefix");
			else
			{
				LOG_ERROR("  FAILED: Grow after consume drops the dead prefix");
				allPassed = false;
			}
		}

		// --- Append preserves order across many growths ---
		{
			ByteQueue q;
			BOOL passed = true;

			CHAR chunk[1];
			for (INT32 i = 0; i < 600; i++)
			{
				chunk[0] = (CHAR)(i % 26 + 'A');
				if (!q.Append(Span<const CHAR>(chunk, 1)))
				{
					LOG_ERROR("Append failed at %d", i);
					passed = false;
					break;
				}
			}
			if (passed && q.GetSize() != 600)
			{
				LOG_ERROR("GetSize() = %u, expected 600", (UINT32)q.GetSize());
				passed = false;
			}
			if (passed)
			{
				Span<CHAR> live = q.GetLiveBuffer();
				for (USIZE i = 0; i < 600; i++)
				{
					CHAR expected = (CHAR)(i % 26 + 'A');
					if (live.Data()[i] != expected)
					{
						LOG_ERROR("byte %u = %c, expected %c", (UINT32)i, live.Data()[i], expected);
						passed = false;
						break;
					}
				}
			}

			if (passed)
				LOG_INFO("  PASSED: Byte order preserved across growths");
			else
			{
				LOG_ERROR("  FAILED: Byte order preserved across growths");
				allPassed = false;
			}
		}

		return allPassed;
	}

	static BOOL TestMoveSemanticsSuite()
	{
		BOOL allPassed = true;

		// --- Move construct ---
		{
			ByteQueue q;
			if (!q.Init(32)) return false;
			const CHAR *text = "move";
			if (!q.Append(Span<const CHAR>(text, 4))) return false;
			q.Consume(1);

			CHAR *origData = q.Data;
			ByteQueue q2((ByteQueue &&)q);

			BOOL passed = true;
			if (q.Data != nullptr || q.Capacity != 0 || q.Size != 0 || q.ReadPos != 0 || q.OwnsMemory)
			{
				LOG_ERROR("Source not zeroed after move construct");
				passed = false;
			}
			if (passed && (q2.Data != origData || q2.Capacity != 32 || q2.Size != 4 || q2.ReadPos != 1))
			{
				LOG_ERROR("Destination does not match original after move construct");
				passed = false;
			}
			if (passed && q2.GetSize() != 3)
			{
				LOG_ERROR("Destination GetSize() != 3");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Move construction transfers ownership");
			else
			{
				LOG_ERROR("  FAILED: Move construction transfers ownership");
				allPassed = false;
			}
		}

		// --- Move assign ---
		{
			ByteQueue q;
			if (!q.Init(32)) return false;
			const CHAR *text = "move";
			if (!q.Append(Span<const CHAR>(text, 4))) return false;

			CHAR *origData = q.Data;
			ByteQueue q2;
			if (!q2.Init(8)) return false;
			q2 = (ByteQueue &&)q;

			BOOL passed = true;
			if (q.Data != nullptr || q.Size != 0)
			{
				LOG_ERROR("Source not zeroed after move assign");
				passed = false;
			}
			if (passed && (q2.Data != origData || q2.Size != 4))
			{
				LOG_ERROR("Destination incorrect after move assign");
				passed = false;
			}

			if (passed)
				LOG_INFO("  PASSED: Move assignment frees destination and steals source");
			else
			{
				LOG_ERROR("  FAILED: Move assignment frees destination and steals source");
				allPassed = false;
			}
		}

		// --- Self-assign ---
		{
			ByteQueue q;
			if (!q.Init(16)) return false;
			const CHAR *text = "keep";
			if (!q.Append(Span<const CHAR>(text, 4))) return false;

			CHAR *origData = q.Data;
			q = (ByteQueue &&)q;

			BOOL passed = q.Data == origData && q.Size == 4 && q.GetLiveBuffer().Data()[0] == 'k';

			if (passed)
				LOG_INFO("  PASSED: Move self-assignment is safe");
			else
			{
				LOG_ERROR("Self move-assign corrupted data");
				LOG_ERROR("  FAILED: Move self-assignment is safe");
				allPassed = false;
			}
		}

		return allPassed;
	}
};

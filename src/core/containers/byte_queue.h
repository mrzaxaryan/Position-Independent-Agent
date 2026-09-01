/**
 * @file byte_queue.h
 * @brief Linear Byte Queue with Deferred Compaction
 *
 * @details Provides an append/consume byte queue over a single linear heap
 * buffer. Deliberately NOT a circular ring: a ring breaks the contiguity that
 * BinaryReader and TlsBuffer::Read depend on — callers wrap GetLiveBuffer()
 * in a Span or BinaryReader without copying, which only works when the live
 * bytes are adjacent in memory.
 *
 * Consume(n) is O(1) — it just advances the read cursor. The dead prefix it
 * leaves behind is reclaimed by Compact(), which Memory::Moves the live
 * suffix over the dead prefix. Compaction runs only when the dead prefix
 * exceeds half the capacity, keeping the amortized cost per byte O(1) while
 * avoiding a memmove on every consume.
 *
 * Key properties:
 * - RAII: destructor frees the backing array (when owned)
 * - Move-only: copy is deleted, move transfers ownership
 * - Fallible: growth members return BOOL (false on allocation failure)
 * - Stack-only: heap allocation of the queue itself is deleted
 *
 * Growth and API shape model TlsBuffer so adopting it there is a thin change.
 *
 * @ingroup core
 *
 * @defgroup byte_queue ByteQueue
 * @ingroup core
 * @{
 */

#pragma once

#include "core/memory/memory.h"
#include "core/types/primitives.h"
#include "core/types/span.h"

/// Minimum capacity the queue grows to (matches TlsBuffer::CheckSize)
static constexpr USIZE ByteQueueMinCapacity = 256;

/**
 * @brief Append/consume byte queue with an O(1) read cursor
 *
 * @par Example Usage:
 * @code
 * ByteQueue q;
 * if (!q.Init(1024)) return;
 * if (!q.Append(Span<const CHAR>(src, n))) return;
 * BinaryReader reader{q.GetLiveBuffer()};  // contiguous, no copy
 * // ... parse ...
 * q.Consume(reader.GetOffset());           // O(1)
 * // destructor frees automatically
 * @endcode
 */
struct ByteQueue
{
	CHAR *Data;        ///< Backing array (null when empty)
	USIZE Capacity;    ///< Allocated byte count
	USIZE Size;        ///< Bytes appended (live + dead)
	USIZE ReadPos;     ///< Consumed prefix length (live region starts at Data + ReadPos)
	BOOL OwnsMemory;   ///< false when wrapping caller-owned storage

	// Stack-only: heap allocation of the queue itself is deleted
	VOID *operator new(USIZE) = delete;
	VOID *operator new[](USIZE) = delete;
	VOID operator delete(VOID *) = delete;
	VOID operator delete[](VOID *) = delete;
	// Placement new/delete required by Result<ByteQueue, Error>
	VOID *operator new(USIZE, PVOID ptr) noexcept { return ptr; }
	VOID operator delete(VOID *, PVOID) noexcept {}

	/**
	 * @brief Construct an empty queue owning nothing
	 * @note All members are zeroed; call Init() or Wrap() before use
	 */
	constexpr ByteQueue() : Data(nullptr), Capacity(0), Size(0), ReadPos(0), OwnsMemory(true) {}

	/**
	 * @brief Construct a non-owning queue over caller-owned storage (read mode)
	 * @param data Existing bytes to wrap; not freed by the destructor
	 * @note Size starts at data.Size() with ReadPos at 0
	 */
	explicit constexpr ByteQueue(Span<CHAR> data)
		: Data(data.Data()), Capacity(data.Size()), Size(data.Size()), ReadPos(0), OwnsMemory(false)
	{
	}

	/**
	 * @brief Free the backing array if owned
	 */
	~ByteQueue()
	{
		if (Data && OwnsMemory)
			delete[] Data;
	}

	ByteQueue(const ByteQueue &) = delete;
	ByteQueue &operator=(const ByteQueue &) = delete;

	/**
	 * @brief Move-construct, stealing the source's backing array
	 * @param other Queue to steal from (left empty and non-owning)
	 */
	constexpr ByteQueue(ByteQueue &&other)
		: Data(other.Data), Capacity(other.Capacity), Size(other.Size), ReadPos(other.ReadPos), OwnsMemory(other.OwnsMemory)
	{
		other.Data = nullptr;
		other.Capacity = 0;
		other.Size = 0;
		other.ReadPos = 0;
		other.OwnsMemory = false;
	}

	/**
	 * @brief Move-assign, freeing any array this queue already owns
	 * @param other Queue to steal from (left empty and non-owning)
	 * @return Reference to this queue
	 */
	ByteQueue &operator=(ByteQueue &&other)
	{
		if (this != &other)
		{
			if (Data && OwnsMemory)
				delete[] Data;
			Data = other.Data;
			Capacity = other.Capacity;
			Size = other.Size;
			ReadPos = other.ReadPos;
			OwnsMemory = other.OwnsMemory;
			other.Data = nullptr;
			other.Capacity = 0;
			other.Size = 0;
			other.ReadPos = 0;
			other.OwnsMemory = false;
		}
		return *this;
	}

	/**
	 * @brief Allocate a backing array of the requested capacity
	 * @param capacity Byte count to allocate
	 * @return true on success, false on allocation failure or re-init of a non-empty queue
	 * @note Size and ReadPos are left at 0 — the queue starts logically empty
	 */
	[[nodiscard]] BOOL Init(USIZE capacity)
	{
		if (Data)
			return false;
		Data = new CHAR[capacity];
		if (!Data)
			return false;
		Capacity = capacity;
		Size = 0;
		ReadPos = 0;
		OwnsMemory = true;
		return true;
	}

	/**
	 * @brief Wrap caller-owned storage without taking ownership (read mode)
	 * @param data Existing bytes to expose as the live region
	 * @return true on success, false if the queue already holds memory
	 * @note The caller must keep the storage alive; the queue never frees it
	 */
	[[nodiscard]] BOOL Wrap(Span<CHAR> data)
	{
		if (Data)
			return false;
		Data = data.Data();
		Capacity = data.Size();
		Size = data.Size();
		ReadPos = 0;
		OwnsMemory = false;
		return true;
	}

	/**
	 * @brief Ensure capacity for a number of additional bytes
	 * @param appendSize Byte count that must fit beyond Size
	 * @return true on success, false on allocation failure
	 * @note Growth doubles Capacity (minimum 256) until the request fits;
	 *       the live region is compacted first so growth never copies dead bytes
	 * @note Fails on queues that do not own their memory (cannot grow a wrap)
	 */
	[[nodiscard]] BOOL CheckSize(USIZE appendSize)
	{
		USIZE required = Size + appendSize;
		if (required < Size)
			return false; // overflow
		if (required <= Capacity)
			return true;

		if (!OwnsMemory)
			return false;

		// Drop the dead prefix so the reallocation only carries live bytes
		Compact();

		// Seed at the minimum capacity, then double until the request fits.
		// The floor applies only to the first allocation so a caller that
		// deliberately Init()ed a small queue keeps its sizing intent.
		USIZE newCapacity = Capacity ? Capacity * 2 : ByteQueueMinCapacity;
		if (newCapacity < Capacity)
			return false; // overflow
		while (newCapacity < required)
		{
			USIZE doubled = newCapacity * 2;
			if (doubled < newCapacity)
				return false; // overflow
			newCapacity = doubled;
		}

		CHAR *newData = new CHAR[newCapacity];
		if (!newData)
			return false;
		Memory::Copy(newData, Data, Size);
		delete[] Data;
		Data = newData;
		Capacity = newCapacity;
		return true;
	}

	/**
	 * @brief Append bytes to the tail, growing if needed
	 * @param data Bytes to append
	 * @return true on success, false on allocation failure
	 */
	[[nodiscard]] BOOL Append(Span<const CHAR> data)
	{
		if (data.Size() == 0)
			return true;
		if (!CheckSize(data.Size()))
			return false;
		Memory::Copy(Data + Size, data.Data(), data.Size());
		Size += data.Size();
		return true;
	}

	/**
	 * @brief Consume bytes from the front without moving data
	 * @param bytes Byte count to drop; clamped to the live size
	 * @note O(1) — only advances ReadPos. Dead bytes are reclaimed later by
	 *       Compact(), which runs automatically once they exceed Capacity / 2
	 */
	VOID Consume(USIZE bytes)
	{
		if (ReadPos + bytes >= Size)
		{
			ReadPos = 0;
			Size = 0;
			return;
		}
		ReadPos += bytes;
		if (ReadPos > (Capacity >> 1))
			Compact();
	}

	/**
	 * @brief Reclaim the dead prefix by moving the live suffix down
	 * @note Public so latency-sensitive callers can force it before a large
	 *       append; also called automatically from Consume()
	 */
	VOID Compact()
	{
		if (ReadPos == 0)
			return;
		USIZE live = Size - ReadPos;
		if (live > 0)
			Memory::Move(Data, Data + ReadPos, live);
		Size = live;
		ReadPos = 0;
	}

	/**
	 * @brief Count of live (unconsumed) bytes
	 * @return Size - ReadPos
	 */
	constexpr USIZE GetSize() const
	{
		return Size - ReadPos;
	}

	/**
	 * @brief View the contiguous live region
	 * @return Span over Data + ReadPos, GetSize() bytes long
	 * @note The bytes are adjacent in memory — safe to wrap in a BinaryReader
	 *       or pass to any consumer expecting a contiguous buffer
	 */
	constexpr Span<CHAR> GetLiveBuffer() const
	{
		return Span<CHAR>(Data + ReadPos, Size - ReadPos);
	}
};

/** @} */ // end of byte_queue group

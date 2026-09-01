#include "lib/network/tls/tls_buffer.h"
#include "core/memory/memory.h"
#include "platform/console/logger.h"

/// @brief Append data to the TLS buffer
/// @param data The span of data to append to the buffer
/// @return The offset at which the data was appended

INT32 TlsBuffer::Append(Span<const CHAR> data)
{
	auto r = CheckSize((INT32)data.Size());
	if (!r)
		return -1;
	Memory::Copy(buffer + size, data.Data(), data.Size());
	size += (INT32)data.Size();
	return size - (INT32)data.Size() - startPos;
}

/// @brief Append a value of any type to the TLS buffer
/// @param count The number of bytes to append
/// @return The offset at which the bytes were appended
INT32 TlsBuffer::AppendSize(INT32 count)
{
	auto r = CheckSize(count);
	if (!r)
		return -1;
	size += count;
	return size - count - startPos;
}

/// @brief Set the size of the TLS buffer
/// @param size The new size of the buffer
/// @return Result indicating success or failure
Result<VOID, Error> TlsBuffer::SetSize(INT32 newSize)
{
	// SetSize is a write-mode operation: it discards any dead prefix too
	startPos = 0;
	readPos = 0;
	size = 0;
	auto r = CheckSize(newSize);
	if (!r)
		return Result<VOID, Error>::Err(r.Error());
	size = newSize;
	return Result<VOID, Error>::Ok();
}

/// @brief Clean up the TLS buffer by freeing memory if owned and resetting size and capacity
/// @return void
VOID TlsBuffer::Clear()
{
	if (buffer && ownsMemory)
	{
		delete[] buffer;
	}
	buffer = nullptr;
	size = 0;
	capacity = 0;
	readPos = 0;
	startPos = 0;
}

/// @brief Ensure there is enough capacity in the TLS buffer to append additional data
/// @param appendSize The size of the data to be appended
/// @return Result indicating success or failure
Result<VOID, Error> TlsBuffer::CheckSize(INT32 appendSize)
{
	// Capacity check
	if (size + appendSize <= capacity)
	{
		LOG_DEBUG("Buffer size is sufficient: %d + %d <= %d", size, appendSize, capacity);
		return Result<VOID, Error>::Ok();
	}

	// A wrapped (non-owning) buffer cannot be grown
	if (!ownsMemory)
		return Result<VOID, Error>::Err(Error::TlsBuffer_AllocationFailed);

	// Delegate the grow-and-copy to the core container: the new capacity keeps
	// TlsBuffer's original sizing (4x the request, floored at 256 bytes).
	// Only the live bytes are carried over — the dead prefix is dropped.
	UINT32 newLen = (UINT32)(size + appendSize) * 4;
	if (newLen < 256)
		newLen = 256;
	if (newLen < (UINT32)(size + appendSize))
		newLen = (UINT32)(size + appendSize); // INT32 overflow guard

	Buffer<CHAR> grown;
	if (!grown.Init(newLen) || !grown.Append(Span<const CHAR>(buffer + startPos, (USIZE)(size - startPos))))
		return Result<VOID, Error>::Err(Error::TlsBuffer_AllocationFailed);

	LOG_DEBUG("Resizing buffer from %d to %u bytes", capacity, newLen);

	delete[] buffer;
	buffer = grown.Release();
	capacity = (INT32)newLen;
	// The dead prefix is gone: size shrinks to the live byte count and the read
	// cursor — an absolute index — shifts down with it, exactly like Compact().
	readPos = (readPos > startPos) ? (readPos - startPos) : 0;
	size -= startPos;
	startPos = 0;
	ownsMemory = true;
	return Result<VOID, Error>::Ok();
}

/// @brief Remove consumed bytes from the front without moving data
/// @param bytes Number of bytes to consume from the front of the buffer
/// @return void
/// @note O(1) — advances the dead-prefix cursor. The live suffix is moved down
///       later, by Compact(), once the dead prefix exceeds capacity / 2.
VOID TlsBuffer::Consume(INT32 bytes)
{
	if (bytes <= 0)
		return;
	if (startPos + bytes >= size)
	{
		// Everything consumed: reset to an empty buffer, keep the allocation
		size = 0;
		startPos = 0;
		readPos = 0;
		return;
	}
	startPos += bytes;
	// The read cursor previously reset to 0 because Consume moved data down;
	// it now resets to the live start instead
	readPos = startPos;
	if (startPos > (capacity >> 1))
		Compact();
}

/// @brief Reclaim the dead prefix left by Consume() by moving the live suffix down
/// @return void
VOID TlsBuffer::Compact()
{
	if (startPos == 0)
		return;
	INT32 dead = startPos;
	INT32 live = size - dead;
	if (live > 0)
		Memory::Move(buffer, buffer + dead, live);
	size = live;
	startPos = 0;
	// readPos is an absolute index, so it shifts down by the reclaimed prefix
	readPos = (readPos > dead) ? (readPos - dead) : 0;
}

/// @brief Read a block of data from the TLS buffer
/// @param buf The span to receive the data read from the buffer
/// @return void
VOID TlsBuffer::Read(Span<CHAR> buf)
{
	INT32 available = size - readPos;
	INT32 count = (INT32)buf.Size();
	// Adjust count if it exceeds available data
	if (count > available)
		count = available;
	if (count > 0)
		Memory::Copy(buf.Data(), buffer + readPos, count);
	readPos += count;
}

/// @brief Read a 24-bit big-endian unsigned integer from the TLS buffer
/// @return The 24-bit value read from the buffer
UINT32 TlsBuffer::ReadU24BE()
{
	// Ensure there are at least 3 bytes available to read (24 bits)
	if (readPos + 3 > size)
	{
		readPos = size;
		return 0;
	}
	UINT8 b0 = (UINT8)buffer[readPos];
	UINT8 b1 = (UINT8)buffer[readPos + 1];
	UINT8 b2 = (UINT8)buffer[readPos + 2];
	readPos += 3;
	return ((UINT32)b0 << 16) | ((UINT32)b1 << 8) | (UINT32)b2;
}

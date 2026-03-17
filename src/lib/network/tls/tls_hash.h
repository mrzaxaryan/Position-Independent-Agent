#pragma once

#include "core/core.h"
#include "lib/network/tls/tls_buffer.h"

// Hash structure
class TlsHash
{
private:
	TlsBuffer cache;
public:
	// Default constructor and destructor
	TlsHash() = default;
	~TlsHash() = default;

	// Non-copyable
	TlsHash(const TlsHash &) = delete;
	TlsHash &operator=(const TlsHash &) = delete;

	// Movable
	TlsHash(TlsHash &&) = default;
	TlsHash &operator=(TlsHash &&) = default;

	// Stack-only — placement new/delete required by Result<TlsHash, Error>
	VOID *operator new(USIZE) = delete;
	VOID operator delete(VOID *) = delete;
	VOID *operator new(USIZE, PVOID ptr) noexcept { return ptr; }
	VOID operator delete(VOID *, PVOID) noexcept {}

	// Reset the hash cache
	VOID Reset();
	// Append data to the hash cache
	VOID Append(Span<const CHAR> buffer);
	// Get the hash value from the cache; dispatch on out.Size() (32 = SHA-256, 48 = SHA-384)
	VOID GetHash(Span<CHAR> out);
};
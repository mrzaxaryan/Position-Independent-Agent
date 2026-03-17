#pragma once

#include "platform/platform.h"
#include "lib/network/tls/tls_buffer.h"
#include "lib/network/tls/tls_cipher.h"

// TLS state
struct TlsState
{
	INT32 contentType;   // TLS content type
	INT32 handshakeType; // TLS handshake type
};

class TlsClient final
{
private:
	PCCHAR host;
	IPAddress ip;
	Socket context;
	TlsCipher crypto;
	BOOL secure;             // Whether to use TLS handshake or plain TCP
	INT32 stateIndex;        // Current state index
	TlsBuffer sendBuffer;    // Send buffer
	TlsBuffer recvBuffer;    // Receive buffer
	TlsBuffer channelBuffer; // Channel buffer for received data
	INT32 channelBytesRead;  // Number of bytes read from channel buffer
	[[nodiscard]] Result<INT32, Error> ReadChannel(Span<CHAR> output);
	[[nodiscard]] Result<void, Error> ProcessReceive();
	[[nodiscard]] Result<void, Error> OnPacket(INT32 packetType, INT32 version, TlsBuffer &TlsReader);
	[[nodiscard]] Result<void, Error> OnServerFinished();
	[[nodiscard]] Result<void, Error> VerifyFinished(TlsBuffer &TlsReader);
	[[nodiscard]] Result<void, Error> OnServerHelloDone();
	[[nodiscard]] Result<void, Error> OnServerHello(TlsBuffer &TlsReader);
	[[nodiscard]] Result<void, Error> SendChangeCipherSpec();
	[[nodiscard]] Result<void, Error> SendClientExchange();
	[[nodiscard]] Result<void, Error> SendClientFinished();
	[[nodiscard]] Result<void, Error> SendClientHello(const CHAR *host);
	[[nodiscard]] Result<void, Error> SendPacket(INT32 packetType, INT32 ver, TlsBuffer &TlsBuffer);

	// Private trivial constructor — only used by Create()
	TlsClient(PCCHAR host, const IPAddress &ipAddress, Socket &&socket, BOOL secure)
		: host(host), ip(ipAddress), context(static_cast<Socket &&>(socket)), secure(secure), stateIndex(0), channelBytesRead(0) {}

public:
	VOID *operator new(USIZE) = delete;
	VOID operator delete(VOID *) = delete;
	// Placement new required by Result<TlsClient, Error>
	VOID *operator new(USIZE, PVOID ptr) noexcept { return ptr; }
	VOID operator delete(VOID *, PVOID) noexcept {}
	TlsClient() : host(nullptr), ip(), secure(true), stateIndex(0), channelBytesRead(0) {}

	// Factory — caller MUST check the result (enforced by [[nodiscard]])
	[[nodiscard]] static Result<TlsClient, Error> Create(PCCHAR host, const IPAddress &ipAddress, UINT16 port, BOOL secure = true);

	// Destructor
	~TlsClient()
	{
		if (IsValid())
			(void)Close();
	}

	// Disallow copy construction and assignment
	TlsClient(const TlsClient &) = delete;
	TlsClient &operator=(const TlsClient &) = delete;

	// Move semantics
	TlsClient(TlsClient &&other) noexcept
		: host(other.host), ip(other.ip), context(static_cast<Socket &&>(other.context)), crypto(static_cast<TlsCipher &&>(other.crypto)), secure(other.secure), stateIndex(other.stateIndex), sendBuffer(static_cast<TlsBuffer &&>(other.sendBuffer)), recvBuffer(static_cast<TlsBuffer &&>(other.recvBuffer)), channelBuffer(static_cast<TlsBuffer &&>(other.channelBuffer)), channelBytesRead(other.channelBytesRead)
	{
		other.host = nullptr;
		other.secure = true;
		other.stateIndex = 0;
		other.channelBytesRead = 0;
	}

	TlsClient &operator=(TlsClient &&other) noexcept
	{
		if (this != &other)
		{
			if (IsValid())
				(void)Close();
			host = other.host;
			ip = other.ip;
			context = static_cast<Socket &&>(other.context);
			crypto = static_cast<TlsCipher &&>(other.crypto);
			secure = other.secure;
			stateIndex = other.stateIndex;
			sendBuffer = static_cast<TlsBuffer &&>(other.sendBuffer);
			recvBuffer = static_cast<TlsBuffer &&>(other.recvBuffer);
			channelBuffer = static_cast<TlsBuffer &&>(other.channelBuffer);
			channelBytesRead = other.channelBytesRead;
			other.host = nullptr;
			other.secure = true;
			other.stateIndex = 0;
			other.channelBytesRead = 0;
		}
		return *this;
	}

	// Accessors and operations
	BOOL IsValid() const { return context.IsValid(); }
	BOOL IsSecure() const { return secure; }
	[[nodiscard]] Result<void, Error> Open();
	[[nodiscard]] Result<void, Error> Close();
	[[nodiscard]] Result<SSIZE, Error> Read(Span<CHAR> buffer);
	[[nodiscard]] Result<UINT32, Error> Write(Span<const CHAR> buffer);
};

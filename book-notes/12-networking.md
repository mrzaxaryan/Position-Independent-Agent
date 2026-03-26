# 12 - Network Stack

Questions about DNS, HTTP, TLS 1.3, and WebSocket.

---

### How does the full network connection chain work?
**File:** `src/beacon/README.md` **Line(s):** (architecture diagram)
**Type:** QUESTION
**Priority:** HIGH

Lay out the layering from bottom to top so readers have a map before diving into details:
```
1. DNS Resolution (dns_client.h)
   "relay.nostdlib.workers.dev" -> 1.2.3.4

2. TCP Socket (socket.h)
   Connect to 1.2.3.4:443

3. TLS 1.3 Handshake (tls_client.h)
   Exchange keys, establish encrypted channel

4. HTTP/1.1 Request (http_client.h)
   GET /agent HTTP/1.1 with Upgrade: websocket

5. WebSocket (websocket_client.h)
   Binary frames carrying commands and responses
```
Each layer wraps the one below it. Trace a complete connection from DNS lookup to the first WebSocket message -- that gives readers the full picture before they look at any individual component.

---

### What is DNS-over-HTTPS and why use it?
**File:** `src/lib/network/dns/dns_client.h` **Line(s):** 56-161
**Type:** QUESTION
**Priority:** MEDIUM

Regular DNS sends a UDP packet to port 53, usually to your ISP's resolver. It's unencrypted and trivial to monitor or block.

DNS-over-HTTPS (DoH) sends the same DNS query as an HTTPS POST to a web server. Cloudflare and Google both provide DoH endpoints. The traffic is indistinguishable from normal HTTPS browsing, and since it's encrypted, network monitors can't see what domains you're resolving.

The fallback chain: Cloudflare 1.1.1.1, then 1.0.0.1, then Google 8.8.8.8, then 8.8.4.4. If AAAA (IPv6) lookup fails, it retries with A (IPv4).

---

### What is TLS 1.3 and what happens during the handshake?
**File:** `src/lib/network/tls/tls_client.h` **Line(s):** 28-38
**Type:** QUESTION
**Priority:** HIGH

Most people know TLS means "encrypted." Here's what actually happens on the wire:
```
Client                              Server
  |                                   |
  |--- ClientHello ------------------>|
  |    (supported ciphers, random,    |
  |     ECDH public key)              |
  |                                   |
  |<-- ServerHello + Certificate -----|
  |    (chosen cipher, random,        |
  |     ECDH public key, cert chain)  |
  |                                   |
  |    [Both compute shared secret]   |
  |    [Derive encryption keys]       |
  |                                   |
  |<-- EncryptedExtensions -----------|
  |<-- CertificateVerify -------------|
  |<-- Finished ----------------------|
  |                                   |
  |--- Finished --------------------->|
  |                                   |
  |=== Encrypted application data ====|
```
TLS 1.3 completes in just 1 round trip (down from 2 in TLS 1.2). This implementation only supports ChaCha20-Poly1305 as the cipher suite, which keeps things much simpler. ECDH key exchange uses P-256 or P-384 curves. Once the handshake is done, all data flows through the derived encryption keys.

---

### What is the TLS state machine?
**File:** `src/lib/network/tls/tls_client.h` **Line(s):** 28-38
**Type:** QUESTION
**Priority:** MEDIUM

The private methods reveal a state machine driven by `stateIndex`. Each state processes one handshake message and advances: SendClientHello, OnServerHello, OnEncryptedExtensions, OnCertificate, OnCertificateVerify, OnServerFinished, SendClientFinished, Done.

Any state failure aborts the entire handshake. After completion, Read and Write just encrypt and decrypt transparently -- the caller doesn't need to think about TLS anymore.

---

### How does HKDF key derivation work?
**File:** `src/lib/crypto/sha2.h` **Line(s):** (HMAC used by TLS)
**Type:** QUESTION
**Priority:** MEDIUM

HKDF (HMAC-based Key Derivation Function, RFC 5869) takes a single shared secret and expands it into multiple keys. Two phases:
1. **Extract**: `PRK = HMAC(salt, input_key_material)` -- concentrates entropy into a fixed-size pseudorandom key
2. **Expand**: `OKM = HMAC(PRK, info || counter)` -- generates as many keys as you need

From one shared secret, TLS 1.3 derives client write key, server write key, client write IV, server write IV, plus separate handshake traffic keys. All from the same root.

---

### How does HTTP/1.1 work at the raw level?
**File:** `src/lib/network/http/http_client.h` **Line(s):** 153-208
**Type:** QUESTION
**Priority:** MEDIUM

People use HTTP every day without ever seeing the wire format. Here's what actually gets sent over TLS:
```
Request (bytes sent over TLS):
  GET /agent HTTP/1.1\r\n
  Host: relay.nostdlib.workers.dev\r\n
  Connection: Upgrade\r\n
  Upgrade: websocket\r\n
  Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n
  Sec-WebSocket-Version: 13\r\n
  \r\n

Response (bytes received):
  HTTP/1.1 101 Switching Protocols\r\n
  Upgrade: websocket\r\n
  Connection: Upgrade\r\n
  Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n
  \r\n
```
Each line ends with \r\n (CRLF). Headers end with a blank line (\r\n\r\n). The HTTP client detects end-of-headers by watching for that \r\n\r\n pattern with a sliding window -- more on that in the rolling window entry below.

---

### What is a WebSocket and why not just use HTTP?
**File:** `src/lib/network/websocket/websocket_client.h` **Line(s):** 26-147
**Type:** QUESTION
**Priority:** HIGH

HTTP is request/response: client asks, server answers, then the connection sits idle. For a C2 agent, the server needs to push commands to the client whenever it wants -- HTTP can't do that efficiently without constant polling.

WebSocket solves this. It starts as an HTTP Upgrade request, then switches to a bidirectional binary frame protocol. Both sides can send messages at any time. Each frame carries an opcode (text/binary/close/ping/pong), a length, an optional mask, and the payload.

Important asymmetry: client-to-server frames MUST be masked (XOR with a random 32-bit key). Server-to-client frames MUST NOT be masked.

---

### What is WebSocket frame masking and why is it required?
**File:** `src/lib/network/websocket/websocket_client.h` **Line(s):** 220-235
**Type:** QUESTION
**Priority:** MEDIUM

Every payload byte from the client gets XORed with a byte from a 4-byte masking key:
```
masked[i] = payload[i] ^ key[i % 4]
```
The key is sent right in the frame header -- it's not a secret. So what's the point?

It prevents cache poisoning attacks against HTTP proxies. Without masking, a crafted WebSocket frame could look like a valid HTTP response to an intermediary proxy, poisoning its cache. Masking makes the payload look like random noise, defeating the attack. And since XOR is self-inverse, applying the mask a second time unmasks the data.

---

### How does the rolling window header detection work?
**File:** `src/lib/network/http/http_client.h` **Line(s):** 208
**Type:** QUESTION
**Priority:** LOW

ReadResponseHeaders needs to find the \r\n\r\n (0x0D 0x0A 0x0D 0x0A) that marks the end of HTTP headers. It reads one byte at a time, maintaining a 4-byte sliding window. After each byte, it checks whether the window matches the terminator.

This works even when the \r\n\r\n sequence is split across multiple TCP reads, which is the whole reason for doing it this way. You can't just read the entire response into a buffer and search -- you don't know the total size upfront.

---

### What happens if DNS resolution fails for all providers?
**File:** `src/lib/network/dns/dns_client.h` **Line(s):** 135
**Type:** QUESTION
**Priority:** LOW

If all four servers fail (1.1.1.1, 1.0.0.1, 8.8.8.8, 8.8.4.4), Resolve returns an Error. The beacon main loop catches this and retries the entire connection after sleeping to avoid burning CPU. In practice, total DNS failure usually means one of two things: network isolation (air-gapped host) or a firewall blocking HTTPS to known DNS providers.

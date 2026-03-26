# 11 - Cryptography

Questions about the crypto implementations.

---

### Why implement crypto from scratch?
**File:** `src/lib/crypto/` (directory)
**Type:** QUESTION
**Priority:** HIGH

"Why not just use OpenSSL?" Because OpenSSL is ~500K lines of code with a pile of dependencies, can't compile in freestanding mode (-nostdlib), and even if it could, its lookup tables and global state would create data sections. Non-starter.

This project implements only what TLS 1.3 actually needs:
- ChaCha20-Poly1305 for symmetric encryption
- SHA-256 / SHA-384 for hashing and HMAC
- ECDH with P-256 and P-384 for key exchange
- HKDF for key derivation

All of it is position-independent: no lookup tables, no global state.

---

### What is ChaCha20 and how does it encrypt data?
**File:** `src/lib/crypto/chacha20.h` **Line(s):** 195-356
**Type:** QUESTION
**Priority:** HIGH

ChaCha20 is the stream cipher used for TLS encryption. It generates a pseudorandom keystream and XORs it with plaintext to get ciphertext. XOR again with the same keystream to decrypt -- it's self-inverse.

The internal state is a 4x4 matrix of 32-bit words:
```
"expa" "nd 3" "2-by" "te k"   <- constants
 key0   key1   key2   key3    <- 256-bit key (8 words)
 key4   key5   key6   key7
 ctr    nonce0 nonce1 nonce2  <- 32-bit counter + 96-bit nonce
```
Twenty rounds of "quarter round" operations mix this matrix thoroughly ("20 rounds" means 10 iterations of 4 column rounds + 4 diagonal rounds). Each block produces 64 bytes of keystream.

---

### What is Poly1305 and what are "limbs"?
**File:** `src/lib/crypto/chacha20.h` **Line(s):** 80-165
**Type:** QUESTION
**Priority:** HIGH

Poly1305 is the MAC (message authentication code) half of ChaCha20-Poly1305. It computes a 128-bit tag that proves a message hasn't been tampered with. Together with ChaCha20, this forms an AEAD scheme -- Authenticated Encryption with Associated Data.

The math: polynomial evaluation modulo 2^130 - 5. Each 16-byte block of the message becomes a number, and you accumulate `acc = (acc + block) * r mod (2^130 - 5)`, then add a secret key `s` at the end to get the tag.

The "limbs" part is honestly the hardest thing to explain here. The 130-bit number gets split into 5 pieces of 26 bits each. Why 26? Because on a 32-bit CPU, multiplying two 26-bit values gives a 52-bit result, which fits in a 64-bit register. If you used full 32-bit limbs, the products would overflow. Keeping limbs at 26 bits avoids needing 128-bit arithmetic entirely.

---

### What is the ChaCha20-Poly1305 "counter = 1" rule?
**File:** `src/lib/crypto/chacha20.cc` **Line(s):** 641
**Type:** QUESTION
**Priority:** MEDIUM

Block 0 (counter=0) is reserved. When encrypting:
1. Run ChaCha20 with counter=0 to generate 64 bytes
2. Take the first 32 bytes as the Poly1305 key
3. Start actual encryption at counter=1, 2, 3...

This way each message gets a unique Poly1305 key derived from the ChaCha20 key + nonce combination. RFC 8439 Section 2.6 specifies this exact construction.

---

### What is constant-time comparison and why does it matter?
**File:** `src/lib/crypto/chacha20.cc` **Line(s):** 661
**Type:** QUESTION
**Priority:** HIGH

Tag verification uses `diff |= a[i] ^ b[i]` instead of returning early on mismatch. This matters more than it might seem.

A naive `if (a[i] != b[i]) return false;` bails on the first wrong byte. An attacker can measure how long the comparison takes: if it returns after 1 byte, the first byte is wrong; if it returns after 8 bytes, the first 8 are correct. Byte-by-byte, the attacker reconstructs the valid tag through timing alone.

Constant-time fix:
```
UINT32 diff = 0;
for (int i = 0; i < 16; i++) diff |= a[i] ^ b[i];
return diff == 0;
```
Always checks every byte. Always takes the same time. The attacker learns nothing.

---

### How does SHA-256 avoid lookup tables?
**File:** `src/lib/crypto/sha2.cc` **Line(s):** 27-54
**Type:** QUESTION
**Priority:** MEDIUM

SHA-256 needs 64 round constants (K[]) and 8 initial hash values (H0[]). Standard implementations store these as arrays in .rodata -- which would break position independence.

Instead, FillK() and FillH0() functions populate arrays on the stack using immediate values embedded in the instructions themselves. These functions are marked NOINLINE to stop the compiler from being clever and constant-folding the values back into a data section. The actual values come from the SHA-256 spec: cube roots of the first 64 primes and square roots of the first 8 primes.

---

### What is HMAC and how does it use SHA-256?
**File:** `src/lib/crypto/sha2.h` **Line(s):** 342-418
**Type:** QUESTION
**Priority:** MEDIUM

HMAC (Hash-based Message Authentication Code) is a way to hash a message together with a secret key. It shows up throughout TLS key derivation.

The construction: `HMAC(key, message) = SHA256(opad || SHA256(ipad || message))` where ipad is the key XOR'd with 0x36 repeated, and opad is the key XOR'd with 0x5C repeated.

There's a nice optimization in the code: it caches the SHA state after processing the key pads (via CopyStateFrom). When computing many HMACs with the same key, you skip re-hashing the key each time -- just restore the cached state and feed in the new message.

---

### What is ECDH key exchange?
**File:** `src/lib/crypto/ecc.h` **Line(s):** 79-205
**Type:** QUESTION
**Priority:** HIGH

ECDH (Elliptic Curve Diffie-Hellman) lets two parties agree on a shared secret over an insecure channel without an eavesdropper being able to compute it. This is what makes the TLS 1.3 handshake work.

The flow:
1. Client picks a random private key `a`, computes public key `A = a*G` (G is a known point on the curve)
2. Server picks a random private key `b`, computes public key `B = b*G`
3. They exchange public keys in the open
4. Client computes `a*B = a*b*G`. Server computes `b*A = b*a*G`. Same value.
5. An eavesdropper sees A and B but can't get `a*b*G` without knowing a or b -- that's the elliptic curve discrete logarithm problem

The shared secret then feeds into HKDF to derive the actual encryption keys for the session.

---

### What is the Montgomery ladder and why constant-time?
**File:** `src/lib/crypto/ecc.h` **Line(s):** 101-104
**Type:** QUESTION
**Priority:** MEDIUM

Scalar multiplication -- computing `k*G` where k is a large number -- is done bit by bit. The naive approach: if the current bit is 1, double-and-add; if 0, just double. Problem: double-and-add takes longer than just doubling, so an attacker measuring power consumption or execution time can read off the bits of your private key.

The Montgomery ladder always performs the same two operations per bit, regardless of the bit value. It just swaps which variable gets which result. Same timing for every key, no side-channel leakage.

---

### What are Jacobian coordinates?
**File:** `src/lib/crypto/ecc.h` **Line(s):** 101-104
**Type:** QUESTION
**Priority:** LOW

In normal (affine) coordinates, a point is (x, y) and point addition requires a modular inverse -- basically an expensive division. Jacobian coordinates represent a point as (X, Y, Z) where x = X/Z^2 and y = Y/Z^3. The payoff: point addition in Jacobian form uses only multiplication, no division at all. You convert back to affine just once, at the very end. Roughly 3x faster per point operation.

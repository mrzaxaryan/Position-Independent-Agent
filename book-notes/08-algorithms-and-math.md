# 08 - Algorithms and Math

Questions about DJB2 hashing, Base64, PRNG, byte order, and bit operations.

---

### What is DJB2 hashing and what is it used for?
**File:** `src/core/algorithms/djb2.h` **Line(s):** 123-157
**Type:** QUESTION
**Priority:** HIGH

DJB2 is a simple, fast hash function (not cryptographic) and it's central to how the project resolves Windows APIs. The core formula is `hash = ((hash << 5) + hash) + character`, which works out to `hash * 33 + character`.

The usage pattern: at compile time, `DJB2::HashCompileTime("ZwCreateFile")` produces a constant like 0xABCD1234. At runtime, the code walks ntdll.dll's export table, hashes each function name, and compares against that constant. Match found means function found.

Why hashing instead of string comparison? Strings would land in `.rodata`. Hash values are just integer constants that embed directly in instructions -- no data section needed.

---

### What is the compile-time seed and why does it change every build?
**File:** `src/core/algorithms/djb2.h` **Line(s):** 63-69, 123
**Type:** QUESTION
**Priority:** HIGH

If the DJB2 seed were fixed, the hash of "ZwCreateFile" would be the same constant in every binary ever compiled. Antivirus and EDR products could just scan for those values as signatures.

The fix: seed the hash with a value derived from `__DATE__` (e.g., "Mar 25 2026"). The seed itself is computed via FNV-1a (another hash function) at compile time, and `consteval` guarantees none of this runs at runtime. A binary compiled Monday ends up with completely different hash constants than one compiled Tuesday. Same source, different signatures.

---

### What is constexpr vs consteval?
**File:** `src/core/algorithms/djb2.h` **Line(s):** 148-157, 211-221
**Type:** QUESTION
**Priority:** MEDIUM

Both `Hash()` (constexpr) and `HashCompileTime()` (consteval) exist, and the distinction matters.

`constexpr` means the function *can* run at compile time but is also allowed to run at runtime. Pass it a runtime string (a `char*` pointer) and it executes at runtime. Pass a string literal and the compiler *may* fold it at compile time -- but there's no guarantee.

`consteval` means it *must* run at compile time. Period. If it can't, the code won't compile. The signature uses `const TChar (&value)[N]` (an array reference) which only accepts literals; passing a `char*` is a compile error.

Both are needed. `Hash()` handles runtime hashing when walking export tables. `HashCompileTime()` produces the compile-time constants those runtime hashes get compared against.

---

### How does Base64 encoding work without a lookup table?
**File:** `src/core/algorithms/base64.cc` **Line(s):** 15-24
**Type:** QUESTION
**Priority:** HIGH

Standard Base64 uses a 64-byte lookup table mapping indices 0-63 to characters. That table would go straight into `.rodata`. The alternative here is pure arithmetic:
```
Standard lookup table approach:
  table[0]='A', table[1]='B', ... table[25]='Z',
  table[26]='a', ... table[51]='z',
  table[52]='0', ... table[61]='9',
  table[62]='+', table[63]='/'

Arithmetic approach (this project):
  if (v < 26) return 'A' + v;           // 0-25  -> A-Z
  if (v < 52) return 'a' + (v - 26);    // 26-51 -> a-z
  if (v < 62) return '0' + (v - 52);    // 52-61 -> 0-9
  if (v == 62) return '+';              // 62    -> +
  return '/';                           // 63    -> /
```

Same output, computed instead of looked up. All logic lives in code, no data section involved.

---

### What is Base64 and why is it used?
**File:** `src/core/algorithms/base64.h` **Line(s):** 47-54
**Type:** QUESTION
**Priority:** MEDIUM

Base64 converts arbitrary binary data into printable ASCII. Every 3 input bytes become 4 output characters. It shows up whenever binary data needs to travel through a text-only channel: HTTP headers, JSON payloads, email attachments.

In this project specifically, the WebSocket `Sec-WebSocket-Key` is Base64-encoded, and the compiled `.bin` shellcode gets Base64-encoded for transport. The `=` padding at the end signals how many bytes were in the final group -- no padding means the input was a multiple of 3, one `=` means 2 bytes, two `==` means 1 byte.

---

### What is xorshift64 and why use it instead of rand()?
**File:** `src/core/math/prng.h` **Line(s):** 96-102
**Type:** QUESTION
**Priority:** MEDIUM

`rand()` is a libc function -- gone with `-nostdlib`. xorshift64 is a dead-simple replacement using only XOR and bit shifts:
```
state ^= state << 13;
state ^= state >> 7;
state ^= state << 17;
```

Three lines, period of 2^64 - 1 (hits every non-zero state before repeating). Not cryptographically secure -- if you know the internal state, you can predict the output. That's fine here since it's only used for things like random file names and WebSocket masking keys, not key generation. Seeded from hardware counters (RDTSC, etc.) so the starting state is at least unpredictable.

---

### How does GetChar generate a random letter without division?
**File:** `src/core/math/prng.h` **Line(s):** 120-128
**Type:** QUESTION
**Priority:** LOW

`((val & 0x7FFF) * 26) >> 15` maps a random number to the range 0-25. The obvious approach would be `val % 26`, but division is slow on some architectures.

The trick: `(val * 26) / 32768` gives a value in [0, 25], and `>> 15` is the same as dividing by 32768 but compiles to a single shift instruction. The `& 0x7FFF` mask limits val to [0, 32767] so the multiplication doesn't overflow. Classic embedded programming technique for avoiding hardware division.

---

### What is byte swapping and when do you need it?
**File:** `src/core/math/byteorder.h` **Line(s):** 57-91
**Type:** QUESTION
**Priority:** MEDIUM

ByteOrder::Swap16/32/64 reverse byte order to handle endianness differences. Quick recap: little-endian (x86, ARM default) stores the least significant byte first, so 0x1234 sits in memory as [34, 12]. Big-endian (network protocols, some ARM modes) stores most significant first: [12, 34].

Network protocols -- TCP, DNS, TLS -- all use big-endian ("network byte order"). So when sending port 443 (0x01BB) from an x86 machine, the in-memory representation [BB, 01] needs to become [01, BB] on the wire. That's what Swap16 does.

Under the hood, `__builtin_bswap16/32/64` compiles down to a single instruction: BSWAP on x86, REV on ARM.

---

### What is bit rotation and why is it important?
**File:** `src/core/math/bitops.h` **Line(s):** 54-89
**Type:** QUESTION
**Priority:** MEDIUM

Rotr32 and Rotl32 show up constantly in the crypto code. Rotation shifts bits in one direction, but instead of dropping off the edge, they wrap around to the other side.

Example: `Rotr32(0xABCD1234, 8) = 0x34ABCD12` -- the lowest 8 bits (0x34) wrap around to become the highest 8 bits. The implementation pattern is `(x >> n) | (x << (32 - n))`, which every modern compiler recognizes and emits as a single ROL or ROR instruction.

These are fundamental building blocks of SHA-256 (Sigma functions), ChaCha20 (quarter round), and other cryptographic primitives used throughout the project.

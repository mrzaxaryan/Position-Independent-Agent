# 07 - Compiler Runtime

Questions about the compiler runtime helpers for 32-bit architectures.

---

### Why does 32-bit code need special division helpers?
**File:** `src/core/compiler/compiler_runtime.i386.cc` **Line(s):** 41-91
**Type:** QUESTION
**Priority:** HIGH

On a 64-bit CPU, dividing two 64-bit numbers is one instruction. On 32-bit, the hardware division only handles 32-bit operands. So when your C++ code does `uint64_t a = b / c`, the compiler emits a call to a helper function like `__udivdi3`. Normally that lives in libgcc or compiler-rt, but with `-nostdlib` those are gone too.

We have to supply our own. The algorithm is binary long division -- same long division you learned in school, just base-2 instead of base-10.

---

### What is binary long division?
**File:** `src/core/compiler/compiler_runtime.i386.cc` **Line(s):** 72-87
**Type:** QUESTION
**Priority:** MEDIUM

Walk through an example to make it concrete:
```
Example: 13 / 3 in binary (1101 / 11)

         0100  <- quotient (4)
      -------
  11 | 1101
       11     subtract at bit 3: 1101 - 1100 = 0001
       ---
       0001
         11   can't subtract at bit 1 (1 < 11)
          11  can't subtract at bit 0 (01 < 11)

Quotient: 0100 = 4, Remainder: 0001 = 1
Check: 4 * 3 + 1 = 13
```

Start from the most significant bit (found via `__builtin_clzll`). At each position, try subtracting the divisor shifted to that position. If the subtraction works, set that quotient bit to 1. If not, leave it 0 and move on. That's the whole thing.

---

### What are the power-of-2 fast paths?
**File:** `src/core/compiler/compiler_runtime.i386.cc` **Line(s):** 54-56
**Type:** QUESTION
**Priority:** LOW

`(denominator & (denominator - 1)) == 0` detects powers of 2. When the divisor is a power of 2, division is just a right shift (`x / 8 = x >> 3`) and modulo is just a mask (`x % 8 = x & 7`).

The bit trick: powers of 2 have exactly one bit set (8 = `1000`). Subtracting 1 clears that bit and sets everything below it (7 = `0111`). AND them together and you get zero. Non-powers-of-2 won't zero out. This fast path skips the expensive long division loop entirely.

---

### What are "naked" functions in the ARM runtime?
**File:** `src/core/compiler/compiler_runtime.armv7a.cc` **Line(s):** 356-370
**Type:** QUESTION
**Priority:** HIGH

`__attribute__((naked))` tells the compiler: "emit no prologue, no epilogue -- I'm writing the entire function body in inline assembly." Normally the compiler adds code to save/restore registers and manage the stack frame. Naked strips all of that out.

Why do it? ARM EABI functions have rigid requirements about which registers hold arguments and return values. `__aeabi_uldivmod` must receive the numerator in r0:r1, denominator in r2:r3, and return quotient in r0:r1 with remainder in r2:r3. The compiler's default register allocation might not respect that convention, so you take full manual control.

---

### What is the ARM EABI and why is it different from x86?
**File:** `src/core/compiler/compiler_runtime.armv7a.cc` **Line(s):** 175-200
**Type:** QUESTION
**Priority:** MEDIUM

ARM EABI (Embedded Application Binary Interface) defines calling conventions for ARM. Different world from x86.

On x86 32-bit, arguments go on the stack and results come back in EAX (or EAX:EDX for 64-bit values). On ARM EABI, the first four arguments land in r0-r3 and results come back in r0 (or r0:r1 for 64-bit).

There's a subtlety with division: `__aeabi_uidivmod` returns both quotient AND remainder packed together -- quotient in r0, remainder in r1. The 64-bit variant `__aeabi_uldivmod` returns quotient in r0:r1 and remainder in r2:r3. These function names are standardized; the compiler generates calls to them automatically whenever you write division in C++.

---

### What are the __aeabi_memcpy and __aeabi_memset wrappers?
**File:** `src/core/compiler/compiler_runtime.armv7a.cc` **Line(s):** 647-718
**Type:** QUESTION
**Priority:** LOW

ARM EABI has its own memory operation helpers. This one's gonna trip people up: `__aeabi_memset(dest, size, value)` has the size and value arguments *swapped* compared to C's `memset(dest, value, size)`. ARM compilers emit calls to the EABI variant instead of the standard one.

Get the argument order wrong and memory gets silently corrupted. No crash, no warning -- just wrong data. Definitely worth a callout box or warning in the text.

---

### Why does RISC-V 32 need special builtins?
**File:** `src/core/compiler/compiler_builtins.h` **Line(s):** 5-64
**Type:** QUESTION
**Priority:** MEDIUM

RISC-V 32-bit without the Zbb extension has no hardware CLZ/CTZ instructions. CLZ (Count Leading Zeros) answers "how many zero bits before the first 1?" -- for example, `CLZ(0x00FF0000) = 8`. CTZ counts from the other end. These get used in the division algorithm to find the starting bit position.

Here's the problem: Clang's `__builtin_clzll` on RISC-V 32 generates a De Bruijn lookup table that lands in `.rodata`, breaking position independence. The fix is a hand-rolled binary search that uses no table at all. It halves the search space each step -- check if the lower 16 bits are zero, then lower 8, then 4, and so on. Slightly slower, but no data section.

---

### What are the floating-point conversion functions?
**File:** `src/core/compiler/compiler_runtime.armv7a.cc` **Line(s):** 470-635
**Type:** QUESTION
**Priority:** LOW

ARM 32-bit needs software conversions between int64 and double. Quick refresher: IEEE-754 double has 1 sign bit, 11 exponent bits, and 52 mantissa bits.

Converting int64 to double means finding the MSB position (that becomes the exponent), shifting the mantissa to fit in 52 bits, and assembling the bit pattern. Going the other direction, you extract the exponent and mantissa from the double, then shift the mantissa based on the exponent to recover the integer.

The compiler emits calls to `__aeabi_l2d` and `__aeabi_d2lz` whenever code converts between `long long` and `double`. On 64-bit ARM the hardware handles this natively, so these only matter for the 32-bit target.

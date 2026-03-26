# 06 - Memory and String Operations

Questions about the custom memory and string implementations.

---

### Why reimplement memset/memcpy/memcmp from scratch?
**File:** `src/core/memory/memory.cc` **Line(s):** 12-129
**Type:** QUESTION
**Priority:** HIGH

These are normally provided by libc, which we don't have. But here's the part that catches people off guard: even with `-nostdlib`, the compiler still *generates calls* to memset, memcpy, and memcmp behind your back -- zeroing a struct, copying a buffer, that sort of thing. If these functions don't exist at link time, you get `undefined reference to 'memset'` and friends.

So we have to provide our own. They're marked `extern "C"` so the linker finds them by their standard C names.

---

### What is the "word-at-a-time" optimization in memset?
**File:** `src/core/memory/memory.cc` **Line(s):** 25-40
**Type:** QUESTION
**Priority:** MEDIUM

The memset implementation doesn't just loop byte-by-byte. Walk through it with a diagram:
```
memset(buffer, 0xAB, 20) on a 64-bit system:

Step 1: Write bytes until pointer is 8-byte aligned
  [AB][AB]          (2 bytes to align)

Step 2: Replicate byte into 8-byte word
  0xAB -> 0xABABABABABABABAB

Step 3: Write full words (8 bytes at a time)
  [ABABABABABABABAB][ABABABABABABABAB]   (16 bytes)

Step 4: Write remaining bytes
  [AB][AB]          (2 bytes left)
```

Writing 8 bytes at once is dramatically faster than one at a time -- the CPU's memory bus is 64 bits wide, so one 8-byte write is a single bus transaction. The alignment step at the start ensures we never do unaligned word writes, which are slow on x86 and actually crash on some ARM configurations.

---

### What is the difference between memcpy and memmove?
**File:** `src/core/memory/memory.cc` **Line(s):** 52-103
**Type:** QUESTION
**Priority:** MEDIUM

They look the same, but memcpy has undefined behavior when source and destination overlap. memmove handles overlap correctly. Here's why overlap is a problem:
```
src:  [A B C D E]
dst:    [A B C D E]    (dst starts 2 bytes after src)

Forward copy: copy A->dst[0], B->dst[1], C->dst[2]...
But dst[2] IS src[4]! By the time we copy src[4], we've already
overwritten it with C.
```

memmove solves this by checking direction: if dst < src, copy forward (safe). If dst > src, copy backward (safe). Simple once you see it.

---

### Why do we need __bzero and bzero?
**File:** `src/core/memory/memory.cc` **Line(s):** 121-129
**Type:** QUESTION
**Priority:** LOW

Both are thin wrappers around `memset(..., 0, ...)`. `bzero` is a legacy POSIX function; `__bzero` is an Apple-specific variant. They exist because LLVM's Link-Time Optimizer sometimes decides to emit calls to these, even though the source code never mentions them. Without implementations, you get linker errors. Purely defensive -- "just in case" stubs.

---

### Why is the COMPILER_RUNTIME attribute on memory functions?
**File:** `src/core/memory/memory.cc` **Line(s):** 12
**Type:** QUESTION
**Priority:** MEDIUM

This attribute combines `noinline` + `used` + `optnone`, and each part matters:

`noinline` -- don't inline the function. If memcpy gets inlined, the compiler might optimize it in ways that generate data sections. `used` -- don't remove it as dead code. The compiler generates implicit calls to memcpy/memset that the optimizer can't always see, so it might incorrectly conclude the function is unused. `optnone` -- skip all optimization passes. Without this, the optimizer might "improve" our memcpy by introducing SIMD instructions that reference constant vectors in `.rodata`.

All three are load-bearing. Remove any one and you risk subtle breakage.

---

### How do string functions work without the C standard library?
**File:** `src/core/string/string.h` **Line(s):** 70-176
**Type:** QUESTION
**Priority:** HIGH

The string module reimplements strlen, strcmp, toupper, and so on from scratch. All functions are templates that work with both CHAR and WCHAR. `Length()` just walks forward until it hits a null terminator, counting steps. `Compare()` supports case-insensitive comparison, which matters on Windows where DLL names are case-insensitive.

No dynamic allocation anywhere -- all operations work on fixed-size buffers, and the caller is responsible for sizing them correctly.

---

### How does float-to-string conversion work without printf?
**File:** `src/core/string/string.cc` **Line(s):** 7-70
**Type:** QUESTION
**Priority:** MEDIUM

FloatToStr is essentially a manual implementation of what `printf("%f", ...)` does. The algorithm step by step:

1. Handle negative numbers (prepend '-')
2. Add a rounding correction: 0.5 * 10^(-precision). For 2 decimal places, add 0.005.
3. Extract the integer part via cast: `intPart = (INT64)value`
4. Convert integer part to string with standard digit extraction
5. Add the decimal point
6. For each fractional digit: multiply by 10, grab the integer, subtract it off
   ```
   3.14159 -> intPart=3, frac=0.14159
   0.14159 * 10 = 1.4159 -> digit '1', remainder 0.4159
   0.4159 * 10 = 4.159  -> digit '4', remainder 0.159
   ... and so on
   ```
7. Trim trailing zeros

Straightforward once you lay it out, but honestly the rounding step is the part most people miss on first read.

---

### What is the StringFormatter and how does type erasure work?
**File:** `src/core/string/string_formatter.h` **Line(s):** 93-147
**Type:** QUESTION
**Priority:** MEDIUM

The formatter provides printf-style formatting without C-style variadic arguments. The problem with `va_list` is that it's type-unsafe -- if the format string doesn't match the arguments, you get silent corruption or crashes.

The solution uses C++ variadic templates to capture argument types at compile time. Each argument gets packed into an Argument struct that uses a union for storage (int, string, float, pointer) alongside a type discriminator tag. This is type erasure -- the concrete type is recorded once, then the formatter dispatches via switch/case.

One nice design detail: the writer callback pattern means the formatter never needs to allocate a buffer. It writes one character at a time through the callback, so the caller decides where output goes.

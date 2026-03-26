# 05 - Core Types and Primitives

Questions about the custom type system.

---

### Why define INT32, UINT64, etc. instead of using stdint.h?
**File:** `src/core/types/primitives.h` **Line(s):** 42-59
**Type:** QUESTION
**Priority:** HIGH

The obvious first question: "why not just `#include <cstdint>`?" Because `<cstdint>` is part of libc, and we're building with `-nostdlib` -- no standard headers at all. Instead, these types come from compiler built-in types that exist even without a C library:

  - `using INT32 = __INT32_TYPE__;`
  - `using USIZE = __SIZE_TYPE__;`

Clang predefines these and guarantees correct sizing on every platform. They're always available, no headers needed.

---

### What is USIZE and why is it important?
**File:** `src/core/types/primitives.h` **Line(s):** 110-117
**Type:** QUESTION
**Priority:** HIGH

USIZE shows up everywhere in this codebase. It's a pointer-sized unsigned integer -- 4 bytes on 32-bit systems (addressing up to 4 GB), 8 bytes on 64-bit (up to 16 EB). Think of it as the `-nostdlib` equivalent of `size_t`.

It's the go-to type for array sizes, byte counts, and pointer arithmetic. Why pointer-sized specifically? Because memory operations (memcpy and friends) process data in pointer-sized chunks for maximum throughput. Worth calling out in the book since readers will see it on practically every page.

---

### What is the TCHAR concept?
**File:** `src/core/types/primitives.h` **Line(s):** 91-92
**Type:** QUESTION
**Priority:** MEDIUM

```cpp
template <typename TChar>
concept TCHAR = __is_same_as(TChar, CHAR) || __is_same_as(TChar, WCHAR);
```

This is a C++20 concept -- essentially a constraint on template parameters. TCHAR says "this type must be either CHAR (8-bit) or WCHAR (wide)." If someone tries to instantiate a string function with `int` or `float`, they get a clean compile error rather than three pages of template noise.

Only these two types because every string operation in the project is either narrow (UTF-8) or wide (UTF-16). Nothing else makes sense here.

---

### Why is WCHAR different sizes on different platforms?
**File:** `src/core/types/primitives.h` **Line(s):** 65-80
**Type:** QUESTION
**Priority:** MEDIUM

WCHAR is 2 bytes on Windows/UEFI but 4 bytes on Linux/macOS. Historical divergence: Windows committed to 2-byte wide chars early (UCS-2, later UTF-16), while Unix/Linux went with 4-byte (UTF-32/UCS-4).

The practical consequence is that WCHAR is not portable for wire protocols. That's why the project also defines CHAR16 (always 2 bytes) for anything that crosses a network boundary. This size difference is a classic cross-platform bug source -- definitely needs emphasis.

---

### What is Result<T, Error> and why not use exceptions?
**File:** `src/core/types/result.h` **Line(s):** 35-65
**Type:** QUESTION
**Priority:** HIGH

Result shows up in virtually every function signature, so readers need to internalize it early. C++ exceptions (`try`/`catch`) require runtime support -- stack unwinding tables, type info -- all of which create data sections and violate the Golden Rule. Not an option.

Result<T, Error> is an "either" type. It holds a success value (T) or an error (Error), never both. Callers must check before accessing. The `[[nodiscard]]` attribute makes the compiler warn if someone ignores the return value entirely.

Typical usage pattern:
```cpp
auto result = File::Create("test.txt");
if (!result.IsOk()) {
    // handle error via result.GetError()
    return Result<void, Error>::Err(result);  // propagate
}
auto file = result.Value();  // safe to access
```

Very similar to Rust's `Result<T, E>` -- readers familiar with Rust will feel at home.

---

### What is the union inside Result and why does it save memory?
**File:** `src/core/types/result.h` **Line(s):** 54-65
**Type:** QUESTION
**Priority:** MEDIUM

A union makes all its members share the same memory. Since a Result is either a value or an error (never both simultaneously), `union { T value; Error error; }` lets them overlap. Total size becomes `max(sizeof(T), sizeof(Error)) + 1 byte` for the discriminator flag, versus `sizeof(T) + sizeof(Error) + 1` without the union. Compact and elegant.

---

### What is Span<T> and why not just use a pointer and size?
**File:** `src/core/types/span.h` **Line(s):** 69-150
**Type:** QUESTION
**Priority:** HIGH

Span bundles a pointer and its associated length into a single object. Instead of `void foo(const char* data, size_t length)` you write `void foo(Span<const CHAR> data)`. This eliminates an entire class of bugs -- you can't accidentally swap the pointer and size arguments, and the length always travels with the data.

Slicing operations (Subspan, First, Last) make it easy to pass around sub-regions safely. There's also a nice string literal convenience: `Span<const CHAR> s = "hello"` auto-sets the size to 5, excluding the null terminator.

The static extent variant `Span<T, N>` embeds the size at compile time, so the compiler can check bounds without any runtime cost.

---

### What is the Error type and how does error chaining work?
**File:** `src/core/types/error.h` **Line(s):** 63-150
**Type:** QUESTION
**Priority:** MEDIUM

There are 50+ error codes spread across modules (SocketCreate, TlsOpen, DnsResolve, etc.). Each error also carries a platform-specific code -- NTSTATUS on Windows, errno on POSIX.

The interesting part is error chaining. When function A fails because function B failed, B's error gets stored inside A's error. You end up with a full propagation path: "WebSocket handshake failed because TLS open failed because socket connect failed because DNS resolution failed." Essentially a stack trace, but for errors instead of function calls.

---

### Why does Result delete operator new?
**File:** `src/core/types/result.h` **Line(s):** (near destructor)
**Type:** QUESTION
**Priority:** MEDIUM

`void* operator new(USIZE) = delete;` appears on Result and several other types. This prevents heap allocation -- `new Result<int, Error>(...)` flat out won't compile. Everything must live on the stack. Heap allocation requires a memory allocator (malloc/free), and stack-only objects sidestep all the lifetime management headaches. When the function returns, cleanup is automatic.

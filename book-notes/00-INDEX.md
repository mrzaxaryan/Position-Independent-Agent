# Book Notes Index

These are reference notes for the book team. Each file covers a different area of the codebase with questions, observations, and suggested edits — all tied to specific files and line numbers so nothing is vague.

The target audience for the book is complete beginners with zero systems programming experience.

## Reference Files

| File | Topic | What It Covers |
|------|-------|----------------|
| [01-foundational-concepts.md](01-foundational-concepts.md) | Prerequisite Knowledge | Terms and concepts a rookie needs BEFORE reading any code |
| [02-project-overview.md](02-project-overview.md) | README and Architecture | Questions about the top-level README, architecture, and project goals |
| [03-build-system.md](03-build-system.md) | CMake and Build Pipeline | Questions about CMakeLists, presets, compiler flags, linker scripts |
| [04-entry-point-and-bootstrap.md](04-entry-point-and-bootstrap.md) | Entry Point | Questions about how the code starts executing |
| [05-core-types-and-primitives.md](05-core-types-and-primitives.md) | Type System | Questions about custom types, Result, Span, Error |
| [06-memory-and-strings.md](06-memory-and-strings.md) | Memory and Strings | Questions about custom memset/memcpy/memmove and string ops |
| [07-compiler-runtime.md](07-compiler-runtime.md) | Compiler Runtime | Questions about division helpers, ARM EABI, RISC-V builtins |
| [08-algorithms-and-math.md](08-algorithms-and-math.md) | DJB2, Base64, PRNG, Bit Ops | Questions about hashing, encoding, randomness |
| [09-kernel-and-syscalls.md](09-kernel-and-syscalls.md) | Kernel Interface | Questions about syscalls, PEB walking, NT Native API |
| [10-platform-abstraction.md](10-platform-abstraction.md) | Platform Layer | Questions about console, filesystem, memory allocation, sockets |
| [11-crypto.md](11-crypto.md) | Cryptography | Questions about ChaCha20, Poly1305, SHA-256, ECC, TLS 1.3 |
| [12-networking.md](12-networking.md) | Network Stack | Questions about DNS, HTTP, TLS, WebSocket |
| [13-beacon-and-commands.md](13-beacon-and-commands.md) | Beacon Layer | Questions about the command loop, handlers, screen capture |
| [14-pic-transform.md](14-pic-transform.md) | PIC Transform LLVM Pass | Questions about how string literals become stack immediates |
| [15-loaders.md](15-loaders.md) | Python and PowerShell Loaders | Questions about how the shellcode gets loaded and executed |
| [16-suggested-edits.md](16-suggested-edits.md) | Suggested Edits | Spots where existing docs/comments could be clearer |

## How to Use These Notes

Each entry follows this format:

```
### [SHORT TITLE]
**File:** `path/to/file.ext` **Line(s):** 42-50
**Type:** QUESTION | COMMENT | EDIT
**Priority:** HIGH | MEDIUM | LOW

[The actual question, comment, or suggested edit]
```

- **QUESTION** = Something a rookie would ask. Book should answer this.
- **COMMENT** = Observation about something non-obvious. Book should explain.
- **EDIT** = Suggestion to improve existing docs/comments for clarity.
- **HIGH** = A rookie literally cannot proceed without understanding this.
- **MEDIUM** = Important context that prevents confusion.
- **LOW** = Nice-to-have deeper understanding.

# 16 - Suggested Edits

Places where the existing documentation or code comments could be clearer. These are concrete suggestions, not vague complaints — each one says what to add and where.

---

### Add a "Prerequisites" section to the main README
**File:** `README.md` **Line(s):** 1-10
**Type:** EDIT
**Priority:** HIGH

The README jumps straight into features without telling people what they need to already know. A short prerequisite section would save a lot of confusion: basic C/C++ (pointers, structs, templates), a rough idea of what compilers and linkers do, what an OS kernel is, and basic TCP/IP networking. Without that baseline, the technical content is basically impenetrable.

---

### Expand the "Golden Rule" explanation in CONTRIBUTING.md
**File:** `CONTRIBUTING.md` **Line(s):** 168-175
**Type:** EDIT
**Priority:** HIGH

The Golden Rule says "binary must have ONLY .text section" but doesn't really sell the reader on WHY. Needs a clear 3-sentence explanation: if shellcode has multiple sections, the loader would need to parse a section table, map each section to separate memory regions, and fix up cross-section references — basically reimplementing an OS loader. A before/after example showing how a simple string literal creates a .rodata section would drive the point home.

---

### Add a glossary to the main README or CONTRIBUTING.md
**File:** `README.md` **Line(s):** (end of file)
**Type:** EDIT
**Priority:** HIGH

There's a wall of unexplained acronyms throughout the project: PEB, TEB, PIC, ASLR, GOT, PLT, IAT, CRT, RTTI, AEAD, ECDH, HKDF. Terms like syscall, shellcode, beacon, implant, C2, relay. Binary formats: ELF, PE, Mach-O, COFF. Compiler concepts: freestanding vs hosted, LTO. Low-level details: red zone, stack canary, calling convention.

A glossary with 1-2 sentence definitions for each of these would make the project dramatically more accessible.

---

### Add inline comments to entry_point.cc
**File:** `src/entry_point.cc` **Line(s):** 14-32
**Type:** EDIT
**Priority:** MEDIUM

Twenty lines of dense code with almost no comments. This is literally the first thing that runs. Specific spots that need annotations: line 14 (why the ENTRYPOINT macro exists — it places this function at offset 0), lines 20-21 (why context lives on the stack instead of as a global), lines 24-25 (why TPIDR_EL0/GS register is used — avoiding a .data section), and line 28 (what the watchdog timer is and why it gets disabled).

---

### Explain the "no STL" rule more clearly in CONTRIBUTING.md
**File:** `CONTRIBUTING.md` **Line(s):** 186-208
**Type:** EDIT
**Priority:** MEDIUM

The style guide says "no STL" but doesn't explain the reasoning. The STL (std::string, std::vector, std::map, etc.) requires exception handling, dynamic memory via operator new (which calls malloc), and RTTI. All of these generate data sections, which is fatal for position-independent code. The project provides its own replacements — Span instead of std::span, Result instead of std::optional/exceptions, and so on. That context needs to be right there in the style guide.

---

### Add a "How a build works" walkthrough to CONTRIBUTING.md
**File:** `CONTRIBUTING.md` **Line(s):** 38-42
**Type:** EDIT
**Priority:** MEDIUM

The contributing guide lists build commands but doesn't explain what actually happens when you run them. A step-by-step trace would help a lot:
```
cmake --preset linux-x86_64-release
  -> Toolchain.cmake: set up freestanding cross-compiler
  -> Options.cmake: validate architecture=x86_64, platform=linux
  -> Triples.cmake: set target triple to x86_64-unknown-linux-gnu
  -> CompilerFlags.cmake: assemble -nostdlib -fno-exceptions etc.
  -> Sources.cmake: find all .cc files, filter for linux platform
  -> PICTransform.cmake: find or build pic-transform pass
  -> Target.cmake: create executable target with all flags

cmake --build --preset linux-x86_64-release
  -> For each .cc file:
     -> Compile with pic-transform pass (eliminates data sections)
     -> Produces .o object file
  -> Link all .o files with custom linker script
  -> PostBuild: extract .text section -> .bin, verify PIC, base64 encode
```

---

### Add comments to the linker scripts
**File:** `cmake/data/linker.i386.ld` **Line(s):** 1-20
**Type:** EDIT
**Priority:** LOW

The linker scripts have zero comments. They should explain what each SECTIONS entry does, why .rodata gets merged into .text, what /DISCARD/ removes, and what the section patterns like `.text.*` and `.rodata.cst4` actually match.

---

### Clarify the README's architecture diagram
**File:** `README.md` **Line(s):** 95-127
**Type:** EDIT
**Priority:** MEDIUM

The ASCII art architecture diagram is dense and hard to parse. It needs labels showing which directory maps to which layer, a note about dependency direction (upper depends on lower, never reverse), and a concrete example like: "Beacon (src/beacon/) calls Lib (src/lib/) which calls Platform (src/platform/) which calls Core (src/core/)."

---

### Add "why this matters" notes to each platform module README
**File:** `src/platform/socket/README.md` **Line(s):** (throughout)
**Type:** EDIT
**Priority:** LOW

The platform READMEs explain HOW things work but rarely WHY. Every major design decision should have a brief justification: AFD instead of Winsock because Winsock requires importing ws2_32.dll. Raw X11 protocol instead of libX11 because you can't link shared libraries. Fork before CoreGraphics for crash isolation. These "why" notes are what turn documentation from a reference into something you can actually learn from.

---

### Add a "Common Mistakes" section to CONTRIBUTING.md
**File:** `CONTRIBUTING.md` **Line(s):** 360-367
**Type:** EDIT
**Priority:** MEDIUM

The "Common Pitfalls" section is too brief. Real examples would make it actionable:
- Global array? Put it in a function as a local instead.
- `switch` on a string? Use DJB2 hash comparison.
- Returning a pointer to a stack string? Allocate with `new[]` and transfer ownership.
- Forgot `#pragma pack` on a wire struct? Always pack protocol structures.
- Used WCHAR in wire format? Always use CHAR16.

---

### Document the test suite more thoroughly
**File:** `tests/` (directory)
**Type:** EDIT
**Priority:** LOW

The test directory barely has any documentation. Needs to cover: how to run tests (which presets, which flags), what's being tested (TLS handshake, crypto correctness, etc.), how to add a new test, and whether tests run on real hardware or QEMU emulation (the CMakePresets.json has QEMU test presets, so this is clearly relevant).

---

### Add a "Reading Order" guide for newcomers
**File:** `README.md` or `CONTRIBUTING.md` **Line(s):** (new section)
**Type:** EDIT
**Priority:** HIGH

Someone new to the project has no idea where to start. A suggested reading order would go a long way:
1. README.md (project overview and goals)
2. src/README.md (architecture overview)
3. CONTRIBUTING.md (how it's built and structured)
4. src/entry_point.cc (how execution starts)
5. src/core/ (bottom-up: types, memory, strings)
6. src/platform/kernel/ (how it talks to the OS)
7. src/platform/ (cross-platform abstractions)
8. src/lib/crypto/ (cryptographic primitives)
9. src/lib/network/ (networking stack bottom-up: socket, TLS, HTTP, WS)
10. src/beacon/ (the agent itself)
11. tools/pic-transform/ (the LLVM pass)
12. cmake/ (build system)

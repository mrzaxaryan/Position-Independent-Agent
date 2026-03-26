# 03 - Build System (CMake, Compiler Flags, Linker Scripts)

Notes on how the project gets compiled and linked. The build system is doing
a LOT of heavy lifting here — arguably more than the application code itself.

---

### What is CMake and why not just use a Makefile?
**File:** `CMakeLists.txt` **Line(s):** 1-51
**Type:** QUESTION
**Priority:** MEDIUM

CMake is a build system generator — it produces Makefiles or Ninja build files
rather than being a build system itself. It handles cross-platform build
configuration, which matters a lot here: this project has 60+ build
configurations (8 platforms x 7 architectures x debug/release). Managing that
matrix with raw Makefiles would be brutal. CMake presets in `CMakePresets.json`
define the full configuration matrix so you can just pick one and build.

---

### What does "include_guard(GLOBAL)" do?
**File:** `cmake/Common.cmake` **Line(s):** 8
**Type:** QUESTION
**Priority:** LOW

Same concept as `#pragma once` in C++ headers — prevents the file from being
processed more than once if it gets included from multiple places. Brief mention
is enough.

---

### Why are there so many compiler flags?
**File:** `cmake/CompilerFlags.cmake` **Line(s):** 10-29
**Type:** QUESTION
**Priority:** HIGH

Fifteen-plus flags, each there for a reason. A table works well here:

| Flag | What It Does | Why It's Needed |
|------|-------------|-----------------|
| `-std=c++23` | Use C++23 standard | consteval, concepts, modern features |
| `-nostdlib` | No C standard library | Can't depend on libc |
| `-fno-exceptions` | Disable try/catch | Exception tables land in data sections |
| `-fno-rtti` | Disable dynamic_cast/typeid | Type info tables land in data sections |
| `-fno-builtin` | Don't replace memcpy etc | Builtins may reference external symbols |
| `-ffunction-sections` | Each function gets own section | Enables dead code elimination |
| `-fno-asynchronous-unwind-tables` | No .eh_frame | Unwind tables land in data sections |
| `-fno-addrsig` | No address-significance table | Extra section we don't want |
| `-Wno-main-return-type` | Suppress warning | Entry point returns void, not int |

Every single one of these flags exists to either (a) prevent extra sections from
being generated or (b) survive without a runtime. If readers remember nothing
else: the compiler wants to put things in data sections, and these flags are the
first line of defense.

---

### What is "-mno-red-zone" and why is it on some platforms?
**File:** `cmake/platforms/Windows.cmake` **Line(s):** 15
**Type:** QUESTION
**Priority:** MEDIUM

Shows up on x86_64 Windows, Linux, FreeBSD, macOS, and Solaris.

The "red zone" is 128 bytes below the stack pointer (RSP) on x86_64. Leaf
functions — functions that don't call other functions — can use this space
without adjusting RSP. It's a performance optimization from the System V ABI.
The problem is that signal handlers and interrupts can clobber this area. In
shellcode, you can't guarantee the red zone won't be stepped on, so
`-mno-red-zone` disables the optimization entirely. Better safe than segfaulted.

---

### What is Link-Time Optimization (LTO) and why does it matter here?
**File:** `cmake/CompilerFlags.cmake` **Line(s):** 42-62
**Type:** QUESTION
**Priority:** MEDIUM

Release builds use `-flto=full`. Normally each `.cc` file is compiled
independently, then linked. With LTO, the compiler defers optimization to link
time — the linker sees ALL code at once and can optimize across file boundaries.
Benefits: unused functions get removed, cross-file inlining kicks in, the binary
shrinks.

But there's a catch. LTO can generate "constant pools" — unnamed data blocks
placed before your code. If a constant pool lands before `entry_point`, the
entry-point-at-offset-0 guarantee breaks. The project has specific workarounds:
`entry_point.cc` gets compiled without LTO on macOS/iOS, and custom linker
scripts merge any generated `.rodata` back into `.text`.

---

### What are "function order files"?
**File:** `cmake/data/function.order.linux` **Line(s):** 1-5
**Type:** QUESTION
**Priority:** HIGH

This one's critical. When you load shellcode, you jump to byte 0 — the very
start of the blob. If `entry_point` isn't at offset 0, you jump into the middle
of some other function and crash.

The linker normally orders functions however it pleases. Function order files
force a specific layout: `entry_point` first, then everything else. Different
platforms use different mechanisms to achieve this:
- Linux: `--symbol-ordering-file`
- Windows: `/ORDER:@file`
- macOS: `-order_file`

Same goal, three different flags. The build system abstracts this away.

---

### What do the linker scripts actually do?
**File:** `cmake/data/linker.i386.ld` **Line(s):** 1-20
**Type:** QUESTION
**Priority:** MEDIUM

Someone seeing `SECTIONS { .text : { *(.text .text.* .rodata .rodata.*) } }`
for the first time won't parse that syntax at all.

In plain English: this script tells the linker "take everything from `.text`
sections AND everything from `.rodata` sections and merge them all into one
output section called `.text`." Why? Some architectures generate read-only data
(`.rodata`) for floating-point constants or lookup tables that LTO couldn't
eliminate. By merging `.rodata` into `.text`, the final binary still has only
one section. The `DISCARD` directive throws away everything else — debug info,
symbol tables, the works.

---

### What is the PIC verification step?
**File:** `cmake/scripts/VerifyPICMode.cmake` **Line(s):** 21-172
**Type:** QUESTION
**Priority:** HIGH

After building, the build system verifies the binary is truly position-
independent. This is the safety net. It checks four things:

1. Entry point is at offset 0 (first function in `.text`)
2. No `.rodata`, `.rdata`, `.data`, or `.bss` sections exist
3. No `.got` or `.plt` sections (dynamic linking stubs)
4. On macOS: only `__TEXT,__text` exists in the `__TEXT` segment

If any check fails, the build errors out. Even if you accidentally write code
that generates data sections, the build will catch it. This turns a subtle
runtime bug (shellcode crashes when loaded at a random address) into a loud
build-time error. Invaluable.

---

### What is the pic-transform build step?
**File:** `cmake/PICTransform.cmake` **Line(s):** 20-182
**Type:** QUESTION
**Priority:** HIGH

This is the most unique part of the entire build system. `pic-transform` is a
custom LLVM compiler pass — a plugin that runs during compilation and transforms
the code's intermediate representation (LLVM IR).

What it does: finds string literals, float constants, and arrays that would
normally land in `.rodata` and converts them to stack-based immediate stores.
So `"Hello"` becomes machine instructions that push `0x6F6C6C6548` onto the
stack — the bytes of "Hello" encoded as an integer, constructed at runtime
instead of stored as data.

It runs in two modes: as a plugin inside the compiler, or standalone against
bitcode files. Without this pass, most real-world C++ code would generate data
sections and break position-independence. It's the key innovation that makes the
whole project viable.

---

### What is the ExtractBinary post-build step?
**File:** `cmake/scripts/ExtractBinary.cmake` **Line(s):** 36-168
**Type:** QUESTION
**Priority:** MEDIUM

After building, the `.text` section gets extracted into a standalone `.bin`
file. The compiler produces a full ELF/PE/Mach-O executable with headers,
section tables, and all the usual metadata. We only want the raw machine code.
`llvm-objcopy --dump-section=.text=output.bin` extracts just the code bytes.

That `.bin` file IS the shellcode — it's what gets loaded and executed. A
base64-encoded `.b64.txt` version also gets created for easy transport in text
channels.

---

### What is OSABI patching?
**File:** `cmake/scripts/PatchElfOSABI.cmake` **Line(s):** 24-44
**Type:** QUESTION
**Priority:** LOW

Minor detail but it'll confuse someone eventually. ELF files have a header
with an "OS/ABI" field at byte offset 7. The default value 0 (`ELFOSABI_NONE`)
works fine for Linux. FreeBSD's kernel requires value 9 (`ELFOSABI_FREEBSD`) or
it refuses to load the binary. Solaris requires value 6 (`ELFOSABI_SOLARIS`).
The script patches this single byte after linking. One byte, but without it the
kernel won't even look at your binary.

---

### Why does macOS ARM64 need special handling?
**File:** `cmake/platforms/macOS.cmake` **Line(s):** 81-143
**Type:** COMMENT
**Priority:** MEDIUM

macOS ARM64 is the most complex platform configuration in the project, and it's
worth understanding why.

Apple's ARM64 kernel flat-out refuses to run static binaries — it requires dyld
(the dynamic linker) even for single-section shellcode. LTO can place unnamed
constant pools before `entry_point`, so `entry_point.cc` is compiled WITHOUT LTO
to guarantee it stays at offset 0. The `__text` section must be aligned to a 4KB
page boundary because ARM64 `ADRP` instructions compute page-relative addresses.
Weak symbols from template instantiation would generate lazy-binding stubs,
breaking PIC — hidden visibility prevents this.

Every one of these workarounds was discovered through painful debugging. They're
the kind of platform-specific landmines that don't show up in documentation.

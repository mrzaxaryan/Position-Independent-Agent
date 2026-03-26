# 01 - Foundational Concepts

Prerequisite knowledge readers need before any of the actual code makes sense.
This should probably be an introductory chapter or an appendix — either way,
don't skip it.

---

### What is "position-independent code" and why does it matter?
**File:** `README.md` **Line(s):** 29-31
**Type:** QUESTION
**Priority:** HIGH

The README says the binary "executes from arbitrary memory without relying on
DLLs, import tables, or runtime initialization." That's a lot of jargon for
page one.

Start with what a "normal" binary is. It gets loaded at an expected address,
links against shared libraries, and has an OS-provided runtime that sets up
stdin/stdout, parses arguments, etc. "Position" here means memory address — the
code assumes nothing about where it landed. Why would anyone want that? Because
shellcode gets injected, reflectively loaded, or dropped into memory by
some other mechanism, and you can't predict the address.

Good analogy: a normal program is like a house with a fixed address and plumbing
connections; shellcode is a self-contained camping tent you can pitch anywhere.

---

### What is a "section" in a binary file?
**File:** `CONTRIBUTING.md` **Line(s):** 168-175
**Type:** QUESTION
**Priority:** HIGH

The "Golden Rule" says the binary must have ONLY a `.text` section. That won't
mean anything to someone who hasn't cracked open a binary before.

Binary files (ELF, PE, Mach-O) are divided into sections: `.text` is executable
code (your actual instructions), `.data` holds initialized global variables,
`.rodata` / `.rdata` is read-only data (string literals, lookup tables,
constants), and `.bss` is uninitialized globals (zeroed at startup).

Why does having ONLY `.text` matter? A loader can just copy one blob of bytes
into memory and jump to it. Multiple sections would need a proper loader that
maps each section to the right place with the right permissions. One section =
dumb copy + jump. That's the whole trick.

---

### What is a "syscall"?
**File:** `README.md` **Line(s):** 33
**Type:** QUESTION
**Priority:** HIGH

The README mentions "direct syscalls" like it's obvious. It isn't.

Programs talk to the OS kernel to do anything useful — read files, open sockets,
allocate memory. Normally you call a C library function like `write()`, which
internally triggers the syscall. A "direct syscall" skips the C library and
talks to the kernel directly via a special CPU instruction (`syscall` on x86_64,
`svc #0` on ARM64, etc.).

This matters here because there is no C library. We're in -nostdlib mode, so
direct syscalls are the only option. Worth calling out that each OS and CPU
architecture has different syscall numbers for the same operation — "write" is
syscall 1 on x86_64 Linux but syscall 4 on i386 Linux.

---

### What is the difference between an OS, a kernel, and a C library?
**File:** `src/README.md` **Line(s):** 8-27
**Type:** QUESTION
**Priority:** HIGH

The layered architecture separates "kernel" from "platform" from "core," and
those distinctions matter.

The kernel is the actual OS core — Linux kernel, Windows NT kernel, XNU on
macOS. The C library (glibc, musl, msvcrt) is a user-space wrapper that gives
you `printf`, `malloc`, etc. by calling into the kernel on your behalf. Most
programs depend heavily on the C library. This project bypasses it entirely.
"Platform" in this codebase refers to the OS-specific layer that wraps raw
syscalls into a friendlier internal API.

---

### What is a linker and what does it do?
**File:** `cmake/data/linker.i386.ld` **Line(s):** 1-20
**Type:** QUESTION
**Priority:** HIGH

There are multiple linker scripts in `cmake/data/`. Readers who've only ever
hit "Build" in an IDE won't know what a linker is.

The compiler turns `.cc` files into `.o` object files — machine code plus
metadata. The linker combines all those `.o` files into one executable and
decides where each section lands in the final binary. A "linker script" is a
recipe that overrides the linker's default layout. In this project, linker
scripts are used to merge `.rodata` into `.text` so there's only one section
in the output. That's a critical part of the whole position-independence story.

---

### What is cross-compilation?
**File:** `cmake/Toolchain.cmake` **Line(s):** 7
**Type:** QUESTION
**Priority:** MEDIUM

The toolchain sets `CMAKE_SYSTEM_NAME = Generic`.

Normally you compile code on the same type of machine that'll run it.
Cross-compilation means compiling on one machine (say, your x86_64 Linux laptop)
for a completely different target (ARM Android phone, RISC-V dev board,
whatever). "Generic" tells CMake "we're not targeting any specific OS" — a
freestanding environment. This project cross-compiles for 8 different platforms
from a single host machine.

---

### What is the difference between x86, x86_64, ARM, RISC-V, and MIPS?
**File:** `CMakePresets.json` **Line(s):** 48-375
**Type:** QUESTION
**Priority:** MEDIUM

The presets define builds for i386, x86_64, aarch64, armv7a, riscv32, riscv64,
and mips64. Quick rundown:

- **x86 (i386)** — 32-bit Intel/AMD. Older PCs, still everywhere in legacy systems.
- **x86_64** — 64-bit Intel/AMD. The modern desktop/server workhorse.
- **aarch64 (ARM64)** — 64-bit ARM. Phones, Apple Silicon Macs, Raspberry Pi 4+.
- **armv7a** — 32-bit ARM. Older phones, tons of embedded devices.
- **riscv32/riscv64** — RISC-V. Open-source ISA with a growing ecosystem.
- **mips64** — MIPS. Routers, embedded gear, some servers.

Each architecture has different registers, instruction formats, and syscall
conventions. Every one of those differences creates work in this project.

---

### What is LLVM/Clang and why not GCC?
**File:** `CONTRIBUTING.md` **Line(s):** 46-72
**Type:** QUESTION
**Priority:** MEDIUM

The toolchain requires LLVM 22+ / Clang. This isn't arbitrary preference — the
project uses a custom LLVM pass (`pic-transform`) that modifies the compiler's
intermediate representation. LLVM passes can only run inside the LLVM/Clang
pipeline; GCC has a completely different internal architecture. LLVM 22+ is
required because the pass depends on specific IR features introduced in that
version.

---

### What is "freestanding" code?
**File:** `cmake/CompilerFlags.cmake` **Line(s):** 10-29
**Type:** QUESTION
**Priority:** HIGH

The flags include `-nostdlib`, `-fno-exceptions`, `-fno-rtti`, `-fno-builtin`.
This is what "freestanding" means in practice:

- **"Hosted"** = normal program that can use `printf`, `malloc`, `std::string`, etc.
- **"Freestanding"** = code that has NO runtime support whatsoever.

The flags break down like this: `-nostdlib` means don't link the C standard
library. `-fno-exceptions` disables C++ try/catch (which requires runtime
support and generates data sections). `-fno-rtti` disables `dynamic_cast` and
`typeid` (which need type info tables — also data sections). `-fno-builtin`
prevents the compiler from replacing functions like `memcpy` with intrinsics
that might reference external symbols.

This is the same environment you'd be in writing an OS kernel or a bootloader.

---

### What is a "target triple"?
**File:** `cmake/Triples.cmake` **Line(s):** 10-37
**Type:** QUESTION
**Priority:** MEDIUM

The file defines strings like `x86_64-unknown-linux-gnu`. A target triple tells
the compiler what machine you're building for. The format is
`<architecture>-<vendor>-<os>-<environment>`. So `x86_64-unknown-linux-gnu`
means 64-bit Intel, unknown vendor, Linux, GNU libc ABI. And
`arm64-apple-macos11` means ARM64, Apple, macOS 11+.

The compiler uses this to select instruction encoding, calling conventions, and
the object file format (ELF vs PE vs Mach-O). Get it wrong and you get binaries
that silently do the wrong thing.

---

### What are ELF, PE/COFF, and Mach-O?
**File:** `cmake/platforms/Linux.cmake` **Line(s):** 22-28
**Type:** QUESTION
**Priority:** MEDIUM

Different platforms use different binary formats, which is why the linker flags
vary per-platform.

ELF (Executable and Linkable Format) covers Linux, FreeBSD, Solaris, and
Android. PE/COFF (Portable Executable) is Windows and UEFI. Mach-O is macOS
and iOS. Each format has its own section naming conventions — ELF uses `.text`,
`.rodata`, `.data`; PE uses `.text`, `.rdata`, `.data`; Mach-O uses
`__TEXT,__text`, `__DATA,__data`. The build system handles all these differences
per-platform so the rest of the code doesn't have to care.

---

### What is UEFI and why is it a target?
**File:** `src/entry_point.cc` **Line(s):** 14-29
**Type:** QUESTION
**Priority:** MEDIUM

UEFI gets special handling throughout the codebase.

UEFI (Unified Extensible Firmware Interface) is the firmware that runs before
your OS boots — it replaced the old BIOS. UEFI applications run in a pre-OS
environment with their own API, which means the agent can run even before
Windows or Linux loads. UEFI uses PE/COFF format but with a special
`EFI_APPLICATION` subsystem, and it provides its own networking, filesystem,
and display protocols. It's a genuinely different execution environment from
everything else the project targets.

---

### What does "no standard library" actually mean in practice?
**File:** `src/core/README.md` **Line(s):** 74-102
**Type:** QUESTION
**Priority:** HIGH

The core layer reimplements `memset`, `memcpy`, `strlen`, and more. "Why not
just use the ones that already exist?" Because they don't exist here.

In a normal program, functions like `memcpy` come from the C library (libc),
which is a shared library (`.so`/`.dll`) loaded by the OS. Position-independent
shellcode can't depend on shared libraries being present or loaded at known
addresses. So this project implements everything from scratch: memory operations,
string operations, math, even 64-bit division on 32-bit CPUs (since that's
normally a compiler runtime helper).

Honestly, this is one of the hardest aspects of the whole project and a major
learning opportunity. Reimplementing the things you take for granted teaches
you what the standard library actually does.

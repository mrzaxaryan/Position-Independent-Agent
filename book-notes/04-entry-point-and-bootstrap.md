# 04 - Entry Point and Bootstrap

Notes on how the code starts executing. This is where rubber meets road —
the very first instructions that run when the shellcode is loaded.

---

### Why is the entry point not `main()`?
**File:** `src/entry_point.cc` **Line(s):** 14-18
**Type:** QUESTION
**Priority:** HIGH

Every C/C++ program has `main()`. This one has `entry_point()`.

Here's the deal: `main()` is called by the C runtime (CRT) startup code. The
CRT initializes global variables, sets up stdin/stdout, parses command-line
arguments, calls constructors for global objects — all before your `main` ever
runs. Since this project uses `-nostdlib`, there IS no CRT. The OS or shellcode
loader jumps directly to the first byte of code with no preamble. So we define
our own entry point with a custom name and use the `ENTRYPOINT` macro to apply
the right compiler attributes.

---

### What is the ENTRYPOINT macro doing?
**File:** `src/core/compiler/compiler.h` **Line(s):** 99-103
**Type:** QUESTION
**Priority:** HIGH

The macro applies different attributes depending on architecture:

`__attribute__((noinline))` — don't inline this function. It must be a real,
standalone function the linker can place at offset 0. `__attribute__((used))` —
don't remove it even if nothing appears to call it. Without this, the linker
would see no callers and strip it as dead code. `__attribute__((force_align_arg_pointer))`
(x86_64 POSIX only) — fixes stack alignment at entry, which deserves its own
entry below.

---

### Why does x86_64 need stack alignment fixing?
**File:** `src/core/compiler/compiler.h` **Line(s):** 100
**Type:** QUESTION
**Priority:** HIGH

This is subtle and critical. Getting it wrong means random segfaults in SSE
instructions that nobody can explain.

The x86_64 ABI requires RSP to be 16-byte aligned AFTER a `CALL` instruction.
`CALL` pushes an 8-byte return address, so RSP must be 16-byte aligned BEFORE
the `CALL` (i.e., `16n + 0`), making it `16n - 8` after. But when shellcode
starts, there was no `CALL`. The loader just jumped to the code. RSP could be
16-byte aligned (`16n + 0`) instead of the expected `16n - 8`.

If RSP is wrong, any SSE instruction that requires 16-byte alignment will
segfault. `force_align_arg_pointer` tells the compiler to emit a prologue that
realigns RSP regardless of its incoming value. One attribute saves hours of
debugging.

---

### Why does UEFI get a different entry point signature?
**File:** `src/entry_point.cc` **Line(s):** 14-18
**Type:** QUESTION
**Priority:** MEDIUM

The UEFI entry point takes `(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)`
while all others take `(void)`.

UEFI firmware calls the entry point and passes two arguments: `ImageHandle` is
an opaque handle identifying this loaded application, and `SystemTable` is a
pointer to UEFI's master API table — boot services, runtime services, console
I/O, everything. Without these, the UEFI application can't do anything at all.
No output, no memory allocation, no network access.

Other platforms bootstrap differently. Linux, macOS, and FreeBSD provide nothing
at the entry point — you make raw syscalls. Windows is its own thing: you find
`ntdll.dll` by walking the PEB (Process Environment Block). Each platform has a
completely different "first 20 lines of code" problem.

---

### What is the EFI_CONTEXT and why store it in a CPU register?
**File:** `src/entry_point.cc` **Line(s):** 20-26
**Type:** QUESTION
**Priority:** HIGH

This is a key position-independence technique and honestly the hardest part to
explain cleanly.

The UEFI system table pointer must be accessible throughout the entire program.
Normally you'd toss it in a global variable. But global variables create `.data`
sections, which violates the Golden Rule. So instead, the pointer gets stored in
a CPU register that's preserved across function calls: `TPIDR_EL0` on ARM64,
the `GS` segment register on x86_64. These registers are normally used by the
OS for thread-local storage, but since we're freestanding, nobody else is using
them.

The actual `EFI_CONTEXT` struct is allocated on the stack, and a pointer to it
goes into the register. Stack allocation, register-based access, no data
sections. Problem solved.

---

### What does the "start()" forward declaration mean?
**File:** `src/entry_point.cc` **Line(s):** 9
**Type:** QUESTION
**Priority:** LOW

`extern void start();` just tells the compiler "this function exists somewhere
else." It's defined in `src/beacon/main.cc`. The entry point calls it after
bootstrap is done. Clean separation: platform-specific bootstrap lives in
`entry_point.cc`, actual agent logic lives in `start()`.

---

### Why disable the UEFI watchdog timer?
**File:** `src/entry_point.cc` **Line(s):** 28
**Type:** QUESTION
**Priority:** LOW

`SetWatchdogTimer(0, 0, 0, nullptr)` disables the UEFI watchdog.

UEFI firmware ships with a watchdog timer, typically set to 5 minutes. If an
application runs longer than the timeout, the firmware assumes it's hung and
resets the entire system. Since the agent runs an infinite command loop, the
watchdog has to go — otherwise you'd get a hard reboot every 5 minutes. Not
exactly subtle.

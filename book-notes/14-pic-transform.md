# 14 - PIC Transform LLVM Pass

The custom compiler pass that eliminates data sections. This one's gonna trip people up if they've never worked with compiler internals, so it needs careful treatment.

---

### What is an LLVM pass?
**File:** `tools/pic-transform/` (directory)
**Type:** QUESTION
**Priority:** HIGH

Readers need to understand the compilation pipeline before any of this makes sense. When clang processes C++ code, it goes through stages: parsing the source into an AST, lowering that to LLVM Intermediate Representation (IR), running a series of optimization passes that transform the IR, and finally emitting machine code from the optimized IR.

An LLVM pass is one of those transformations in the middle. pic-transform is a custom pass that gets inserted into the optimization pipeline. It walks the IR looking for anything that would create a data section — string literals, float constants, arrays — and rewrites those as stack-based operations instead.

---

### What exactly does pic-transform transform?
**File:** `tools/pic-transform/README.md` **Line(s):** 9-13
**Type:** QUESTION
**Priority:** HIGH

Concrete before/after examples are essential here.

**String literals:**
```
// Before (C++ source):
const char* msg = "Hello";

// LLVM IR before pic-transform:
@.str = private unnamed_addr constant [6 x i8] c"Hello\00"
  ; This constant would land in .rodata

// After pic-transform (conceptual assembly):
sub rsp, 8
mov qword ptr [rsp], 0x006F6C6C6548  ; "Hello\0" as integer
lea rax, [rsp]                         ; pointer to stack string
  ; String is now on the stack, no .rodata needed
```

**Floating-point constants:**
```
// Before:
double pi = 3.14159;

// Without transform: 3.14159 stored as 8 bytes in .rodata
// After: loaded as integer bit pattern, moved to FP register
mov rax, 0x400921FB54442D18  ; IEEE-754 bits for 3.14159
movq xmm0, rax
```

**Arrays:**
```
// Before:
int table[] = {1, 2, 3, 4};

// After: four immediate stores to stack
mov dword ptr [rsp], 1
mov dword ptr [rsp+4], 2
mov dword ptr [rsp+8], 3
mov dword ptr [rsp+12], 4
```

---

### What are "register barriers" and why are they needed?
**File:** `tools/pic-transform/README.md` **Line(s):** 88-96
**Type:** QUESTION
**Priority:** MEDIUM

Here's the problem: pic-transform runs early in the optimization pipeline. Later optimization passes see those stack values and think "hey, these are constants — I should hoist them to .rodata for efficiency." Which completely undoes the work.

Register barriers prevent this. They're inline assembly statements that force a value through a register, making it opaque to the optimizer:
```
// Without barrier:
int x = 42;  // optimizer sees constant 42, may hoist to .rodata

// With barrier:
int x;
asm volatile("" : "=r"(x) : "0"(42));  // optimizer can't see through this
```
The barrier generates zero actual instructions — it's purely a hint that tells the optimizer "you don't know what this value is, leave it alone."

---

### What is the difference between plugin mode and standalone mode?
**File:** `cmake/PICTransform.cmake` **Line(s):** 61-86
**Type:** QUESTION
**Priority:** MEDIUM

pic-transform can run two ways. **Plugin mode** loads it into the compiler as a shared library via `-fpass-plugin=/path/to/pic-transform.so`. It runs automatically during compilation — one step, simple. **Standalone mode** is a separate binary that transforms LLVM bitcode files in a three-step pipeline: compile to bitcode (.bc), run the transform, then compile to object (.o). You'd use standalone when the plugin can't load, typically due to mismatched LLVM versions.

The build system auto-detects which mode is available and picks accordingly.

---

### Does pic-transform handle ALL data section sources?
**File:** `tools/pic-transform/README.md` **Line(s):** (transformation list)
**Type:** QUESTION
**Priority:** MEDIUM

No. No single tool catches everything. It's a layered defense:

1. **pic-transform** handles string literals, float constants, and some arrays
2. **Compiler flags** knock out other sources: `-fno-exceptions` (no .eh_frame), `-fno-rtti` (no typeinfo), `-fno-asynchronous-unwind-tables`
3. **Linker scripts** merge any surviving .rodata into .text
4. **Code discipline** — `optnone` attribute on sensitive functions, NOINLINE on constant-filling functions
5. **VerifyPICMode** runs post-build and catches anything that slipped through all of the above

No silver bullet. Just layers.

---

### What if someone writes code that pic-transform can't handle?
**File:** `cmake/scripts/VerifyPICMode.cmake` **Line(s):** 95-172
**Type:** QUESTION
**Priority:** MEDIUM

VerifyPICMode scans the linker map after every build. If it finds a .rodata, .data, .bss, .got, or .plt section, the build fails with a clear error. The developer then has to track it down: check the linker map for which object file contributed the section, look at the disassembly to find the offending function, and fix the source (use `optnone`, move data to the stack, etc.). Think of it as a regression test that runs on every build — you can't accidentally ship code that breaks the single-section constraint.

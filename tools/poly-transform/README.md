# poly-transform

LLVM pass that constrains instruction selection to a random per-build subset of the target's instruction set. Produces binaries where every build uses a different instruction vocabulary, making static signature detection effectively impossible.

Designed for position-independent shellcode — companion to [pic-transform](../pic-transform/) which eliminates data sections.

## What it does

| Normal compilation | After poly-transform |
|---|---|
| `add %eax, %ecx` | `neg %ecx; sub %eax, %ecx` (ADD replaced by SUB) |
| `xor %eax, %ecx` | `or + and + not` sequence (XOR replaced by De Morgan's) |
| `shl %eax, 3` | `imul %eax, 8` (shift replaced by multiply) |

Each build randomly selects 10 instructions from ~50 base opcodes. With billions of valid combinations per architecture, no two builds share the same instruction fingerprint.

## How it works

Operates on LLVM IR with algebraic transformations:

1. **ADD ↔ SUB** — `add a, b` → `sub a, (sub 0, b)` (negate + subtract)
2. **XOR → AND + OR** — De Morgan's: `a ^ b = (a | b) & ~(a & b)`
3. **AND → NOT + OR** — De Morgan's: `a & b = ~(~a | ~b)`
4. **OR → NOT + AND** — De Morgan's: `a | b = ~(~a & ~b)`
5. **SHL ↔ MUL** — shift by constant → multiply by power of 2
6. **CMP → SUB** — comparison via subtraction (for eq/ne)

The compiler then handles register allocation, instruction encoding, and branch offsets — all correctness concerns are automatic.

## Pipeline

```
source.cc
  → LLVM IR
  → pic-transform   (eliminate data sections)
  → poly-transform  (constrain instruction selection)    ← THIS PASS
  → backend code generation
  → link
  → output.bin (polymorphic shellcode)
```

Both passes run at compile time. The output.bin already contains the polymorphically transformed instructions — no post-processing needed.

## Usage

### As a pass plugin (Linux — preferred)

```bash
POLY_TRANSFORM_SEED=0xDEADBEEF POLY_TRANSFORM_COUNT=10 \
  clang++ -fpass-plugin=./PolyTransform.so -O2 input.cpp -o output
```

### As a standalone tool

```bash
clang++ -emit-llvm -c -O2 input.cpp -o input.bc
poly-transform --seed=0xDEADBEEF --count=10 input.bc -o output.bc
clang++ output.bc -o output
```

## Building

### Requirements

- LLVM 20+ development headers and libraries
- CMake 3.20+

### Linux

```bash
cmake -B build -DLLVM_DIR=/usr/lib/llvm-22/lib/cmake/llvm
cmake --build build
```

### macOS

```bash
cmake -B build -DLLVM_DIR=$(brew --prefix llvm)/lib/cmake/llvm
cmake --build build
```

## Build Integration

The tool integrates automatically via `cmake/PolyTransform.cmake`:
- Reuses the LLVM installation found by pic-transform
- Builds as plugin (preferred) or standalone tool
- Seed is auto-generated from build date via MD5
- Shown in the build summary alongside pic-transform

## License

Same as the parent project.

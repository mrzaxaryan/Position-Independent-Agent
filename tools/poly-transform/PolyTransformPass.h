//===-- PolyTransformPass.h - Polymorphic instruction selection pass -------===//
//
// LLVM Module pass that constrains instruction selection to a random subset
// of the target's instruction set.  Each build produces a different random
// N-instruction vocabulary (seeded from __DATE__), making static signature
// detection effectively impossible.
//
// Works by transforming LLVM IR operations into equivalent sequences that
// the backend naturally lowers to the allowed instruction subset:
//   - ADD ↔ SUB (negate operand)
//   - XOR ↔ AND+OR (De Morgan's)
//   - INC/DEC → ADD/SUB 1
//   - etc.
//
// The compiler then handles register allocation, branch encoding, and all
// correctness concerns automatically.
//
//===----------------------------------------------------------------------===//

#ifndef POLY_TRANSFORM_PASS_H
#define POLY_TRANSFORM_PASS_H

#include "llvm/IR/PassManager.h"
#include <set>
#include <string>

namespace llvm {

class PolyTransformPass : public PassInfoMixin<PolyTransformPass> {
public:
  /// Allowed base mnemonic set (e.g. {"mov", "sub", "xor", "call", ...}).
  /// If empty, the pass is a no-op.
  std::set<std::string> AllowedSet;

  /// Seed for random instruction set generation (0 = use default).
  uint64_t Seed = 0;

  /// Number of instructions to select.
  int Count = 10;

  /// Target architecture name (e.g. "x86_64", "i386").
  std::string Arch;

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
  static bool isRequired() { return true; }
};

} // namespace llvm

#endif // POLY_TRANSFORM_PASS_H

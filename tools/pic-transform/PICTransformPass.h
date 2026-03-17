//===-- PICTransformPass.h - Eliminate data sections for PIC shellcode -----===//
//
// LLVM Module pass that transforms global constant data (string literals,
// floating-point constants, constant arrays) into stack-local allocations
// with immediate-value stores, and replaces function pointer references with
// PC-relative inline assembly. Produces binaries with only a .text section.
//
//===----------------------------------------------------------------------===//

#ifndef PIC_TRANSFORM_PASS_H
#define PIC_TRANSFORM_PASS_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class PICTransformPass : public PassInfoMixin<PICTransformPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
  static bool isRequired() { return true; }
};

} // namespace llvm

#endif // PIC_TRANSFORM_PASS_H

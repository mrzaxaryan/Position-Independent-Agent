//===-- PICTransformOpt.cpp - Standalone PIC transform tool ----------------===//
//
// Reads LLVM bitcode or textual IR, runs PICTransformPass to eliminate
// data sections, and writes the transformed result.
//
// Usage:
//   clang++ -emit-llvm -c -O2 input.cpp -o input.bc
//   pic-transform input.bc -o output.bc
//   clang++ output.bc -o output.exe
//
//===----------------------------------------------------------------------===//

#ifndef PIC_TRANSFORM_STANDALONE
#define PIC_TRANSFORM_STANDALONE
#endif

#include "PICTransformPass.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/raw_ostream.h"

#include <chrono>

using namespace llvm;

static cl::opt<std::string>
    InputFilename(cl::Positional, cl::desc("<input bitcode/IR file>"),
                  cl::Required);

static cl::opt<std::string>
    OutputFilename("o", cl::desc("Output filename"),
                   cl::value_desc("filename"), cl::init("-"));

static cl::opt<bool>
    OutputAssembly("S", cl::desc("Write output as LLVM assembly (.ll)"));

int main(int argc, char **argv) {
  InitLLVM X(argc, argv);

  cl::ParseCommandLineOptions(argc, argv,
      "pic-transform - Eliminate data sections from LLVM IR\n\n"
      "Transforms global constants (strings, floats, arrays) into\n"
      "stack-local allocations with immediate-value stores.\n"
      "Produces binaries with only a .text section.\n");

  // ── Parse input ─────────────────────────────────────────────────────
  errs() << "pic-transform: reading " << InputFilename << "\n";

  LLVMContext Context;
  SMDiagnostic Err;
  auto ParseStart = std::chrono::steady_clock::now();
  std::unique_ptr<Module> M = parseIRFile(InputFilename, Err, Context);
  auto ParseEnd = std::chrono::steady_clock::now();

  if (!M) {
    Err.print(argv[0], errs());
    return 1;
  }

  auto ParseMs = std::chrono::duration_cast<std::chrono::milliseconds>(
      ParseEnd - ParseStart).count();
  errs() << "pic-transform: parsed " << M->getName()
         << " (" << ParseMs << " ms)\n";

  // ── Run pass ────────────────────────────────────────────────────────
  ModuleAnalysisManager MAM;
  PICTransformPass Pass;

  auto PassStart = std::chrono::steady_clock::now();
  PreservedAnalyses PA = Pass.run(*M, MAM);
  auto PassEnd = std::chrono::steady_clock::now();

  auto PassMs = std::chrono::duration_cast<std::chrono::milliseconds>(
      PassEnd - PassStart).count();
  bool WasChanged = !PA.areAllPreserved();

  errs() << "pic-transform: pass completed in " << PassMs << " ms"
         << (WasChanged ? " (module modified)" : " (no changes)") << "\n";

  // ── Write output ───────────────────────────────────────────────────
  std::error_code EC;
  ToolOutputFile Out(OutputFilename, EC, sys::fs::OF_None);
  if (EC) {
    errs() << "pic-transform: error: cannot open output file '"
           << OutputFilename << "': " << EC.message() << "\n";
    return 1;
  }

  auto WriteStart = std::chrono::steady_clock::now();
  if (OutputAssembly)
    M->print(Out.os(), nullptr);
  else
    WriteBitcodeToFile(*M, Out.os());
  auto WriteEnd = std::chrono::steady_clock::now();

  auto WriteMs = std::chrono::duration_cast<std::chrono::milliseconds>(
      WriteEnd - WriteStart).count();

  Out.keep();

  errs() << "pic-transform: wrote " << OutputFilename
         << (OutputAssembly ? " (assembly)" : " (bitcode)")
         << " (" << WriteMs << " ms)\n";

  auto TotalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
      WriteEnd - ParseStart).count();
  errs() << "pic-transform: total time: " << TotalMs << " ms\n";

  return 0;
}

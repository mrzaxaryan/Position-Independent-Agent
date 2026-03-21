//===-- PolyTransformOpt.cpp - Standalone poly-transform tool --------------===//
//
// Reads LLVM bitcode or textual IR, runs PolyTransformPass to constrain
// instruction selection, and writes the transformed result.
//
// Usage:
//   clang++ -emit-llvm -c -O2 input.cpp -o input.bc
//   poly-transform --seed=0xDEAD --count=10 input.bc -o output.bc
//   clang++ output.bc -o output.exe
//
//===----------------------------------------------------------------------===//

#ifndef POLY_TRANSFORM_STANDALONE
#define POLY_TRANSFORM_STANDALONE
#endif

#include "PolyTransformPass.h"

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

#include "llvm/IR/Verifier.h"

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

static cl::opt<uint64_t>
    Seed("seed", cl::desc("Random seed for instruction set generation"),
         cl::init(0));

static cl::opt<int>
    Count("count", cl::desc("Number of instructions to select"),
          cl::init(10));

static cl::opt<std::string>
    Arch("arch", cl::desc("Target architecture (x86_64, i386)"),
         cl::init("x86_64"));

int main(int argc, char **argv) {
  InitLLVM X(argc, argv);

  cl::ParseCommandLineOptions(argc, argv,
      "poly-transform - Polymorphic instruction selection for LLVM IR\n\n"
      "Constrains instruction selection to a random subset of the\n"
      "target's instruction set. Each build with a different seed\n"
      "produces a different instruction vocabulary.\n");

  // ── Parse input ─────────────────────────────────────────────────────
  errs() << "poly-transform: reading " << InputFilename << "\n";

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
  errs() << "poly-transform: parsed " << M->getName()
         << " (" << ParseMs << " ms)\n";

  // ── Run pass ────────────────────────────────────────────────────────
  ModuleAnalysisManager MAM;
  PolyTransformPass Pass;
  Pass.Seed = Seed;
  Pass.Count = Count;
  Pass.Arch = Arch;

  auto PassStart = std::chrono::steady_clock::now();
  PreservedAnalyses PA = Pass.run(*M, MAM);
  auto PassEnd = std::chrono::steady_clock::now();

  auto PassMs = std::chrono::duration_cast<std::chrono::milliseconds>(
      PassEnd - PassStart).count();
  bool WasChanged = !PA.areAllPreserved();

  errs() << "poly-transform: pass completed in " << PassMs << " ms"
         << (WasChanged ? " (module modified)" : " (no changes)") << "\n";

  // ── Write output ───────────────────────────────────────────────────
  // If no changes were made, copy input to output verbatim to avoid
  // bitcode round-trip issues (some modules with inline asm from
  // pic-transform don't survive WriteBitcodeToFile round-trips).
  if (!WasChanged && !OutputAssembly && OutputFilename != "-") {
    std::error_code CopyEC =
        sys::fs::copy_file(InputFilename, OutputFilename);
    if (CopyEC) {
      errs() << "poly-transform: error copying: " << CopyEC.message() << "\n";
      return 1;
    }
    errs() << "poly-transform: no changes — copied input to "
           << OutputFilename << "\n";
    return 0;
  }

  // Verify the module is valid before writing.  If verification fails
  // (e.g., pic-transform created IR with types that don't round-trip
  // through bitcode), fall back to copying the input unchanged.
  if (verifyModule(*M, &errs())) {
    errs() << "poly-transform: module verification failed after transform"
           << " — copying input unchanged\n";
    std::error_code CopyEC =
        sys::fs::copy_file(InputFilename, OutputFilename);
    if (CopyEC) {
      errs() << "poly-transform: error copying: " << CopyEC.message() << "\n";
      return 1;
    }
    return 0;
  }

  std::error_code EC;
  ToolOutputFile Out(OutputFilename, EC, sys::fs::OF_None);
  if (EC) {
    errs() << "poly-transform: error: cannot open output file '"
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

  errs() << "poly-transform: wrote " << OutputFilename
         << (OutputAssembly ? " (assembly)" : " (bitcode)")
         << " (" << WriteMs << " ms)\n";

  auto TotalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
      WriteEnd - ParseStart).count();
  errs() << "poly-transform: total time: " << TotalMs << " ms\n";

  return 0;
}

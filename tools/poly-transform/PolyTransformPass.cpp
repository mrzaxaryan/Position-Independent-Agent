//===-- PolyTransformPass.cpp - Polymorphic instruction selection pass -----===//
//
// Transforms LLVM IR so the backend emits only instructions from an allowed
// subset.  Operates on binary arithmetic, logic, and comparison operations.
//
// Key transformations:
//   - ADD → SUB with negated operand (when ADD disallowed, SUB allowed)
//   - SUB → ADD with negated operand
//   - XOR → AND + OR + NOT  (De Morgan's: a^b = (a|b) & ~(a&b))
//   - AND → NOT(NOT(a) | NOT(b))  (De Morgan's)
//   - OR  → NOT(NOT(a) & NOT(b))  (De Morgan's)
//   - MUL by power-of-2 → SHL
//   - Comparison operand swaps (to influence Jcc selection)
//
// The pass runs AFTER pic-transform and BEFORE the backend, so the compiler
// handles register allocation and instruction encoding correctly.
//
//===----------------------------------------------------------------------===//

#include "PolyTransformPass.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Triple.h"

#include <vector>

using namespace llvm;

namespace {

// ============================================================================
// PRNG — xorshift64 (mirrors core/math/prng.h)
// ============================================================================

class PRNG {
  uint64_t State;

public:
  explicit PRNG(uint64_t Seed) : State(Seed ? Seed : 1) {}
  uint64_t next() {
    State ^= State << 13;
    State ^= State >> 7;
    State ^= State << 17;
    return State;
  }
  uint64_t range(uint64_t N) { return N > 1 ? next() % N : 0; }
};

// ============================================================================
// Instruction set categories and generation
// ============================================================================

struct Category {
  const char *Name;
  std::vector<std::string> Instructions;
  int MinPicks;
};

struct ArchDef {
  std::vector<std::string> Mandatory;
  std::vector<Category> Categories;
};

static ArchDef getX86Def() {
  return {{"mov", "call"}, {
    {"data_move",   {"lea", "push", "pop", "movzx", "movsx", "xchg"}, 1},
    {"add_or_sub",  {"add", "sub"},                                    1}, // must have at least one
    {"arithmetic",  {"imul", "mul", "div", "inc", "dec",
                     "neg", "not", "adc", "sbb"},                      1},
    {"logic",       {"xor", "and", "or"},                              1},
    {"shift",       {"shl", "shr", "sar", "rol", "shld", "shrd"},     0},
    {"compare",     {"cmp", "test", "bt"},                             1},
    {"branch",      {"jcc", "jmp"},                                    1},
    {"control",     {"ret"},                                           0},
    {"conditional", {"cmov", "set"},                                   0},
    {"float",       {"fadd", "fsub", "fmul", "fdiv", "fcvt", "fcmp",
                     "fmov", "flogic"},                                0},
  }};
}

static std::set<std::string> generateInstructionSet(uint64_t Seed, int Count,
                                                    const std::string &Arch) {
  ArchDef Def = getX86Def(); // TODO: add AArch64/RISC-V defs
  PRNG Rng(Seed);

  std::set<std::string> Result;
  for (auto &M : Def.Mandatory)
    Result.insert(M);

  // Pick minimums from each category
  for (auto &Cat : Def.Categories) {
    auto Pool = Cat.Instructions;
    for (size_t I = Pool.size(); I > 1; I--) {
      size_t J = Rng.range(I);
      std::swap(Pool[I - 1], Pool[J]);
    }
    for (int I = 0; I < Cat.MinPicks && I < (int)Pool.size(); I++)
      Result.insert(Pool[I]);
  }

  // Distribute remaining budget
  int Remaining = Count - (int)Result.size();
  int MaxAttempts = Remaining * 100;
  while (Remaining > 0 && MaxAttempts-- > 0) {
    size_t CatIdx = Rng.range(Def.Categories.size());
    auto Pool = Def.Categories[CatIdx].Instructions;
    for (size_t I = Pool.size(); I > 1; I--) {
      size_t J = Rng.range(I);
      std::swap(Pool[I - 1], Pool[J]);
    }
    for (auto &Inst : Pool) {
      if (Result.find(Inst) == Result.end()) {
        Result.insert(Inst);
        Remaining--;
        break;
      }
    }
  }

  return Result;
}

// ============================================================================
// IR Transformation Helpers
// ============================================================================

/// Check if a mnemonic is in the allowed set.
static bool isAllowed(const std::string &Mnemonic,
                      const std::set<std::string> &Allowed) {
  // Always-allowed: syscall, nop, int3, float ops, etc.
  static const std::set<std::string> AlwaysOK = {
      "syscall", "nop",   "int3",  "rdtsc",  "cltd",   "bswap",
      "bsr",     "rep",   "fmov",  "fadd",   "fsub",   "fmul",
      "fdiv",    "fcvt",  "fcmp",  "flogic", "mov",    "call",
  };
  return Allowed.count(Mnemonic) > 0 || AlwaysOK.count(Mnemonic) > 0;
}

// ============================================================================
// Core: transform a single instruction
// ============================================================================

/// Transform binary operations to use only allowed instructions.
/// Returns true if the instruction was modified.
// Per-transform counters for detailed logging
struct TransformStats {
  unsigned AddToSub = 0;
  unsigned SubToAdd = 0;
  unsigned XorToAndOr = 0;
  unsigned AndToOr = 0;
  unsigned OrToAnd = 0;
  unsigned ShlToMul = 0;
  unsigned MulToShl = 0;
  unsigned CmpToSub = 0;
  unsigned Total() const {
    return AddToSub + SubToAdd + XorToAndOr + AndToOr + OrToAnd +
           ShlToMul + MulToShl + CmpToSub;
  }
};

static bool transformBinOp(BinaryOperator *BO,
                           const std::set<std::string> &Allowed,
                           TransformStats &Stats) {
  // Only transform standard integer types (i8, i16, i32, i64).
  // Skip i1 (boolean), vectors, and unusual widths.
  Type *Ty = BO->getType();
  if (!Ty->isIntegerTy() || Ty->getIntegerBitWidth() < 8)
    return false;

  IRBuilder<> B(BO);
  Value *LHS = BO->getOperand(0);
  Value *RHS = BO->getOperand(1);
  Value *Replacement = nullptr;
  const char *RuleName = nullptr;

  switch (BO->getOpcode()) {

  // ── ADD → SUB with negated RHS ─────────────────────────────────────
  case Instruction::Add:
    if (!isAllowed("add", Allowed) && isAllowed("sub", Allowed)) {
      // add a, b → sub a, (sub 0, b)
      // Drop nsw/nuw flags to avoid UB mismatch with LTO
      Value *Zero = ConstantInt::get(Ty, 0);
      Value *Neg = B.CreateSub(Zero, RHS, "poly.neg");
      Replacement = B.CreateSub(LHS, Neg, "poly.sub");
      RuleName = "add->sub";
      Stats.AddToSub++;
    }
    break;

  // ── SUB → ADD with negated RHS ─────────────────────────────────────
  case Instruction::Sub:
    if (!isAllowed("sub", Allowed) && isAllowed("add", Allowed)) {
      // sub a, b → add a, (sub 0, b)
      Value *Zero = ConstantInt::get(Ty, 0);
      Value *Neg = B.CreateSub(Zero, RHS, "poly.neg");
      Replacement = B.CreateAdd(LHS, Neg, "poly.add");
      RuleName = "sub->add";
      Stats.SubToAdd++;
    }
    break;

  // ── XOR → (a | b) & ~(a & b) ──────────────────────────────────────
  case Instruction::Xor:
    if (!isAllowed("xor", Allowed) && isAllowed("and", Allowed) &&
        isAllowed("or", Allowed)) {
      // De Morgan's: a ^ b = (a | b) & ~(a & b)
      Value *Or = B.CreateOr(LHS, RHS, "poly.or");
      Value *And = B.CreateAnd(LHS, RHS, "poly.and");
      Value *NotAnd = B.CreateNot(And, "poly.notand");
      Replacement = B.CreateAnd(Or, NotAnd, "poly.xor");
      RuleName = "xor->and+or";
      Stats.XorToAndOr++;
    }
    break;

  // ── AND → ~(~a | ~b) ──────────────────────────────────────────────
  case Instruction::And:
    if (!isAllowed("and", Allowed) && isAllowed("or", Allowed)) {
      // De Morgan's: a & b = ~(~a | ~b)
      Value *NotA = B.CreateNot(LHS, "poly.nota");
      Value *NotB = B.CreateNot(RHS, "poly.notb");
      Value *Or = B.CreateOr(NotA, NotB, "poly.or");
      Replacement = B.CreateNot(Or, "poly.and");
      RuleName = "and->or+not";
      Stats.AndToOr++;
    }
    break;

  // ── OR → ~(~a & ~b) ───────────────────────────────────────────────
  case Instruction::Or:
    if (!isAllowed("or", Allowed) && isAllowed("and", Allowed)) {
      // De Morgan's: a | b = ~(~a & ~b)
      Value *NotA = B.CreateNot(LHS, "poly.nota");
      Value *NotB = B.CreateNot(RHS, "poly.notb");
      Value *And = B.CreateAnd(NotA, NotB, "poly.and");
      Replacement = B.CreateNot(And, "poly.or");
      RuleName = "or->and+not";
      Stats.OrToAnd++;
    }
    break;

  // ── SHL → MUL by power of 2 ───────────────────────────────────────
  case Instruction::Shl:
    if (!isAllowed("shl", Allowed) && isAllowed("mul", Allowed)) {
      if (auto *C = dyn_cast<ConstantInt>(RHS)) {
        uint64_t ShiftAmt = C->getZExtValue();
        if (ShiftAmt < 64) {
          Value *Pow2 = ConstantInt::get(Ty, 1ULL << ShiftAmt);
          Replacement = B.CreateMul(LHS, Pow2, "poly.mul");
          RuleName = "shl->mul";
          Stats.ShlToMul++;
        }
      }
    }
    break;

  // ── MUL by power-of-2 constant → SHL ──────────────────────────────
  case Instruction::Mul:
    if (!isAllowed("mul", Allowed) && !isAllowed("imul", Allowed) &&
        isAllowed("shl", Allowed)) {
      if (auto *C = dyn_cast<ConstantInt>(RHS)) {
        uint64_t Val = C->getZExtValue();
        if (Val != 0 && (Val & (Val - 1)) == 0) { // is power of 2
          unsigned ShiftAmt = 0;
          uint64_t Tmp = Val;
          while (Tmp > 1) {
            Tmp >>= 1;
            ShiftAmt++;
          }
          Value *Shift = ConstantInt::get(Ty, ShiftAmt);
          Replacement = B.CreateShl(LHS, Shift, "poly.shl");
          RuleName = "mul->shl";
          Stats.MulToShl++;
        }
      }
    }
    break;

  default:
    break;
  }

  if (Replacement) {
    if (auto *NewI = dyn_cast<Instruction>(Replacement))
      NewI->setDebugLoc(BO->getDebugLoc());
    BO->replaceAllUsesWith(Replacement);
    BO->eraseFromParent();
    (void)RuleName; // used by caller's stats
    return true;
  }
  return false;
}

/// Transform comparison operations to prefer certain condition codes.
/// This influences whether the backend emits CMP vs TEST, and which
/// Jcc variant (JE vs JNE, etc.) is selected.
static bool transformCmp(ICmpInst *CI, const std::set<std::string> &Allowed,
                         TransformStats &Stats) {
  // If TEST is disallowed but CMP is allowed, we can't do much at IR level
  // since the backend chooses TEST vs CMP based on operand patterns.
  // If CMP is disallowed but SUB is allowed, we can force SUB-based comparison
  // by replacing icmp with sub + comparison against zero.
  if (!isAllowed("cmp", Allowed) && !isAllowed("test", Allowed) &&
      isAllowed("sub", Allowed)) {
    IRBuilder<> B(CI);
    Value *LHS = CI->getOperand(0);
    Value *RHS = CI->getOperand(1);

    // icmp pred a, b → sub a, b; icmp pred result, 0
    // This forces the backend to use SUB instead of CMP
    Value *Diff = B.CreateSub(LHS, RHS, "poly.diff");
    Value *Zero = ConstantInt::get(Diff->getType(), 0);
    Value *NewCmp = B.CreateICmp(CI->getPredicate(), Diff, Zero, "poly.cmp");
    // Note: this changes semantics for unsigned comparisons.
    // Only safe for eq/ne. For now, only transform eq/ne.
    if (CI->getPredicate() == ICmpInst::ICMP_EQ ||
        CI->getPredicate() == ICmpInst::ICMP_NE) {
      CI->replaceAllUsesWith(NewCmp);
      CI->eraseFromParent();
      Stats.CmpToSub++;
      return true;
    }
  }
  return false;
}

// ============================================================================
// Pass entry point
// ============================================================================

static unsigned transformFunction(Function &F,
                                  const std::set<std::string> &Allowed,
                                  TransformStats &Stats) {
  unsigned Changed = 0;

  // Collect instructions first (avoid iterator invalidation)
  std::vector<Instruction *> Worklist;
  for (auto &BB : F)
    for (auto &I : BB)
      Worklist.push_back(&I);

  for (auto *I : Worklist) {
    if (I->getParent() == nullptr)
      continue;

    if (auto *BO = dyn_cast<BinaryOperator>(I)) {
      if (transformBinOp(BO, Allowed, Stats))
        Changed++;
    } else if (auto *CI = dyn_cast<ICmpInst>(I)) {
      if (transformCmp(CI, Allowed, Stats))
        Changed++;
    }
  }

  return Changed;
}

} // anonymous namespace

// ============================================================================
// Module pass entry
// ============================================================================

PreservedAnalyses PolyTransformPass::run(Module &M,
                                         ModuleAnalysisManager &MAM) {
  // Derive a per-module seed by hashing the module identifier (source path).
  // XOR with the CLI seed so each build date produces different results
  // AND each source file within the same build gets a unique instruction set.
  uint64_t ModuleHash = 0xcbf29ce484222325ULL; // FNV-1a offset basis
  for (char C : M.getModuleIdentifier()) {
    ModuleHash ^= static_cast<uint8_t>(C);
    ModuleHash *= 0x100000001b3ULL;
  }
  uint64_t EffectiveSeed = ModuleHash ^ Seed;

  // Determine the allowed set
  std::set<std::string> Allowed = AllowedSet;

  if (Allowed.empty()) {
    Allowed = generateInstructionSet(EffectiveSeed, Count, Arch);
  }

  if (Allowed.empty()) {
    errs() << "poly-transform: no instruction set specified, skipping\n";
    return PreservedAnalyses::all();
  }

  // Log the allowed set
  errs() << "poly-transform: allowed set = {";
  bool First = true;
  for (auto &S : Allowed) {
    if (!First) errs() << ", ";
    errs() << S;
    First = false;
  }
  errs() << "} (seed=0x";
  errs().write_hex(Seed);
  errs() << ", count=" << Count << ")\n";

  // Transform all functions
  TransformStats Stats;
  unsigned TotalChanged = 0;
  unsigned FuncsChanged = 0;
  unsigned FuncsTotal = 0;
  for (auto &F : M) {
    if (F.isDeclaration())
      continue;
    FuncsTotal++;
    unsigned N = transformFunction(F, Allowed, Stats);
    TotalChanged += N;
    if (N > 0)
      FuncsChanged++;
  }

  errs() << "poly-transform: transformed " << TotalChanged
         << " IR instructions in " << FuncsChanged << "/" << FuncsTotal
         << " functions\n";

  // Detailed per-rule breakdown
  if (TotalChanged > 0) {
    errs() << "poly-transform:   breakdown:";
    if (Stats.AddToSub)   errs() << " add->sub=" << Stats.AddToSub;
    if (Stats.SubToAdd)   errs() << " sub->add=" << Stats.SubToAdd;
    if (Stats.XorToAndOr) errs() << " xor->and+or=" << Stats.XorToAndOr;
    if (Stats.AndToOr)    errs() << " and->or+not=" << Stats.AndToOr;
    if (Stats.OrToAnd)    errs() << " or->and+not=" << Stats.OrToAnd;
    if (Stats.ShlToMul)   errs() << " shl->mul=" << Stats.ShlToMul;
    if (Stats.MulToShl)   errs() << " mul->shl=" << Stats.MulToShl;
    if (Stats.CmpToSub)   errs() << " cmp->sub=" << Stats.CmpToSub;
    errs() << "\n";
  }

  // Show what the backend WILL NOT be able to use (disallowed set)
  errs() << "poly-transform:   disallowed: {";
  bool First2 = true;
  for (auto &Cat : {"add", "sub", "xor", "and", "or", "shl", "shr", "sar",
                     "rol", "shld", "shrd", "imul", "mul", "div", "inc",
                     "dec", "neg", "not", "adc", "sbb", "cmp", "test",
                     "bt", "jcc", "jmp", "call", "ret", "push", "pop",
                     "lea", "movzx", "movsx", "xchg", "cmov", "set"}) {
    if (!isAllowed(Cat, Allowed)) {
      if (!First2) errs() << ", ";
      errs() << Cat;
      First2 = false;
    }
  }
  errs() << "}\n";

  if (TotalChanged == 0)
    return PreservedAnalyses::all();

  return PreservedAnalyses::none();
}

// ============================================================================
// Plugin registration (when built as loadable plugin, not standalone)
// ============================================================================

#ifndef POLY_TRANSFORM_STANDALONE

#include "llvm/Config/llvm-config.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
static void registerPolyTransformPass(PassBuilder &PB) {
  // Register at OptimizerLastEP — runs after all standard optimizations
  // but before LTO code generation.  For non-LTO builds this is the
  // final opportunity; for LTO builds it runs per-TU at compile time
  // AND again on the merged module at link time.
  PB.registerOptimizerLastEPCallback(
      [](ModulePassManager &MPM, OptimizationLevel OL
#if LLVM_VERSION_MAJOR < 22
         ,
         ThinOrFullLTOPhase
#endif
      ) {
        PolyTransformPass Pass;
        MPM.addPass(std::move(Pass));
      });
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "PolyTransform", LLVM_VERSION_STRING,
          registerPolyTransformPass};
}

#endif // POLY_TRANSFORM_STANDALONE

//===-- PICTransformPass.cpp - Eliminate data sections for PIC shellcode ---===//
//
// An LLVM Module pass that transforms global constant data (string literals,
// floating-point constants, constant arrays) into stack-local allocations with
// immediate-value stores. This produces binaries with only a .text section,
// suitable for position-independent shellcode.
//
// Transformations:
//   1. Global constant strings  -> stack alloca + packed word-sized stores
//   2. Global constant arrays   -> stack alloca + packed word-sized stores
//   3. Inline ConstantFP values -> integer immediate + bitcast
//   4. Function pointer values  -> PC-relative inline asm
//   5. Diagnostics for remaining non-eliminable globals
//
//===----------------------------------------------------------------------===//

#include "PICTransformPass.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Triple.h"

#include <vector>

using namespace llvm;

namespace
{

  // ============================================================================
  // Immediate store emission
  // ============================================================================

  /// Emit an inline asm register barrier for a single value.
  /// Equivalent to: __asm__ volatile("" : "+r"(val))
  /// This prevents the optimizer from recognizing the stored pattern and
  /// coalescing it back into a memcpy from .rodata.
  static Value *emitRegisterBarrier(IRBuilder<> &Builder, Value *Val)
  {
    Type *Ty = Val->getType();
    FunctionType *AsmTy = FunctionType::get(Ty, {Ty}, false);
    InlineAsm *Barrier = InlineAsm::get(
        AsmTy, "", "=r,0", /*hasSideEffects=*/true, /*isAlignStack=*/false);
    return Builder.CreateCall(Barrier, {Val});
  }

  /// Pack bytes into word-sized integer constants and emit volatile stores
  /// to write them into a stack allocation. Each word is passed through an
  /// inline asm register barrier first.
  ///
  /// On x86_64 this produces:
  ///   movabsq $0x57202C6F6C6C6548, %rax   ; "Hello, W" packed
  ///   mov [rsp+offset], rax
  ///
  /// On AArch64 this produces:
  ///   movz x0, #imm16
  ///   movk x0, #imm16, lsl #16
  ///   ...
  ///   str x0, [sp, #offset]
  static void emitPackedImmediateStores(IRBuilder<> &Builder, Value *BasePtr,
                                        const uint8_t *Data, uint64_t DataSize,
                                        unsigned WordSize)
  {
    LLVMContext &Ctx = Builder.getContext();
    Type *WordTy = IntegerType::get(Ctx, WordSize * 8);
    Type *I8Ty = Builder.getInt8Ty();
    uint64_t NumFullWords = DataSize / WordSize;
    uint64_t Remainder = DataSize % WordSize;

    for (uint64_t I = 0; I < NumFullWords; ++I)
    {
      // Pack WordSize bytes into one integer constant (little-endian)
      uint64_t Word = 0;
      for (unsigned B = 0; B < WordSize; ++B)
        Word |= static_cast<uint64_t>(Data[I * WordSize + B]) << (B * 8);

      Value *WordVal = emitRegisterBarrier(
          Builder, ConstantInt::get(WordTy, Word));

      Value *Ptr = Builder.CreateConstInBoundsGEP1_64(I8Ty, BasePtr,
                                                      I * WordSize);
      StoreInst *SI = Builder.CreateAlignedStore(WordVal, Ptr, Align(1));
      SI->setVolatile(true);
    }

    // Handle remaining bytes with progressively smaller stores
    if (Remainder > 0)
    {
      uint64_t Offset = NumFullWords * WordSize;

      if (Remainder >= 4 && WordSize == 8)
      {
        uint32_t Word32 = 0;
        for (unsigned B = 0; B < 4; ++B)
          Word32 |= static_cast<uint32_t>(Data[Offset + B]) << (B * 8);

        Type *I32Ty = IntegerType::get(Ctx, 32);
        Value *WordVal = emitRegisterBarrier(
            Builder, ConstantInt::get(I32Ty, Word32));
        Value *Ptr = Builder.CreateConstInBoundsGEP1_64(I8Ty, BasePtr, Offset);
        StoreInst *SI = Builder.CreateAlignedStore(WordVal, Ptr, Align(1));
        SI->setVolatile(true);
        Offset += 4;
        Remainder -= 4;
      }

      if (Remainder >= 2)
      {
        uint16_t Word16 = 0;
        for (unsigned B = 0; B < 2; ++B)
          Word16 |= static_cast<uint16_t>(Data[Offset + B]) << (B * 8);

        Type *I16Ty = IntegerType::get(Ctx, 16);
        Value *WordVal = emitRegisterBarrier(
            Builder, ConstantInt::get(I16Ty, Word16));
        Value *Ptr = Builder.CreateConstInBoundsGEP1_64(I8Ty, BasePtr, Offset);
        StoreInst *SI = Builder.CreateAlignedStore(WordVal, Ptr, Align(1));
        SI->setVolatile(true);
        Offset += 2;
        Remainder -= 2;
      }

      if (Remainder == 1)
      {
        Value *ByteVal = ConstantInt::get(I8Ty, Data[Offset]);
        Value *Ptr = Builder.CreateConstInBoundsGEP1_64(I8Ty, BasePtr, Offset);
        StoreInst *SI = Builder.CreateStore(ByteVal, Ptr);
        SI->setVolatile(true);
      }
    }
  }

  // ============================================================================
  // Constant analysis
  // ============================================================================

  /// Check if a GlobalVariable is a constant that should be transformed.
  static bool isTransformableConstant(const GlobalVariable &GV)
  {
    if (!GV.isConstant() || !GV.hasInitializer())
      return false;
    // Accept private, internal, AND linkonce_odr (used by MSVC ABI for
    // string literal deduplication).
    if (!GV.hasLocalLinkage() && !GV.hasLinkOnceLinkage())
      return false;
    if (GV.hasSection())
      return false;

    // Skip LLVM-internal globals
    if (GV.getName().starts_with("llvm.") ||
        GV.getName().starts_with("__llvm"))
      return false;

    const Constant *Init = GV.getInitializer();
    return isa<ConstantDataSequential>(Init) ||
           isa<ConstantFP>(Init) ||
           isa<ConstantInt>(Init) ||
           isa<ConstantAggregateZero>(Init) ||
           isa<ConstantAggregate>(Init);
  }

  /// Extract raw bytes from a Constant initializer into a byte vector.
  static bool extractConstantBytes(const DataLayout &DL, Constant *C,
                                   SmallVectorImpl<uint8_t> &Bytes)
  {
    uint64_t Size = DL.getTypeAllocSize(C->getType());
    Bytes.resize(Size, 0);

    if (auto *CDS = dyn_cast<ConstantDataSequential>(C))
    {
      StringRef RawData = CDS->getRawDataValues();
      std::copy(RawData.begin(), RawData.end(), Bytes.begin());
      return true;
    }

    if (auto *CFP = dyn_cast<ConstantFP>(C))
    {
      APInt Bits = CFP->getValueAPF().bitcastToAPInt();
      unsigned ByteCount = Bits.getBitWidth() / 8;
      for (unsigned I = 0; I < ByteCount; ++I)
        Bytes[I] = static_cast<uint8_t>(Bits.extractBitsAsZExtValue(8, I * 8));
      return true;
    }

    if (auto *CI = dyn_cast<ConstantInt>(C))
    {
      unsigned ByteCount = CI->getBitWidth() / 8;
      APInt Bits = CI->getValue();
      for (unsigned I = 0; I < ByteCount; ++I)
        Bytes[I] = static_cast<uint8_t>(Bits.extractBitsAsZExtValue(8, I * 8));
      return true;
    }

    if (isa<ConstantAggregateZero>(C))
      return true;

    if (auto *CA = dyn_cast<ConstantAggregate>(C))
    {
      if (auto *AT = dyn_cast<ArrayType>(C->getType()))
      {
        uint64_t ElemSize = DL.getTypeAllocSize(AT->getElementType());
        for (unsigned I = 0; I < CA->getNumOperands(); ++I)
        {
          SmallVector<uint8_t, 64> ElemBytes;
          if (!extractConstantBytes(DL, CA->getOperand(I), ElemBytes))
            return false;
          std::copy(ElemBytes.begin(), ElemBytes.end(),
                    Bytes.begin() + I * ElemSize);
        }
        return true;
      }

      if (auto *ST = dyn_cast<StructType>(C->getType()))
      {
        const StructLayout *SL = DL.getStructLayout(ST);
        for (unsigned I = 0; I < CA->getNumOperands(); ++I)
        {
          SmallVector<uint8_t, 64> ElemBytes;
          if (!extractConstantBytes(DL, CA->getOperand(I), ElemBytes))
            return false;
          uint64_t Offset = SL->getElementOffset(I);
          std::copy(ElemBytes.begin(), ElemBytes.end(),
                    Bytes.begin() + Offset);
        }
        return true;
      }
    }

    return false;
  }

  // ============================================================================
  // Use replacement
  // ============================================================================

  struct UseInfo
  {
    Use *U;
    /// The outermost Constant through which the GV is used (nullptr if the GV
    /// is used directly by an Instruction).
    Constant *OuterC;
  };

  /// Recursively walk Constant users to find instruction-level uses in \p F.
  /// Handles ConstantExpr, ConstantStruct, ConstantArray, ConstantVector, and
  /// any other Constant nesting.
  static void collectConstUsesInFunction(Constant *C, Function &F,
                                         Constant *Outermost,
                                         SmallVectorImpl<UseInfo> &Result)
  {
    for (Use &U : C->uses())
    {
      Value *User = U.getUser();
      if (auto *I = dyn_cast<Instruction>(User))
      {
        if (I->getFunction() == &F)
          Result.push_back({&U, Outermost});
      }
      else if (auto *Inner = dyn_cast<Constant>(User))
      {
        // Each level updates Outermost to be the current constant, so it
        // always reflects the constant directly used by the instruction.
        // materializeConstant needs this outermost constant to rebuild the
        // full chain from instruction operand down to the target GV.
        collectConstUsesInFunction(Inner, F, Inner, Result);
      }
    }
  }

  static void collectUsesInFunction(GlobalVariable &GV, Function &F,
                                    SmallVectorImpl<UseInfo> &Result)
  {
    for (Use &U : GV.uses())
    {
      Value *User = U.getUser();
      if (auto *I = dyn_cast<Instruction>(User))
      {
        if (I->getFunction() == &F)
          Result.push_back({&U, nullptr});
      }
      else if (auto *C = dyn_cast<Constant>(User))
      {
        collectConstUsesInFunction(C, F, C, Result);
      }
    }
  }

  /// Recursively rebuild a Constant as instructions, replacing every occurrence
  /// of \p OldGV with \p NewVal. Constants that do not (transitively) reference
  /// \p OldGV are returned as-is.
  static Value *materializeConstant(IRBuilder<> &B, Constant *C,
                                    GlobalVariable &OldGV, Value *NewVal)
  {
    // Base case: the global itself.
    if (C == &OldGV)
      return NewVal;

    // ConstantExpr — recreate as an instruction with patched operands.
    if (auto *CE = dyn_cast<ConstantExpr>(C))
    {
      if (CE->getOpcode() == Instruction::GetElementPtr)
      {
        auto *GEP = cast<GEPOperator>(CE);
        Value *Ptr = materializeConstant(B, cast<Constant>(GEP->getPointerOperand()),
                                         OldGV, NewVal);
        SmallVector<Value *, 4> Indices(GEP->idx_begin(), GEP->idx_end());
        return B.CreateInBoundsGEP(GEP->getSourceElementType(), Ptr, Indices,
                                   "gep.local");
      }
      if (CE->getOpcode() == Instruction::BitCast)
      {
        Value *Src = materializeConstant(B, CE->getOperand(0), OldGV, NewVal);
        return B.CreateBitCast(Src, CE->getType(), "cast.local");
      }
      if (CE->getOpcode() == Instruction::PtrToInt)
      {
        Value *Src = materializeConstant(B, CE->getOperand(0), OldGV, NewVal);
        return B.CreatePtrToInt(Src, CE->getType(), "ptoi.local");
      }
      if (CE->getOpcode() == Instruction::IntToPtr)
      {
        Value *Src = materializeConstant(B, CE->getOperand(0), OldGV, NewVal);
        return B.CreateIntToPtr(Src, CE->getType(), "itop.local");
      }
      if (CE->getOpcode() == Instruction::AddrSpaceCast)
      {
        Value *Src = materializeConstant(B, CE->getOperand(0), OldGV, NewVal);
        return B.CreateAddrSpaceCast(Src, CE->getType(), "asc.local");
      }
      // Unknown CE opcode — fall through and return as-is.
      return C;
    }

    // ConstantStruct — rebuild with insertvalue.
    if (auto *CS = dyn_cast<ConstantStruct>(C))
    {
      Value *Agg = PoisonValue::get(CS->getType());
      for (unsigned I = 0; I < CS->getNumOperands(); ++I)
      {
        Value *Elem = materializeConstant(B, CS->getOperand(I), OldGV, NewVal);
        Agg = B.CreateInsertValue(Agg, Elem, {I});
      }
      return Agg;
    }

    // ConstantArray — rebuild with insertvalue.
    if (auto *CA = dyn_cast<ConstantArray>(C))
    {
      Value *Agg = PoisonValue::get(CA->getType());
      for (unsigned I = 0; I < CA->getNumOperands(); ++I)
      {
        Value *Elem = materializeConstant(B, CA->getOperand(I), OldGV, NewVal);
        Agg = B.CreateInsertValue(Agg, Elem, {I});
      }
      return Agg;
    }

    // ConstantVector — rebuild with insertelement.
    if (auto *CV = dyn_cast<ConstantVector>(C))
    {
      Value *Vec = PoisonValue::get(CV->getType());
      for (unsigned I = 0; I < CV->getNumOperands(); ++I)
      {
        Value *Elem = materializeConstant(B, CV->getOperand(I), OldGV, NewVal);
        Vec = B.CreateInsertElement(Vec, Elem, B.getInt32(I));
      }
      return Vec;
    }

    // Anything else (ConstantInt, ConstantFP, etc.) — return unchanged.
    return C;
  }

  static bool replaceGlobalInFunction(Function &F, GlobalVariable &GV,
                                      const SmallVectorImpl<uint8_t> &Bytes,
                                      const DataLayout &DL)
  {
    SmallVector<UseInfo, 8> Uses;
    collectUsesInFunction(GV, F, Uses);
    if (Uses.empty())
      return false;

    BasicBlock &Entry = F.getEntryBlock();
    IRBuilder<> AllocaBuilder(&Entry, Entry.getFirstInsertionPt());

    Type *AllocTy = GV.getValueType();
    Align AllocAlign = GV.getAlign().value_or(DL.getABITypeAlign(AllocTy));

    AllocaInst *Alloca = AllocaBuilder.CreateAlloca(
        AllocTy, nullptr, GV.getName() + ".local");
    Alloca->setAlignment(AllocAlign);

    IRBuilder<> StoreBuilder(Alloca->getNextNode());

    // Emit an explicit lifetime.start so that the backend's LiveStacks /
    // StackSlotColoring passes see a frame-index reference at the entry
    // block.  Without this, the only frame-index references come from the
    // volatile stores below — but those use GEP-based computed addresses
    // that ISel may lower to plain SP-relative addressing without a
    // frame-index operand.  On AArch64 -Oz the missing reference lets
    // StackSlotColoring merge same-sized alloca slots whose GEP-based
    // stores it cannot see, corrupting the data.
    StoreBuilder.CreateLifetimeStart(Alloca);

    unsigned WordSize = DL.getPointerSize();
    emitPackedImmediateStores(StoreBuilder, Alloca,
                              Bytes.data(), Bytes.size(), WordSize);

    for (auto &UI : Uses)
    {
      if (!UI.OuterC)
      {
        // Direct instruction use — just swap in the alloca.
        UI.U->set(Alloca);
      }
      else
      {
        auto *UserInst = cast<Instruction>(UI.U->getUser());

        // For PHI nodes, insert materialization at the end of the incoming
        // block (before its terminator), not before the PHI itself.
        IRBuilder<> B(UserInst);
        if (auto *PHI = dyn_cast<PHINode>(UserInst))
        {
          unsigned OpIdx = UI.U->getOperandNo();
          BasicBlock *IncomingBB = PHI->getIncomingBlock(OpIdx);
          B.SetInsertPoint(IncomingBB->getTerminator());
        }

        Value *Replacement = materializeConstant(B, UI.OuterC, GV, Alloca);
        UI.U->set(Replacement);
      }
    }

    return true;
  }

  /// Structural replacement for ConstantAggregate globals that contain pointer
  /// elements (e.g., `const char *args[] = {"cmd.exe", nullptr}`).
  /// Instead of byte-packing (which cannot serialize relocatable pointers),
  /// this creates a stack alloca and emits element-wise stores using the
  /// original Constant values.  Pointer elements that reference other globals
  /// (e.g., string literals) are stored directly; when those globals are
  /// processed later in Phase 2, collectUsesInFunction finds and patches
  /// the store instructions automatically.
  static bool replaceAggregateGlobalInFunction(Function &F, GlobalVariable &GV,
                                                const DataLayout &DL)
  {
    SmallVector<UseInfo, 8> Uses;
    collectUsesInFunction(GV, F, Uses);
    if (Uses.empty())
      return false;

    BasicBlock &Entry = F.getEntryBlock();
    IRBuilder<> AllocaBuilder(&Entry, Entry.getFirstInsertionPt());

    Type *AllocTy = GV.getValueType();
    Align AllocAlign = GV.getAlign().value_or(DL.getABITypeAlign(AllocTy));

    AllocaInst *Alloca = AllocaBuilder.CreateAlloca(
        AllocTy, nullptr, GV.getName() + ".local");
    Alloca->setAlignment(AllocAlign);

    IRBuilder<> StoreBuilder(Alloca->getNextNode());
    StoreBuilder.CreateLifetimeStart(Alloca);

    Constant *Init = GV.getInitializer();

    if (auto *AT = dyn_cast<ArrayType>(AllocTy))
    {
      for (unsigned I = 0; I < AT->getNumElements(); ++I)
      {
        Constant *Elem = Init->getAggregateElement(I);
        Value *ElemPtr = StoreBuilder.CreateConstInBoundsGEP2_64(
            AllocTy, Alloca, 0, I);
        StoreBuilder.CreateStore(Elem, ElemPtr);
      }
    }
    else if (auto *ST = dyn_cast<StructType>(AllocTy))
    {
      for (unsigned I = 0; I < ST->getNumElements(); ++I)
      {
        Constant *Elem = Init->getAggregateElement(I);
        Value *ElemPtr = StoreBuilder.CreateStructGEP(AllocTy, Alloca, I);
        StoreBuilder.CreateStore(Elem, ElemPtr);
      }
    }

    for (auto &UI : Uses)
    {
      if (!UI.OuterC)
      {
        UI.U->set(Alloca);
      }
      else
      {
        auto *UserInst = cast<Instruction>(UI.U->getUser());

        IRBuilder<> B(UserInst);
        if (auto *PHI = dyn_cast<PHINode>(UserInst))
        {
          unsigned OpIdx = UI.U->getOperandNo();
          BasicBlock *IncomingBB = PHI->getIncomingBlock(OpIdx);
          B.SetInsertPoint(IncomingBB->getTerminator());
        }

        Value *Replacement = materializeConstant(B, UI.OuterC, GV, Alloca);
        UI.U->set(Replacement);
      }
    }

    return true;
  }

  // ============================================================================
  // Function pointer PIC transformation
  // ============================================================================

  /// Emit PC-relative inline asm to compute a function's address at runtime.
  /// On i386 and armv7a, explicit asm is needed because the backends lack
  /// PC-relative addressing modes suitable for arbitrary symbol references.
  /// On x86_64, aarch64, and riscv, a register barrier suffices because the
  /// backend already generates PC-relative code (LEA, ADR, LLA) when the
  /// symbol has hidden visibility.
  static Value *emitPCRelativeFuncRef(IRBuilder<> &B, Function *F,
                                      const Triple &TT)
  {
    LLVMContext &Ctx = B.getContext();
    PointerType *PtrTy = PointerType::get(Ctx, 0);

    if (TT.getArch() == Triple::x86)
    {
      // i386: call/pop to get EIP, then leal for PC-relative offset.
      // The "s" constraint accepts a relocatable symbol reference (unlike "i"
      // which requires a link-time immediate and fails in PIE/LTO builds).
      // ${1:c} strips the $ prefix to get the raw symbol name.
      FunctionType *AsmTy = FunctionType::get(PtrTy, {PtrTy}, false);
      InlineAsm *Asm = InlineAsm::get(
          AsmTy,
          "call 1f\n\t1: popl $0\n\tleal ${1:c}-1b($0), $0",
          "=&r,s", /*hasSideEffects=*/true, /*isAlignStack=*/false);
      return B.CreateCall(Asm, {F}, "func.pic");
    }

    if (TT.getArch() == Triple::arm || TT.getArch() == Triple::thumb)
    {
      // ARM/Thumb: embed the PC-relative offset inline to avoid literal-pool
      // range limits that arise under LTO.  Load the offset from a .word placed
      // right after the sequence, then add PC.  The PC bias when reading the PC
      // register differs between ARM mode (+8) and Thumb mode (+4).
      FunctionType *AsmTy = FunctionType::get(PtrTy, {PtrTy}, false);
      const char *AsmStr = (TT.getArch() == Triple::thumb)
                               ? "ldr $0, 1f\n\t"
                                 "0: add $0, $0, pc\n\t"
                                 "b 2f\n\t"
                                 ".p2align 2\n"
                                 "1: .word ${1:c} - 0b - 4\n"
                                 "2:"
                               : "ldr $0, 1f\n\t"
                                 "0: add $0, $0, pc\n\t"
                                 "b 2f\n\t"
                                 ".p2align 2\n"
                                 "1: .word ${1:c} - 0b - 8\n"
                                 "2:";
      InlineAsm *Asm = InlineAsm::get(
          AsmTy, AsmStr,
          "=&r,X", /*hasSideEffects=*/true, /*isAlignStack=*/false);
      return B.CreateCall(Asm, {F}, "func.pic");
    }

    // x86_64, aarch64, riscv, etc.: register barrier.
    // The backend will materialise the address PC-relatively (leaq %rip,
    // adr, lla) given -fvisibility=hidden / -fdirect-access-external-data.
    FunctionType *AsmTy = FunctionType::get(PtrTy, {PtrTy}, false);
    InlineAsm *Barrier = InlineAsm::get(
        AsmTy, "", "=r,0", /*hasSideEffects=*/true, /*isAlignStack=*/false);
    return B.CreateCall(Barrier, {F}, "func.pic");
  }

  /// Scan every instruction in the module for Function* operands that are used
  /// as *values* (not as the direct callee of a call/invoke).  Replace each
  /// such use with a PC-relative inline asm computation so that no absolute
  /// relocations are emitted for function pointers.
  static bool transformFunctionPointerUses(Module &M, const Triple &TT)
  {
    struct FuncPtrUse
    {
      Instruction *UserInst;
      unsigned OpIdx;
      Function *Func;
    };
    SmallVector<FuncPtrUse, 16> Uses;

    for (Function &F : M)
    {
      if (F.isDeclaration())
        continue;
      for (BasicBlock &BB : F)
      {
        for (Instruction &I : BB)
        {
          for (unsigned OpIdx = 0; OpIdx < I.getNumOperands(); ++OpIdx)
          {
            auto *FnRef = dyn_cast<Function>(I.getOperand(OpIdx));
            if (!FnRef || FnRef->isIntrinsic())
              continue;

            // Skip the callee operand of a direct call/invoke — the call
            // instruction itself is already PC-relative.
            if (auto *CB = dyn_cast<CallBase>(&I))
            {
              if (&I.getOperandUse(OpIdx) == &CB->getCalledOperandUse())
                continue;
            }

            Uses.push_back({&I, OpIdx, FnRef});
          }
        }
      }
    }

    if (Uses.empty())
      return false;

    errs() << "pic-transform: [phase 1] transforming " << Uses.size()
           << " function pointer reference(s) to PC-relative asm\n";

    // Log unique functions referenced
    SmallPtrSet<Function *, 8> UniqueFuncs;
    for (auto &U : Uses)
      UniqueFuncs.insert(U.Func);
    errs() << "pic-transform: [phase 1] " << UniqueFuncs.size()
           << " unique function(s) referenced as values:";
    for (Function *F : UniqueFuncs)
      errs() << " " << F->getName();
    errs() << "\n";

    for (auto &U : Uses)
    {
      IRBuilder<> B(U.UserInst);

      // PHI operands must be materialised in the corresponding predecessor
      // block, not at the PHI itself.
      if (auto *PHI = dyn_cast<PHINode>(U.UserInst))
      {
        BasicBlock *IncomingBB = PHI->getIncomingBlock(U.OpIdx);
        B.SetInsertPoint(IncomingBB->getTerminator());
      }

      Value *PICAddr = emitPCRelativeFuncRef(B, U.Func, TT);
      U.UserInst->setOperand(U.OpIdx, PICAddr);
    }

    return true;
  }

} // anonymous namespace

// ============================================================================
// Pass entry point
// ============================================================================

PreservedAnalyses PICTransformPass::run(Module &M, ModuleAnalysisManager &MAM)
{
  const DataLayout &DL = M.getDataLayout();
  Triple TT(M.getTargetTriple());
  bool Changed = false;

  errs() << "pic-transform: running on module '" << M.getName() << "'\n";
  errs() << "pic-transform: target triple: " << TT.str() << "\n";
  errs() << "pic-transform: pointer size: " << DL.getPointerSizeInBits()
         << " bits\n";

  // ── Collect transformable globals ─────────────────────────────────────
  struct GlobalInfo
  {
    GlobalVariable *GV;
    SmallVector<uint8_t, 256> Bytes;
  };
  std::vector<GlobalInfo> Globals;
  std::vector<GlobalVariable *> AggregateGlobals;

  unsigned TotalGlobals = 0;
  unsigned SkippedGlobals = 0;
  uint64_t TotalBytes = 0;

  for (GlobalVariable &GV : M.globals())
  {
    ++TotalGlobals;
    if (!isTransformableConstant(GV))
      continue;

    GlobalInfo Info;
    Info.GV = &GV;
    if (!extractConstantBytes(DL, GV.getInitializer(), Info.Bytes))
    {
      // ConstantAggregates with pointer elements (e.g., arrays of pointers)
      // cannot be byte-serialized but can be structurally reconstructed.
      if (isa<ConstantAggregate>(GV.getInitializer()))
      {
        errs() << "pic-transform: '" << GV.getName() << "' ("
               << *GV.getValueType()
               << ") will use structural replacement\n";
        AggregateGlobals.push_back(&GV);
        continue;
      }
      errs() << "pic-transform: warning: cannot serialize '"
             << GV.getName() << "' (" << *GV.getValueType()
             << ") -- skipping\n";
      ++SkippedGlobals;
      continue;
    }
    TotalBytes += Info.Bytes.size();
    Globals.push_back(std::move(Info));
  }

  errs() << "pic-transform: scanned " << TotalGlobals << " global(s), "
         << Globals.size() << " transformable, "
         << AggregateGlobals.size() << " structural, "
         << SkippedGlobals << " skipped (" << TotalBytes << " bytes total)\n";

  // ── Phase 0: Replace inline ConstantFP uses with integer bitcasts ──────
  // The x86/ARM backends emit floating-point constants as loads from constant
  // pools in .rodata even when the value is an IR-level inline constant.
  // To prevent this, we replace:
  //   %x = fadd double %a, 3.14159
  // with:
  //   %imm = call i64 asm sideeffect "", "=r,0"(i64 0x400921FB54442D11)
  //   %fp = bitcast i64 %imm to double
  //   %x = fadd double %a, %fp
  //
  // On 32-bit targets (i386, armv7), i64 does not fit in a single register,
  // so the "=r,0" barrier is invalid for 64-bit values.  Instead we split
  // the constant into two 32-bit halves, barrier each half, store both to
  // a stack slot, and load back as double.
  unsigned PtrSize = DL.getPointerSizeInBits();
  unsigned FPConstCount = 0;
  unsigned FPNarrowCount = 0;
  unsigned FPWideCount = 0;

  for (Function &F : M)
  {
    if (F.isDeclaration())
      continue;
    for (BasicBlock &BB : F)
    {
      for (auto II = BB.begin(); II != BB.end(); ++II)
      {
        Instruction *I = &*II;
        for (unsigned OpIdx = 0; OpIdx < I->getNumOperands(); ++OpIdx)
        {
          auto *CFP = dyn_cast<ConstantFP>(I->getOperand(OpIdx));
          if (!CFP)
            continue;

          Type *FPTy = CFP->getType();
          unsigned BitWidth = FPTy->getPrimitiveSizeInBits();
          if (BitWidth != 32 && BitWidth != 64)
            continue;

          APInt Bits = CFP->getValueAPF().bitcastToAPInt();

          IRBuilder<> B(I);

          // PHI operands must be materialised in the corresponding
          // predecessor block, not at the PHI itself — otherwise the
          // inserted instructions break the PHI-grouping invariant.
          if (auto *PHI = dyn_cast<PHINode>(I))
          {
            BasicBlock *IncomingBB = PHI->getIncomingBlock(OpIdx);
            B.SetInsertPoint(IncomingBB->getTerminator());
          }

          Value *FPVal;
          if (BitWidth > PtrSize)
          {
            // Wide constant on narrow target (e.g., double on i386/armv7).
            // Split into register-sized halves, barrier each, store to
            // stack, load as the float type.
            BasicBlock &Entry = F.getEntryBlock();
            IRBuilder<> AllocaB(&Entry, Entry.getFirstInsertionPt());
            AllocaInst *Alloca = AllocaB.CreateAlloca(
                FPTy, nullptr, "fp.slot");
            Alloca->setAlignment(Align(BitWidth / 8));

            // Anchor the alloca in LiveStacks (see Phase 2 comment).
            AllocaB.CreateLifetimeStart(Alloca);

            Type *I8Ty = B.getInt8Ty();
            unsigned WordBytes = DL.getPointerSize();
            Type *WordTy = IntegerType::get(M.getContext(), WordBytes * 8);
            unsigned NumWords = BitWidth / 8 / WordBytes;

            for (unsigned W = 0; W < NumWords; ++W)
            {
              uint64_t Word = Bits.extractBitsAsZExtValue(
                  WordBytes * 8, W * WordBytes * 8);
              Value *WordVal = emitRegisterBarrier(
                  B, ConstantInt::get(WordTy, Word));
              Value *Ptr = B.CreateConstInBoundsGEP1_64(
                  I8Ty, Alloca, W * WordBytes);
              StoreInst *SI = B.CreateAlignedStore(WordVal, Ptr, Align(1));
              SI->setVolatile(true);
            }

            FPVal = B.CreateAlignedLoad(
                FPTy, Alloca, Align(BitWidth / 8), "fp.imm");
            ++FPWideCount;
          }
          else
          {
            // Constant fits in a register — simple barrier + bitcast.
            Type *IntTy = IntegerType::get(M.getContext(), BitWidth);
            Value *IntVal = emitRegisterBarrier(
                B, ConstantInt::get(IntTy, Bits));
            FPVal = B.CreateBitCast(IntVal, FPTy, "fp.imm");
            ++FPNarrowCount;
          }

          I->setOperand(OpIdx, FPVal);
          ++FPConstCount;
          Changed = true;
        }
      }
    }
  }

  if (FPConstCount > 0)
  {
    errs() << "pic-transform: [phase 0] replaced " << FPConstCount
           << " floating-point constant(s) ("
           << FPNarrowCount << " register-width, "
           << FPWideCount << " split-width)\n";
  }
  else
  {
    errs() << "pic-transform: [phase 0] no floating-point constants found\n";
  }

  // ── Phase 0b: Lower fneg, uitofp, and fptoui for double/i64 ─────────────
  // The x86 backend (both i386 and x86_64) emits constant-pool entries in
  // .rdata/.rodata when lowering:
  //   - fneg double       (XOR with sign-mask constant)
  //   - uitofp i64→double (magic-number uint-to-double conversion)
  //   - fptoui double→i64 (comparison with 2^63 constant)
  // Replace these with equivalent IR that avoids constant-pool generation.
  // sitofp and fptosi do NOT generate constant pools (they use cvtsi2sd /
  // cvttsd2si on x86_64, or fildll / fistpll on i386).
  if (TT.isX86())
  {
    SmallVector<Instruction *, 32> ToReplace;
    unsigned FNegCount = 0;
    unsigned UIToFPCount = 0;
    unsigned FPToUICount = 0;

    for (Function &F : M)
    {
      if (F.isDeclaration())
        continue;
      for (BasicBlock &BB : F)
      {
        for (auto &I : BB)
        {
          if (auto *UN = dyn_cast<UnaryOperator>(&I))
          {
            if (UN->getOpcode() == Instruction::FNeg &&
                UN->getType()->isDoubleTy())
            {
              ToReplace.push_back(&I);
              ++FNegCount;
            }
            continue;
          }
          if (isa<UIToFPInst>(&I) &&
              I.getType()->isDoubleTy() &&
              I.getOperand(0)->getType()->isIntegerTy(64))
          {
            ToReplace.push_back(&I);
            ++UIToFPCount;
          }
          if (isa<FPToUIInst>(&I) &&
              I.getOperand(0)->getType()->isDoubleTy() &&
              I.getType()->isIntegerTy(64))
          {
            ToReplace.push_back(&I);
            ++FPToUICount;
          }
        }
      }
    }

    if (!ToReplace.empty())
    {
      errs() << "pic-transform: [phase 0b] lowering " << ToReplace.size()
             << " x86 constant-pool instruction(s): "
             << FNegCount << " fneg, "
             << UIToFPCount << " uitofp, "
             << FPToUICount << " fptoui\n";
    }
    else
    {
      errs() << "pic-transform: [phase 0b] no x86 constant-pool instructions found\n";
    }

    Type *I32Ty = IntegerType::get(M.getContext(), 32);
    Type *I64Ty = IntegerType::get(M.getContext(), 64);
    Type *DblTy = Type::getDoubleTy(M.getContext());
    Type *I8Ty = Type::getInt8Ty(M.getContext());

    // Helper: materialise a barriered i64 constant.
    // On 64-bit targets the value fits in one register; on 32-bit targets
    // we split into two i32 barriers and combine.
    auto makeI64 = [&](IRBuilder<> &B, uint64_t Val) -> Value *
    {
      if (PtrSize >= 64)
        return emitRegisterBarrier(B, ConstantInt::get(I64Ty, Val));
      Value *Lo = B.CreateZExt(
          emitRegisterBarrier(B, ConstantInt::get(I32Ty, (uint32_t)Val)),
          I64Ty);
      Value *Hi = B.CreateZExt(
          emitRegisterBarrier(B, ConstantInt::get(I32Ty, (uint32_t)(Val >> 32))),
          I64Ty);
      return B.CreateOr(Lo, B.CreateShl(Hi, 32));
    };

    // Helper: materialise a barriered double constant from its bit pattern.
    auto makeDbl = [&](IRBuilder<> &B, uint64_t Bits) -> Value *
    {
      return B.CreateBitCast(makeI64(B, Bits), DblTy, "dbl.imm");
    };

    for (Instruction *I : ToReplace)
    {
      IRBuilder<> B(I);

      // ── fneg double ───────────────────────────────────────────────
      if (auto *UN = dyn_cast<UnaryOperator>(I))
      {
        // XOR sign bit: bitcast to i64, XOR 0x8000000000000000, bitcast back
        Value *Bits = B.CreateBitCast(UN->getOperand(0), I64Ty);
        Value *Mask = makeI64(B, 0x8000000000000000ULL);
        Value *Flipped = B.CreateXor(Bits, Mask);
        Value *Result = B.CreateBitCast(Flipped, DblTy, "fneg.res");
        I->replaceAllUsesWith(Result);
        I->eraseFromParent();
        Changed = true;
        continue;
      }

      // ── uitofp i64 → double ───────────────────────────────────────
      if (isa<UIToFPInst>(I))
      {
        // Decompose: sitofp + conditional add of 2^64.
        // sitofp i64→double doesn't use constant pools (cvtsi2sd / fildll).
        // If the original value had the sign bit set, sitofp interprets it
        // as negative, so we add 2^64 (= 0x43F0000000000000) to correct.
        Value *IntVal = I->getOperand(0);
        Value *AsSigned = B.CreateSIToFP(IntVal, DblTy, "uitofp.signed");
        Value *IsNeg = B.CreateICmpSLT(IntVal, ConstantInt::get(I64Ty, 0));
        // 2^64 as double
        Value *Corr = makeDbl(B, 0x43F0000000000000ULL);
        // 0.0 as double (all-zero bits)
        Value *Zero = B.CreateBitCast(makeI64(B, 0), DblTy);
        Value *AddVal = B.CreateSelect(IsNeg, Corr, Zero);
        Value *Result = B.CreateFAdd(AsSigned, AddVal, "uitofp.res");
        I->replaceAllUsesWith(Result);
        I->eraseFromParent();
        Changed = true;
        continue;
      }

      // ── fptoui double → i64 ───────────────────────────────────────
      if (isa<FPToUIInst>(I))
      {
        // Decompose: if val >= 2^63, subtract 2^63 as double, fptosi,
        // then add 2^63 as integer.  Otherwise just fptosi directly.
        // fptosi double→i64 doesn't use constant pools (cvttsd2si / fistpll).
        Value *DblVal = I->getOperand(0);
        // 2^63 as double = 0x43E0000000000000
        Value *Pow2_63 = makeDbl(B, 0x43E0000000000000ULL);
        Value *IsLarge = B.CreateFCmpOGE(DblVal, Pow2_63);
        Value *Adjusted = B.CreateFSub(DblVal, Pow2_63);
        Value *ToConvert = B.CreateSelect(IsLarge, Adjusted, DblVal);
        Value *Signed = B.CreateFPToSI(ToConvert, I64Ty, "fptoui.si");
        Value *Fixup = B.CreateSelect(
            IsLarge,
            makeI64(B, 0x8000000000000000ULL),
            ConstantInt::get(I64Ty, 0));
        Value *Result = B.CreateAdd(Signed, Fixup, "fptoui.res");
        I->replaceAllUsesWith(Result);
        I->eraseFromParent();
        Changed = true;
        continue;
      }
    }
  }
  else
  {
    errs() << "pic-transform: [phase 0b] skipped (non-x86 target)\n";
  }

  // ── Phase 1: Replace function pointer values with PC-relative asm ───────
  // When a Function* is used as a value (passed as callback, stored, etc.)
  // rather than as a direct call target, the backend may emit an absolute
  // address or GOT load.  Replace with architecture-specific inline asm
  // that computes the address PC-relatively.
  if (transformFunctionPointerUses(M, TT))
    Changed = true;
  else
    errs() << "pic-transform: [phase 1] no function pointer references found\n";

  if (Globals.empty() && AggregateGlobals.empty() && !Changed)
  {
    errs() << "pic-transform: no transformations needed, module unchanged\n";
    return PreservedAnalyses::all();
  }

  // ── Phase 2: Replace global constants with stack immediates ─────────────
  errs() << "pic-transform: [phase 2] transforming " << Globals.size()
         << " global constant(s) to stack immediates ("
         << TotalBytes << " bytes), "
         << AggregateGlobals.size()
         << " aggregate(s) structurally\n";

  unsigned ReplacedUses = 0;
  for (Function &F : M)
  {
    if (F.isDeclaration())
      continue;

    // Structural aggregates first: their element-wise stores may reference
    // other globals (e.g., string literals).  Processing them first ensures
    // that when those globals are replaced below, collectUsesInFunction
    // finds and patches the store instructions created here.
    for (auto *GV : AggregateGlobals)
    {
      if (GV && replaceAggregateGlobalInFunction(F, *GV, DL))
      {
        ++ReplacedUses;
        Changed = true;
      }
    }

    for (auto &Info : Globals)
    {
      if (replaceGlobalInFunction(F, *Info.GV, Info.Bytes, DL))
      {
        ++ReplacedUses;
        Changed = true;
      }
    }
  }

  errs() << "pic-transform: [phase 2] replaced uses in " << ReplacedUses
         << " function-global pair(s)\n";

  // ── Cleanup: delete now-unused globals ────────────────────────────────
  // Iterate until stable because erasing one global may release constant
  // users that kept another alive.
  unsigned DeletedGlobals = 0;
  bool Progress = true;
  while (Progress)
  {
    Progress = false;
    for (auto &Info : Globals)
    {
      if (!Info.GV)
        continue;
      Info.GV->removeDeadConstantUsers();
      if (Info.GV->use_empty())
      {
        errs() << "pic-transform: [cleanup] deleted global '"
               << Info.GV->getName() << "'\n";
        Info.GV->eraseFromParent();
        Info.GV = nullptr;
        ++DeletedGlobals;
        Changed = true;
        Progress = true;
      }
    }
    for (auto *&GV : AggregateGlobals)
    {
      if (!GV)
        continue;
      GV->removeDeadConstantUsers();
      if (GV->use_empty())
      {
        errs() << "pic-transform: [cleanup] deleted aggregate global '"
               << GV->getName() << "'\n";
        GV->eraseFromParent();
        GV = nullptr;
        ++DeletedGlobals;
        Changed = true;
        Progress = true;
      }
    }
  }

  // Warn about remaining globals
  unsigned ResidualGlobals = 0;
  for (GlobalVariable &GV : M.globals())
  {
    if (GV.isDeclaration() || GV.hasSection())
      continue;
    if (GV.getName().starts_with("llvm.") ||
        GV.getName().starts_with("__llvm"))
      continue;
    GV.removeDeadConstantUsers();
    if (!GV.use_empty())
    {
      errs() << "pic-transform: warning: '" << GV.getName()
             << "' (" << *GV.getValueType()
             << ") could not be eliminated -- may create data section\n";
      ++ResidualGlobals;
    }
  }

  // ── Summary ───────────────────────────────────────────────────────────
  unsigned TotalTransformed = Globals.size() + AggregateGlobals.size();
  errs() << "pic-transform: summary: "
         << FPConstCount << " FP constants, "
         << TotalTransformed << " globals ("
         << AggregateGlobals.size() << " structural, "
         << DeletedGlobals << " deleted, "
         << (TotalTransformed - DeletedGlobals) << " retained), "
         << ResidualGlobals << " residual warning(s)\n";

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

// ============================================================================
// Plugin registration (only when building as loadable plugin)
// ============================================================================

#ifndef PIC_TRANSFORM_STANDALONE

#include "llvm/Config/llvm-config.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

static void registerPICTransformPass(PassBuilder &PB)
{
  // LLVM 20-21: callback has (MPM, OL, LTOPhase) signature
  // LLVM 22+:   callback has (MPM, OL) signature
  PB.registerOptimizerLastEPCallback(
      [](ModulePassManager &MPM, OptimizationLevel OL
#if LLVM_VERSION_MAJOR < 22
         ,
         ThinOrFullLTOPhase
#endif
      )
      {
        MPM.addPass(PICTransformPass());
      });
  PB.registerPipelineStartEPCallback(
      [](ModulePassManager &MPM, OptimizationLevel OL)
      {
        if (OL == OptimizationLevel::O0)
          MPM.addPass(PICTransformPass());
      });
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo()
{
  return {LLVM_PLUGIN_API_VERSION, "PICTransform", LLVM_VERSION_STRING,
          registerPICTransformPass};
}

#endif // PIC_TRANSFORM_STANDALONE

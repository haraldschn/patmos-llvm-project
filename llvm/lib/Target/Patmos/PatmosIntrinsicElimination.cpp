//===-- PatmosSPBundling.cpp - Remove unused function declarations ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass makes the single-pat code utilitize Patmos' dual issue pipeline.
// TODO: more description
//
//===----------------------------------------------------------------------===//

#include "PatmosIntrinsicElimination.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

#define DEBUG_TYPE "patmos-intrinsic-elimination"

char PatmosIntrinsicElimination::ID = 0;

FunctionPass *llvm::createPatmosIntrinsicEliminationPass() {
  return new PatmosIntrinsicElimination();
}

bool PatmosIntrinsicElimination::runOnFunction(Function &F) {
  errs() << "PatmosIntrinsicElimination\n";

  BB_loop:
  for(auto BB_iter = F.begin(), BB_iter_end = F.end(); BB_iter != BB_iter_end; ++BB_iter){
    auto &BB = *BB_iter;
    instr_loop:
    for(auto instr_iter = BB.begin(), instr_iter_end = BB.end(); instr_iter != instr_iter_end; ++instr_iter){
      auto &instr = *instr_iter;

      if(instr.getOpcode() == Instruction::Call || instr.getOpcode() == Instruction::CallBr) {
        if (IntrinsicInst *II = dyn_cast<IntrinsicInst>(&instr)) {
          switch (II->getIntrinsicID()) {
          case Intrinsic::memcpy:
            errs() << "Found memcpy: "; instr.dump();
            break;
          case Intrinsic::memset: {
            errs() << "Found memset: "; instr.dump();
            assert(II->arg_size() >= 3); // We don't care about the volatile flag (4th arg)
            auto arg0 = II->getArgOperand(0);
            auto arg1 = II->getArgOperand(1);
            auto arg2 = II->getArgOperand(2);

            assert(cast<PointerType>(arg0->getType())->getAddressSpace() == 0);
            assert(arg0->getType()->getContainedType(0)->isIntegerTy(8));
            assert(arg1->getType()->isIntegerTy(8));
            assert(arg2->getType()->isIntegerTy(32) || arg2->getType()->isIntegerTy(64));

            if(auto* memset_len = dyn_cast<ConstantInt>(arg2)) {
              errs() << "arg2:" << memset_len->getValue() << "\n";
              IRBuilder<> builder(F.getContext());
              auto len = memset_len->getValue().getLimitedValue(std::numeric_limits<uint32_t>::max());

              if(len == std::numeric_limits<uint32_t>::max())
                report_fatal_error("llvm.memset length argument is too large");

              if(len <= 12) continue; // Too small to be worth it

              auto loop_bound = len/4;
              auto epilogue_len = len % 4;

              auto *memset_entry = BasicBlock::Create(F.getContext(), "memset.entry", &F);
              auto *memset_loop_cond = BasicBlock::Create(F.getContext(), "memset.loop.cond", &F);
              auto *memset_loop_body = BasicBlock::Create(F.getContext(), "memset.loop.body", &F);
              auto *memset_loop_end = BasicBlock::Create(F.getContext(), "memset.loop.end", &F);

              // Prepare i32 version of value
              Value *val_i32_done;
              if(auto* set_to_const = dyn_cast<ConstantInt>(arg1)) {
                auto set_to_val = set_to_const->getValue().getLimitedValue(std::numeric_limits<uint16_t>::max());
                assert(set_to_val <= std::numeric_limits<uint8_t>::max() &&
                    "memset value to set to is out of range");

                auto set_to_shl8 = set_to_val << 8;
                auto set_to_half = set_to_shl8 + set_to_val;
                auto set_to_upper = set_to_half << 16;
                val_i32_done = builder.getInt32(set_to_upper + set_to_half);
              } else {
                auto* val_i32 = (Instruction*)builder.CreateZExt(arg1, Type::getInt32Ty(F.getContext()));
                memset_entry->getInstList().insert(memset_entry->getInstList().begin(), val_i32);
                auto* val_shl8 = (Instruction*)builder.CreateShl(val_i32, builder.getInt32(8));
                memset_entry->getInstList().insert(memset_entry->getInstList().end(), val_shl8);
                auto* val_halfword = (Instruction*)builder.CreateAdd(val_shl8, val_i32);
                memset_entry->getInstList().insert(memset_entry->getInstList().end(), val_halfword);
                auto* val_upper = (Instruction*)builder.CreateShl(val_halfword, builder.getInt32(16));
                memset_entry->getInstList().insert(memset_entry->getInstList().end(), val_upper);
                auto *val_i32_done_instr = (Instruction*)builder.CreateAdd(val_halfword, val_upper, "memset.set.to.word");
                memset_entry->getInstList().insert(memset_entry->getInstList().end(), val_i32_done_instr);
                val_i32_done = val_i32_done_instr;
              }

              auto *dest_i32 = (Instruction*)builder.CreateBitCast(arg0, PointerType::get(builder.getInt32Ty(),0), "memset.dest.i32");
              memset_entry->getInstList().insert(memset_entry->getInstList().end(), dest_i32);

              BranchInst::Create(memset_loop_cond, memset_entry);

              auto *i_phi = PHINode::Create(builder.getInt32Ty(), 2, "memset.i");
              memset_loop_cond->getInstList().insert(memset_loop_cond->getInstList().begin(), i_phi);
              i_phi->addIncoming(builder.getInt32(loop_bound), memset_entry);

              auto *i_cmp = (Instruction*)builder.CreateICmpEQ(i_phi, ConstantInt::get(builder.getInt32Ty(), 0), "memset.loop.finished");
              memset_loop_cond->getInstList().insert( memset_loop_cond->getInstList().end(), i_cmp );

              auto *cond_br = BranchInst::Create(memset_loop_end, memset_loop_body, i_cmp, memset_loop_cond);

              const char *MetadataName = "llvm.loop.bound";
              llvm::MDString *Name = llvm::MDString::get(F.getContext(), MetadataName);
              SmallVector<llvm::Metadata *, 3> OpValues;
              OpValues.push_back(Name);
              OpValues.push_back(llvm::ValueAsMetadata::get(builder.getInt32(loop_bound)));
              OpValues.push_back(llvm::ValueAsMetadata::get(builder.getInt32(loop_bound)));

              SmallVector<llvm::Metadata *, 2> Metadata(1);
              Metadata.push_back(llvm::MDNode::get(F.getContext(), OpValues));
              llvm::MDNode *LoopID = llvm::MDNode::get(F.getContext(), Metadata);
              LoopID->replaceOperandWith(0, LoopID); // First op points to itself.

              cond_br->setMetadata("llvm.loop", LoopID);

              auto *dest_phi = PHINode::Create(PointerType::get(builder.getInt32Ty(),0), 2, "memset.dest");
              memset_loop_cond->getInstList().insert(memset_loop_cond->getInstList().begin(), dest_phi);
              dest_phi->addIncoming(dest_i32, memset_entry);

              auto *dest_inc = (Instruction *)builder.CreateGEP(dest_phi, builder.getInt32(1), "memset.dest.incd");
              memset_loop_body->getInstList().insert(memset_loop_body->getInstList().end(), dest_inc);
              dest_phi->addIncoming(dest_inc, memset_loop_body);

              auto *i_dec = (Instruction *)builder.CreateSub(i_phi, ConstantInt::get(builder.getInt32Ty(), 1), "memset.i.decd");
              memset_loop_body->getInstList().insert(memset_loop_body->getInstList().end(), i_dec);
              i_phi->addIncoming(i_dec, memset_loop_body);

              memset_loop_body->getInstList().insert(memset_loop_body->getInstList().end(),
                  builder.CreateAlignedStore(val_i32_done, dest_phi, MaybeAlign(1))
              );

              BranchInst::Create(memset_loop_cond, memset_loop_body);

              if(epilogue_len) {
                // Need epilogue
                // Just call llvm.memset which would automatically be expanded by LLVM
                auto *dest_phi_i8 = (Instruction*)builder.CreateBitCast(dest_phi, PointerType::get(builder.getInt8Ty(),0), "memset.dest.i32.i8");
                memset_loop_end->getInstList().insert(memset_loop_end->getInstList().end(), dest_phi_i8);

                memset_loop_end->getInstList().insert(memset_loop_end->getInstList().end(),
                    builder.CreateCall(II->getCalledFunction(),
                        {dest_phi_i8, arg1, builder.getInt32(epilogue_len), builder.getFalse()})
                );
              }

              // Replace llvm.memset
              auto *predecessor = BB.splitBasicBlockBefore(instr_iter);

              BranchInst &pred_branch = cast<BranchInst>(predecessor->back());
              pred_branch.setSuccessor(0, memset_entry);

              assert(isa<CallInst>(BB.begin())); // This should be the call to memset
              BB.getInstList().pop_front(); // remove the memset

              BranchInst::Create(&BB, memset_loop_end);

              errs() << "Module after additions: "; F.getParent()->dump();

              // Since we have now messed with the list of blocks the iterators are invalidated
              goto BB_loop; // restart
            } else {
              report_fatal_error("llvm.memset length argument not a constant value");
            }

            break;
          }
          default:
            break;
          }
        }

      }

    }
  }


  return true;
}

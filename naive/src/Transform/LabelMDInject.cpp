
/*
 * Not sure if this is the correct approach. I want something to
 * hold onto IR, but not be a printf or other trace type of thing.
 * Effectively, I want to prune and keep the MD along... so a
 * programmatic tag, rather than the printf tag. So you can more 
 * easily generate the printf trace to line up with the correct
 * labels. Dumb? yes, likely. This is how I do.
 *
 */
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/SymbolTableListTraits.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

#include "LabelMDInject.h"

bool
LabelMDInject::runOnModule(Module &M)
{
	LLVMContext &ctx = M.getContext();

	for (Function &F : M) {
		if (F.isDeclaration() || F.isIntrinsic()) {
			continue;
		}
		if (!F.hasName()) {	
			continue;
		}

		std::string fname = F.getName().str();
		unsigned bbcnt = 0;

		for (BasicBlock &BB : F) {
			++bbcnt;

			std::string blkname = fname + " " + std::to_string(bbcnt); 

			Instruction *before = BB.getFirstNonPHI();

			// BlockAddress not allowed on a entry block
			if (bbcnt == 1) {
				Instruction *dummy = BinaryOperator::CreateNeg(
				  Constant::getIntegerValue(Type::getInt32Ty(ctx),
				  APInt(32, 0, false)),
				  "",
				  before);
				LLVMContext &C = dummy->getContext();
				MDNode *N = MDNode::get(C, MDString::get(C, blkname));
				dummy->setMetadata("fnblk", N);
				continue;
			}

			// Other side of the house, we try to use blockaddress operation.
			BlockAddress *bbAddr = BlockAddress::get(&BB);
			Instruction *ii = new PtrToIntInst(bbAddr, Type::getInt64Ty(ctx), "", before);
			LLVMContext &C = ii->getContext();
			MDNode *N = MDNode::get(C, MDString::get(C, blkname));
			ii->setMetadata("fnblk", N);
		}
	}
	return true;
}

char LabelMDInject::ID = 0;
static RegisterPass<LabelMDInject> XX("pskin-label-inject", 
  "inject dummy instruction and mdnode on it");



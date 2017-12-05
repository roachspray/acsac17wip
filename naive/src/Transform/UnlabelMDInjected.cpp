
/*
 * Undo UnlabelMDInjected.
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

#include "UnlabelMDInjected.h"

bool
UnlabelMDInjected::runOnModule(Module &M)
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

			// Should probably assert each has MDNode oinfo of interest, shrug.

			// BlockAddress not allowed on a entry block 
			if (bbcnt == 1) {
				if (!isa<BinaryOperator>(before)) {
					errs() << "First BB instruction is not BinaryOperator! Uh Oh.\n";
					F.dump();
					return true;
				}
				before->eraseFromParent();
				continue;
			}
				
			before->eraseFromParent();
		}
	}
	return true;
}

char UnlabelMDInjected::ID = 0;
static RegisterPass<UnlabelMDInjected> XX("pskin-label-remove", 
  "Remove these dummies");



#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"


using namespace llvm;

#include "NoPredBlocks.h"

void
NoPredBlocks::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesCFG();
}

bool
NoPredBlocks::runOnModule(Module &M)
{
	unsigned long long total_blocks = 0;
	unsigned long long blocks_without_predecessors = 0;
	unsigned long long blocks_without_successors = 0;
	unsigned long long island_blocks = 0;

	for (Function &F : M) {
		bool entryBlock = true;
		for (BasicBlock &B : F) {

			++total_blocks;

			bool hasSuccessor = false;
			if (succ_begin(&B) != succ_begin(&B)) {
				hasSuccessor = true;
			} else {
				++blocks_without_successors;
			}

			bool hasPredecessor = false;
			if (pred_begin(&B) != pred_end(&B)) {
				hasPredecessor = true;
			} else {
				++blocks_without_predecessors;
			}
			if (hasSuccessor == false && hasPredecessor == false) {
				if (entryBlock == false) {
					++island_blocks;
				}
			}
			entryBlock = false;
		}
	}
	errs() << "Total blocks: " << total_blocks << "\n";
	errs() << "Island blocks: " << island_blocks << "\n";
	errs() << "Blocks w/o preds: " << blocks_without_predecessors << "\n";
	errs() << "Blocks w/o successors: " << blocks_without_successors << "\n";
	return true;
}

char NoPredBlocks::ID = 0;
static RegisterPass<NoPredBlocks> XX("no-pred-blocks", "");

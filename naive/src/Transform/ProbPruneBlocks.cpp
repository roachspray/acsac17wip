/*
 * So, the idea is that we have a list of blocks we 
 * would like to remove, but then the selection of
 * which attempt to be removed is probabilistic. 
 *
 * We just, when adding to the list, say that we have
 * a p.d.f. that is a function of call depth. We
 * can put in whatever p.d.f. we want... set by 
 * cmd line. oh, maybe a function of block count
 * too.
 * 
 */
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/SymbolTableListTraits.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

// Like i said.. ugly as fuhhhh
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <set>

using namespace llvm;

#include "../TraceUtils.h"
#include "ProbPruneBlocks.h"
#include "PruneHelp.h"

bool
ProbPruneBlocks::runOnModule(Module &M)
{
	mod = &M;
//	M.dump();

	traceEntries = TraceUtils::ParseTraceFile(traceFile);	
	if (traceEntries.size() == 0) {
		errs() << "No trace entries found in file: " << traceFile << ".\n";
		return false;
	}

	unsigned dep = pruneDepth;

	do {	
		if (setFocusFunc(dep) == false) {
			errs() << "<PB> Adjusting pruning call depth\n";
			--dep;
			continue;
		}
		errs() << "<PB> Prune call depth: " << dep << "\n";
		errs() << "<PB> Function being pruned: " << focusFunction << "\n";

		keepBlocks.clear();
		removeBlocks.clear();
		parseFocusFuncForBlocks(dep);
		
		for (auto rbi = removeBlocks.begin(); rbi != removeBlocks.end();
		  ++rbi) {
			BBlockRef *bbRef = const_cast<BBlockRef *>(&*rbi);
			BasicBlock *bb = bbRef->getBB();
	
			bool unhandledTerminator = false;
			for (auto pi = pred_begin(bb); pi != pred_end(bb); ++pi) {
				BasicBlock *predBB = *pi;
				TerminatorInst *ti = predBB->getTerminator();

				if (BranchInst *bi = dyn_cast<BranchInst>(ti)) {
					std::set<Function *> f;
					PruneHelp::tryCleanseBranchInst(bbRef, predBB, bi,
					  removeBlocks, f);
	
				} else if (IndirectBrInst *bi = dyn_cast<IndirectBrInst>(ti)) {
					PruneHelp::tryCleanseIndirectBrInst(bbRef, bi,
					  removeBlocks);
	
				} else if (CatchReturnInst *r = dyn_cast<CatchReturnInst>(ti)) {
					PruneHelp::tryCleanseCatchReturnInst(bbRef, r,
					  removeBlocks);
		
				} else if (CatchSwitchInst *r = dyn_cast<CatchSwitchInst>(ti)) {
					PruneHelp::tryCleanseCatchSwitchInst(bbRef, r,
					  removeBlocks);
		
				} else if (SwitchInst *si = dyn_cast<SwitchInst>(ti)) {
					PruneHelp::tryCleanseSwitchInst(bbRef, si, removeBlocks);
	
				} else if (InvokeInst *ini = dyn_cast<InvokeInst>(ti)) {
					PruneHelp::tryCleanseInvokeInst(bbRef, ini, removeBlocks);
	
				} else if (CleanupReturnInst *cri =  \
				  dyn_cast<CleanupReturnInst>(ti)) {
					PruneHelp::tryCleanseCleanupReturnInst(bbRef, cri,
					  removeBlocks);
	
				} else {
					errs() << "Unhandled terminator: \n";
	//				ti->dump();
	//				bb->dump();
	//				unhandledTerminator = true;
					break;
				}
				if (unhandledTerminator) {
					break;
				}
			}
			if (unhandledTerminator == false) {
				if (pred_begin(bb) == pred_end(bb)) {
					DeleteDeadBlock(bb);
					bbRef->removed = true;
				} else {
//					errs() << "Has preds still\n";
				}
			}
		}
		/* final testeroonie */
		for (auto rbi = removeBlocks.begin(); rbi != removeBlocks.end();
		  ++rbi) {
			BBlockRef *bbRef = const_cast<BBlockRef *>(&*rbi);
			BasicBlock *bb = bbRef->getBB();
			if (bbRef->removed == false && pred_begin(bb) == pred_end(bb)) {
		//		errs() << "Actually no preds and wasn't deleted\n";
				DeleteDeadBlock(bb);
				bbRef->removed = true;
			}
		}
		for (auto rbi = keepBlocks.begin(); rbi != keepBlocks.end(); ++rbi) {
			BBlockRef *bbRef = const_cast<BBlockRef *>(&*rbi);
			BasicBlock *bb = bbRef->getBB();
			if (bbRef->idx != 1 && pred_begin(bb) == pred_end(bb)) {
				errs() << "Think to drop:\n";
	//				DeleteDeadBlock(bb);
				bb->dump();
			}
		}
	} while (dep > 0);
	return true;
}

bool
ProbPruneBlocks::setFocusFunc(unsigned dep)
{
	std::string nfocusFunction = "";
	Function *nfun = NULL;

	/*
	 * I think really, there should be some abstraction above this list
	 * so we can have projections of it into other containers for more
	 * intelligent selections.
	 */
	for (auto e = traceEntries.begin(); e != traceEntries.end(); ++e) {
		const TraceEntry *t = &*e;
		if (t->getDepth() == dep) { 
			nfocusFunction = t->getFunction();

			if (prunedFunctions.end() != prunedFunctions.find(nfocusFunction)) {
				nfocusFunction = "";
				continue;
			}
			nfun = mod->getFunction(nfocusFunction);
			if (nfun == NULL) {
				// Try for the next of same dep.
				errs() << "setFocusFunc: function not found: "  \
				  << nfocusFunction << "\n";
				nfocusFunction = "";
				continue;
			}

			if (nfun->isDeclaration()) {
				errs() << "setFocusFunc: skipping external function: "  \
				  << nfocusFunction << "\n";
				nfun = NULL;
				nfocusFunction = "";
				continue;
			}
			break;
		}
	}
	if (nfocusFunction != "" && nfun != NULL) {
		focusFunction = nfocusFunction;
		fun = nfun;
		prunedFunctions.insert(focusFunction);
		return true;
	}
	return false;
}

void
ProbPruneBlocks::parseFocusFuncForBlocks(unsigned depth)
{
	std::set<unsigned> blkNums;

	// Project to focusfunction, blocks
	for (auto e = traceEntries.begin(); e != traceEntries.end(); ++e) {
		const TraceEntry *t = &*e;
		if (t->getFunction() == focusFunction) {
			blkNums.insert(t->getBlock());
			continue;
		}
	}

	unsigned blkCnt = 0;
	for (BasicBlock &BB : *fun) {
		BasicBlock *BBp = &BB;
		++blkCnt;
		if (blkNums.end() != blkNums.find(blkCnt)) {
			keepBlocks.insert(BBlockRef(BBp, blkCnt));
		} else {
			if (prPrune->shouldPrune(depth, blkCnt)) {
				removeBlocks.insert(BBlockRef(BBp, blkCnt));
			}
		}
	}
}
char ProbPruneBlocks::ID = 0;

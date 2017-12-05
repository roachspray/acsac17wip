/*
 * This is the naive block pruning code.
 * 
 * For some depth, we attempt to aggressively remove all
 * blocks not-executed (i.e., those not found in the 
 * trace log file).
 * 
 * For every block in a function, if it was executed, it
 * is safe from being pruned. If it was not executed, it
 * is going to have it's moment of glory to possibly be
 * pruned.
 *
 * Issues:
 *  - Many.
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
#include "../CommonUtils.h"
#include "PruneBlocks.h"
#include "PruneHelp.h"

#include "PruneStats.h"

static cl::opt<std::string> TraceLogFile("pskin-log-file",
  cl::desc("Specify trace log file"), cl::init("default_trace.log"),
  cl::Required);


static cl::opt<unsigned> PruneDepth("pskin-depth", 
  cl::desc("Specify the maximum depth to prune"), cl::init(3),
  cl::Required);

static cl::opt<std::string> FunctionCheckRetList("pskin-fnlist",
  cl::desc("File for call-checkret branch whitelisting"), cl::init(""));

static cl::opt<std::string> LogRemovedBlocks("pskin-log-block-removal",
  cl::desc("Specify the file to output which blocks are deleted"),
  cl::init("block_removal.log"));

static cl::opt<bool> InteractiveMode("pskin-interactive",
  cl::desc("Optional interactive mode"), cl::init(false));

bool
PruneBlocks::runOnModule(Module &M)
{
	mod = &M;

	ModuleLevelStats m(PruneDepth);
	m.setFunctionCount(std::distance(M.begin(), M.end()));

	traceEntries = TraceUtils::ParseTraceFile(TraceLogFile);	
	if (traceEntries.size() == 0) {
		errs() << "No trace entries found in '" << TraceLogFile << "'\n";
		return false;
	}

	if (FunctionCheckRetList != "") {
		std::ifstream fileHandle(FunctionCheckRetList);
		std::string pcfName;

		while (std::getline(fileHandle, pcfName)) {
			// Not really needed.
			probeCallRetCompareNames.push_back(pcfName);

			Function *fp = M.getFunction(pcfName);
			if (fp != NULL) {
				std::cout << "<PB> Ret-check function: " << pcfName << "\n";
				probeCallRetCompareFunctions.insert(fp);
			}
		}
	}

	std::vector<FocusFunctionStats> focusFunctionStats;

	unsigned dep = PruneDepth;
	unsigned total_removed = 0;
	std::ofstream blkRmLog;
	blkRmLog.open(LogRemovedBlocks);
	std::set<Function *> calledFromRemovedBlocks;
	do {	
		unsigned removed_count = 0;
		/* 
		 * The way this is working, we are looking at all functions
		 * of a certain call depth, then depth - 1, depth - 2, ..., 1.
		 *
		 * And looking at the removable blocks in that function.
		 * 
		 */
		if (setFocusFunc(dep) == false) {
#ifdef	_DEBUG
			errs() << "<PB> Adjusting pruning call depth\n";
#endif
			--dep;
			continue;
		}
		FocusFunctionStats ffStats(currentFocusFunctionName, dep);
#ifdef	_DEBUG
		errs() << "<PB> Prune call depth: " << dep << "\n";
		errs() << "<PB> Function being pruned: " << currentFocusFunctionName << "\n";
#endif

		keepBlocks.clear();
		removeBlocks.clear();
		ffStats.setBlockCount(parseFocusFuncForBlocks());
		ffStats.setWantKeepBlockCount(keepBlocks.size());
		ffStats.setWantRemoveBlockCount(removeBlocks.size());
	
		for (auto rbi = removeBlocks.begin(); rbi != removeBlocks.end();
		  ++rbi) {
			BBlockRef *bbRef = const_cast<BBlockRef *>(&*rbi);
			BasicBlock *bb = bbRef->getBB();
	
			bool unhandledTerminator = false;
			for (auto pi = pred_begin(bb); pi != pred_end(bb); ++pi) {
				BasicBlock *predBB = *pi;
				TerminatorInst *ti = predBB->getTerminator();

				/* Handle BranchInst */	
				if (BranchInst *bi = dyn_cast<BranchInst>(ti)) {
					PruneHelp::tryCleanseBranchInst(bbRef, predBB, bi,
					  removeBlocks, probeCallRetCompareFunctions);
	
				/* Handle IndirectBrInst */	
				} else if (IndirectBrInst *bi = dyn_cast<IndirectBrInst>(ti)) {
					PruneHelp::tryCleanseIndirectBrInst(bbRef, bi,
					  removeBlocks);
	
				/* Handle CatchReturnInst */	
				} else if (CatchReturnInst *cri =  \
				  dyn_cast<CatchReturnInst>(ti)) {
					PruneHelp::tryCleanseCatchReturnInst(bbRef, cri,
					  removeBlocks);
		
				/* Handle CatchSwitchInst */	
				} else if (CatchSwitchInst *cri =  \
				  dyn_cast<CatchSwitchInst>(ti)) {
					PruneHelp::tryCleanseCatchSwitchInst(bbRef, cri,
					  removeBlocks);

				/* Handle SwitchInst */
				} else if (SwitchInst *si = dyn_cast<SwitchInst>(ti)) {
					PruneHelp::tryCleanseSwitchInst(bbRef, si, removeBlocks);

				/* Handle InvokeInst */	
				} else if (InvokeInst *ini = dyn_cast<InvokeInst>(ti)) {
					PruneHelp::tryCleanseInvokeInst(bbRef, ini, removeBlocks);


				/* Handle CleanupReturnInst */	
				} else if (CleanupReturnInst *cri =  \
				  dyn_cast<CleanupReturnInst>(ti)) {
					PruneHelp::tryCleanseCleanupReturnInst(bbRef, cri,
					  removeBlocks);
	
				} else {
					errs() << "Unhandled terminator: \n";
					bb->dump();
					unhandledTerminator = true;
					break;
				}
				if (unhandledTerminator) {
					break;
				}
			}
		}
		for (auto rbi = removeBlocks.begin(); rbi != removeBlocks.end();
		  ++rbi) {

			BBlockRef *bbRef = const_cast<BBlockRef *>(&*rbi);
			BasicBlock *bb = bbRef->getBB();

			if (!bbRef->isRemoved() && pred_begin(bb) == pred_end(bb)) {
				if (bbRef->getFocusIdx() == 1) {
					continue;
				}
				if (succ_begin(bb) != succ_end(bb)) {
					continue;
				}

				for (Instruction &I : *bb) {
					if (isa<CallInst>(I) || isa<InvokeInst>(I)) {
						CallSite cs(&I);
						Function *f = cs.getCalledFunction();
						if (f == NULL) {
#ifdef	_DEBUG
							errs() << "<PB> Called null Function...indirect call UNHANDLED\n";
#endif
							continue;
						}
						calledFromRemovedBlocks.insert(f);
					}
				}
				DeleteDeadBlock(bb);
				bbRef->setRemoved(true);
			}
		}

		/*
		 * With the removal of blocks, we should go through and fix PHINodes
		 * to have valid incoming Values and proper predecessors.
		 */
		bool repairedSome = false;
		do {
			repairedSome = PruneHelp::repairPHINodes(currentFocusFunction, removeBlocks);
		} while (repairedSome == true);

		// XXX Count remove blocks
		for (auto rbi = removeBlocks.begin(); rbi != removeBlocks.end(); ++rbi) {
			BBlockRef *bbRef = const_cast<BBlockRef *>(&*rbi);
			BasicBlock *bb = bbRef->getBB();
			if (bbRef->isRemoved()) {
				blkRmLog << bbRef->getFocusName() << ":" <<  bbRef->getFocusIdx() << "\n";
				removed_count++;
			}
		}
		total_removed += removed_count;

		ffStats.setRemovedBlockCount(removed_count);
		focusFunctionStats.push_back(ffStats);
	} while (dep > 0);
	// End of focus function lookup and prune loop

	blkRmLog.close();

	m.setTotalRemovedBlockCount(total_removed);

	// How is this diff from below call to removeUnusedFunctions?
	unsigned rmFormers = 0;
	for (auto fi = calledFromRemovedBlocks.begin(); fi != calledFromRemovedBlocks.end(); ++fi) {
		Function *formerlyCalled = *fi;
		std::string n = "<noname>";
		if (formerlyCalled->hasName()) {
			n = formerlyCalled->getName().str();
		}
		if (n != "main") {
			continue;
		}
#ifdef	_DEBUG
		Value *v = cast<Value>(formerlyCalled);
		if (v->use_empty()) {
			errs() << "Value->use_empty() ...so ";
		}
#endif
		if (formerlyCalled->user_empty()) {
#ifdef	_DEBUG
            errs() << "removing: " << n << "\n";
#endif
			formerlyCalled->eraseFromParent();
			++rmFormers;
		}
	}
#ifdef	_DEBUG
	errs() << "<PB> Removed " << rmFormers << " no longer called functions from Module.\n";
#endif

	std::set<std::string> leaveFunctions = {
	  "main"
	};
	// This is fairly awful.
	for (auto e = traceEntries.begin(); e != traceEntries.end(); ++e) {
		const TraceEntry *t = &*e;
		std::string s = t->getFunction();
		leaveFunctions.insert(s);	
	}
		
	rmFormers += PruneHelp::removeUnusedFunctions(&M, leaveFunctions);
	m.setTotalRemovedFunctionCount(rmFormers);
//	PruneHelp::removeUnusedBasicBlocks(&M, leaveFunctions);

	m.print();
	for (auto i = focusFunctionStats.begin(); i != focusFunctionStats.end(); ++i) {
		FocusFunctionStats foc  = *i;
		foc.print();
	}
	return true;
}

/*
 * For a given depth, find a function of that depth to set as 
 * currentFocusFunctionName. Put function into list so we can iterate 
 * through each.
 */
bool
PruneBlocks::setFocusFunc(unsigned dep)
{
	std::string nameofFocusFunction = "";
	Function *tmpFun = NULL;

	for (auto e = traceEntries.begin(); e != traceEntries.end(); ++e) {
		const TraceEntry *t = &*e;
		if (t->getDepth() == dep) { 
			nameofFocusFunction = t->getFunction();

			if (prunedFunctions.end() != prunedFunctions.find(nameofFocusFunction)) {
				nameofFocusFunction = "";
				continue;
			}
			tmpFun = mod->getFunction(nameofFocusFunction);
			if (tmpFun == NULL) {
				// Try for the next of same dep.
				errs() << "setFocusFunc: not found: "  \
				  << nameofFocusFunction << "\n";
				nameofFocusFunction = "";
				continue;
			}

			if (tmpFun->isDeclaration() || tmpFun->isIntrinsic()) {
				errs() << "setFocusFunc: skipping external: "  \
				  << nameofFocusFunction << "\n";
				tmpFun = NULL;
				nameofFocusFunction = "";
				continue;
			}
			break;
		}
	}
	if (nameofFocusFunction != "" && tmpFun != NULL) {
		currentFocusFunctionName = nameofFocusFunction;
		currentFocusFunction = tmpFun;
		prunedFunctions.insert(currentFocusFunctionName);
		return true;
	}
	return false;
}

unsigned
PruneBlocks::parseFocusFuncForBlocks()
{
	std::set<unsigned> blkNums;
	bool interactiveFn = false; 

	if (InteractiveMode) {
		std::string qf = "\n<PB> Function: " + currentFocusFunctionName  \
			  + ", interactive? (y/n) ";
		char yn = CommonUtils::askQuestion(qf.c_str(), "yn");
		if (yn == 'y') {
			interactiveFn = true;
		}
	}
		

	/*
	 * Project entries related to focus function to block number.
	 *
	 * Note: This ignores call depth. It just grabs any/all blocks
	 * reached within that function so that they are considered 
	 * blocks to keep.
	 */
	for (auto e = traceEntries.begin(); e != traceEntries.end(); ++e) {
		const TraceEntry *t = &*e;
		if (t->getFunction() == currentFocusFunctionName) {
			blkNums.insert(t->getBlock());
			continue;
		}
	}

	/*
	 * Split blocks of function into those we want to keep and those
	 * we hope to remove, keepBlocks and removeBlocks. We store them
	 * in BBlockRef's for some reason. :-P
	 *
	 */
	unsigned blkCnt = 0;
	for (BasicBlock &BB : *currentFocusFunction) {
		BasicBlock *BBp = &BB;
		++blkCnt;
		if (blkNums.end() != blkNums.find(blkCnt)) {
			keepBlocks.push_back(BBlockRef(BBp, blkCnt));

		} else {
			if (InteractiveMode && interactiveFn) {
				errs() << "<PB> Function: " << currentFocusFunctionName << "\n";
				BBp->dump();
				if (pred_begin(BBp) != pred_end(BBp)) {
					auto pr = pred_begin(BBp);
					BasicBlock *prBB = *pr;
					errs() << "<PB> First predecessor: \n";
					prBB->dump();
				}
				char yn = CommonUtils::askQuestion("\n<PB> Keep block? (y/n) ",
				  "yn");
				if (yn == 'y') {
					keepBlocks.push_back(BBlockRef(BBp, blkCnt));
				} else {
					removeBlocks.push_back(BBlockRef(BBp, blkCnt));
				}	


			// Not interactive:
			} else {
				removeBlocks.push_back(BBlockRef(BBp, blkCnt));

			}
		}
	}
	return blkCnt;
}
char PruneBlocks::ID = 0;
static RegisterPass<PruneBlocks> X("pskin-naive-prune",
  "Most basic");


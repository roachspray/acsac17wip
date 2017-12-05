/*
 * This is effectively a mix of GVizPruneBlocks and GraphBlock :-/
 * It is a bit of a hack in the sense that I am calling out to
 * external binaries for the rendering :-/////...drool.blahcough.
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
#include "llvm/Support/DOTGraphTraits.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Analysis/CFGPrinter.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

// WTF...... 
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <set>

using namespace llvm;

#include "../TraceUtils.h"
#include "../GVizUtils.h"
#include "../CommonUtils.h"
#include "GVizPruneBlocks.h"
#include "PruneHelp.h"

static cl::opt<std::string> TraceLogFile("pskin-log-file",
  cl::desc("Specify trace log file"), cl::init("default_trace.log"),
  cl::Required);

static cl::opt<unsigned> PruneDepth("pskin-depth",
  cl::desc("Specify the maximum depth to prune"), cl::init(3),
  cl::Required);

static cl::opt<std::string> FunctionCheckRetList("pskin-fnlist",
  cl::desc("File for call-checkret branch whitelisting"), cl::init(""));

static cl::opt<bool> InteractiveMode("pskin-interactive",
  cl::desc("Optional interactive mode"), cl::init(false));


bool
GVizPruneBlocks::runOnModule(Module &M)
{
	mod = &M;
//	M.dump();

	traceEntries = TraceUtils::ParseTraceFile(TraceLogFile);	
	if (traceEntries.size() == 0) {
		errs() << "No trace entries found in traceFile="  \
		  << TraceLogFile << ".\n";
		return false;
	}

	unsigned dep = PruneDepth;

	do {	
		if (setFocusFunc(dep) == false) {
			errs() << "<PB> Adjusted pruning call depth level\n";
			--dep;
			continue;
		}

		errs() << "<PB> Prune call depth: " << dep << "\n";
		errs() << "<PB> Function being pruned: " << focusFunction << "\n";

		keepBlocks.clear();
		removeBlocks.clear();

		parseFocusFuncForBlocks();
		
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
	
				} else if (CatchReturnInst *u = dyn_cast<CatchReturnInst>(ti)) {
					PruneHelp::tryCleanseCatchReturnInst(bbRef, u,
					  removeBlocks);
		
				} else if (CatchSwitchInst *u = dyn_cast<CatchSwitchInst>(ti)) {
					PruneHelp::tryCleanseCatchSwitchInst(bbRef, u,
					  removeBlocks);
		
				} else if (SwitchInst *si = dyn_cast<SwitchInst>(ti)) {
					PruneHelp::tryCleanseSwitchInst(bbRef, si, removeBlocks);
	
				} else if (InvokeInst *ini = dyn_cast<InvokeInst>(ti)) {
					PruneHelp::tryCleanseInvokeInst(bbRef, ini, removeBlocks);
	
				} else if (CleanupReturnInst *cri = \
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
				DeleteDeadBlock(bb);
				bbRef->removed = true;
			}
		}

		/* XXX INVESTIGATE WHAT YOU DID */
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

	std::set<std::string> leaveFunctions = {
		"main"
	};
	PruneHelp::removeUnusedFunctions(&M, leaveFunctions);

	return true;
}

bool
GVizPruneBlocks::setFocusFunc(unsigned dep)
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
GVizPruneBlocks::parseFocusFuncForBlocks()
{
	std::set<unsigned> blkNums;
	char yn;
	bool interactiveFn = false; 
	bool showWholeFn = false;


	std::string q = "\n<PB> Function: " + focusFunction + "\n";
	q += "<PB> total blocks: " + std::to_string(std::distance(fun->begin(),
	  fun->end())) + "\n";
	q += "<PB> prune blocks interactively? (y/n) ";
	yn = CommonUtils::askQuestion(q.c_str(), "yn");
	if (yn == 'y') {
		interactiveFn = true;
	}
/* This is not an option for now. WriteGraph() was not as helpful as I thought.
	if (interactiveFn) {
		yn = CommonUtils::askQuestion("\n<PB> show whole function? (y/n) ",
		  "yn");
		if (yn == y) {
			showWholeFn = true;
		}
	}

 */
	if (showWholeFn) {
	//	DOTGraphTraits<const Function *>::WriteGraph(wholeHandle, 
 		std::string Filename = ("cfg." + fun->getName() + ".dot").str();
		errs() << "Writing '" << Filename << "'...";
		std::error_code EC;
		raw_fd_ostream File(Filename, EC, sys::fs::F_Text);
		if (!EC) {
			WriteGraph(File, fun);
		} else {
			errs() << "  error opening file for writing!";
		}
	}
		
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
			if (interactiveFn) {
				int resp = -1;

				errs() << "<PB> Function: " << focusFunction << "\n";
				BBp->dump();

				if (showWholeFn == false) {
					resp = viewPruneBlock(BBp);		
				}

				yn = CommonUtils::askQuestion("\n<PB> Keep block? (y/n) ",
				  "yn");
				if (yn == 'y') {
					keepBlocks.insert(BBlockRef(BBp, blkCnt));
				} else {
					removeBlocks.insert(BBlockRef(BBp, blkCnt));
				}	
				if (resp != -1) {
					kill(resp, SIGKILL);
					/* derp */
					unlink("/tmp/givzdingus.ps"); // XXX!
				}
			} else {
				removeBlocks.insert(BBlockRef(BBp, blkCnt));
			}
		}
	}
}

/// XXX Next steps would be to pass in stuff so we can color the prune blocks
// even if it's not he current prune block.... .. so you know if a pred or
// succ is a prunecake
int
GVizPruneBlocks::viewPruneBlock(BasicBlock *pruneBlock)
{
	Function *fn = pruneBlock->getParent();
	std::string dotFile = "/tmp/gvizding.dot"; // XXX
	std::string psFile = "/tmp/gvizding.ps"; // XXX
	std::string pdfFile = "/tmp/gvizding.pdf"; // XXX
	/* I do not really know if I need to do out to .pdf */	
	/* Really, I need some interactive gui to be all clicky clicky */

	std::ofstream dotHandle;
	dotHandle.open(dotFile);
	dotHandle << "digraph BasicBlockGraph {\n";

	std::string NodeLabel = DOTGraphTraits<const Function *>
	  ::getCompleteNodeLabel(pruneBlock, pruneBlock->getParent());

	dotHandle << "  BlockToWhack [shape=box,label=\""  \
	  << NodeLabel << "\",style=filled,color=\".7 .3 1.0\"];\n";

    dotHandle << "  BlockToWhack -> BlockToWhackFn " \
      << " [style=dotted, arrowhead=odot, arrowsize=1];\n";
    dotHandle << "  BlockToWhackFn [shape=plaintext,label=\""  \
      << pruneBlock->getParent()->getName().str() << "\"];\n";
    dotHandle << "  { rank=same; BlockToWhack;BlockToWhackFn}\n";


	unsigned bubba = 1;
	for (auto pi = pred_begin(pruneBlock); pi != pred_end(pruneBlock); ++pi) {
		BasicBlock *predBB = *pi;	

		std::string PredLabel = DOTGraphTraits<const Function *>
	  	  ::getCompleteNodeLabel(predBB, predBB->getParent());

		std::string nn = "Pred" + std::to_string(bubba);
		std::string nnF = "PredF" + std::to_string(bubba);
		dotHandle << "  " << nn << " [shape=box,label=\""  \
		  << PredLabel << "\"];\n";
		dotHandle << "  " << nn << " -> BlockToWhack;\n";
    	dotHandle << "  " << nn << " -> " << nnF 
		  << " [style=dotted, arrowhead=odot, arrowsize=1];\n";
		dotHandle << "  " << nnF << " [shape=plaintext,label=\""  \
		  << predBB->getParent()->getName().str() << "\"];\n";
		dotHandle << "  { rank=same; " << nn << ";" << nnF << " }\n";
		++bubba;
	}

	bubba = 1;
	for (auto pi = succ_begin(pruneBlock); pi != succ_end(pruneBlock); ++pi) {
		BasicBlock *succBB = *pi;	

		std::string SuccLabel = DOTGraphTraits<const Function *>
	  	  ::getCompleteNodeLabel(succBB, succBB->getParent());

		std::string nn = "Succ" + std::to_string(bubba);
		std::string nnF = "SuccF" + std::to_string(bubba);
		dotHandle << "  " << nn << " [shape=box,label=\""  \
		  << SuccLabel << "\"];\n";
		dotHandle << "  BlockToWhack -> " << nn << ";\n";
    	dotHandle << "  " << nn << " -> " << nnF 
		  << " [style=dotted, arrowhead=odot, arrowsize=1];\n";
		dotHandle << "  " << nnF << " [shape=plaintext,label=\""  \
		  << succBB->getParent()->getName().str() << "\"];\n";
		dotHandle << "  { rank=same; " << nn << ";" << nnF << " }\n";
		++bubba;
	}
	dotHandle << "}\n";
	dotHandle.close();

	GVizUtils::dot2ps(dotFile, psFile);
	unlink(dotFile.c_str());

	return GVizUtils::launchGV(psFile);
}

char GVizPruneBlocks::ID = 0;
static RegisterPass<GVizPruneBlocks> X("pskin-gviz-prune",
  "Prune with Hackity hackness");


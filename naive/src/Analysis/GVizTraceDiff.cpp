/*
 * Attempt to provide visual insight into trace diff by
 * displaying the blocks preceding differences. Not sure
 * how totally useful it is.
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
#include "GVizTraceDiff.h"

BasicBlock *
GVizTraceDiff::getBlockByCount(std::string predFnName, unsigned predBlockNum)
{
	Function *predFn = mod->getFunction(predFnName);
	BasicBlock *predBB;
	auto bbi = predFn->begin();
	unsigned pc = predBlockNum;
	do {
		predBB = &*bbi;
		++bbi;
	} while (--pc != 0);
	return predBB;
}

bool
GVizTraceDiff::runOnModule(Module &M)
{
	std::string dotFile = "/tmp/gvizding.dot"; // XXX
	std::string psFile = "/tmp/gvizding.ps"; // XXX
	std::string pdfFile = "/tmp/gvizding.pdf"; // XXX

	mod = &M;

	std::ofstream dotHandle;
	dotHandle.open(dotFile);
	dotHandle << "digraph BasicBlockGraph {\n";

	unsigned sameCount = 0;
	auto divergeMaps = TraceUtils::DiffTraceForDivergentBlocks(origTraceFile,
	  sampleTraceFile, precedingBlockCount, &sameCount);

	errs() << "Trace diff same path count: " << sameCount << "\n";

	unsigned nDivMaps = divergeMaps.size();
	unsigned nPredMaps = nDivMaps - 2;		// tot - orig - sample

	auto dx = divergeMaps.begin();
	while (nPredMaps > 0) {
		auto predMap = *dx;

		std::string predFnName = predMap["function"];
		unsigned predBlockNum = std::stoi(predMap["block"]); 

		// Should have a helper function that returns predBB. same code
		BasicBlock *predBB = getBlockByCount(predFnName, predBlockNum);

		std::string NodeLabel = DOTGraphTraits<const Function *>
		  ::getCompleteNodeLabel(predBB, predBB->getParent());

		std::string gvName = "Predecessor" + std::to_string(nPredMaps);
		dotHandle << "  " << gvName << " [shape=box,label=\""  \
		  << NodeLabel << "\",style=filled,color=\".7 .3 1.0\"];\n";

		std::string gvExtraName = "PLabel" + std::to_string(nPredMaps);
		dotHandle << "  " << gvName << " -> " \
		  << gvExtraName << " [style=dotted, arrowhead=odot, arrowsize=1];\n";

		dotHandle << "  " << gvExtraName << " [shape=plaintext,label=\""  \
		  << predFnName << "\"];\n";
		dotHandle << "  { rank=same; " << gvName << ";"  \
		  << gvExtraName << "}\n"; 

		if (0 != nPredMaps - 1) {
			std::string gvNextName = "Predecessor"  \
			  + std::to_string(nPredMaps - 1);
			dotHandle << "  " << gvName << " -> " << gvNextName << ";\n";
		}

		++dx;
		--nPredMaps;
	}

	auto origNext = *dx;
    std::string origFnName = origNext["function"];
    unsigned origBlockNum = std::stoi(origNext["block"]);

	++dx;
	auto sampleNext = *dx;
	// assert dx.end
    std::string sampleFnName = sampleNext["function"];
    unsigned sampleBlockNum = std::stoi(sampleNext["block"]);

	BasicBlock *origBB = getBlockByCount(origFnName, origBlockNum);
	std::string OrigSuccLabel = DOTGraphTraits<const Function *>
	  	  ::getCompleteNodeLabel(origBB, origBB->getParent());
	dotHandle << "  OriginalNextBlock  [shape=box,label=\""  \
	  << OrigSuccLabel << "\",style=filled,color=\"green\"];\n";
	dotHandle << "  Predecessor1 -> OriginalNextBlock;\n";

	dotHandle << "  OriginalNextBlock -> OriginalNextBlockFn " \
	  << " [style=dotted, arrowhead=odot, arrowsize=1];\n";
	dotHandle << "  OriginalNextBlockFn [shape=plaintext,label=\""  \
	  << origFnName << "\"];\n";
	dotHandle << "  { rank=same; OriginalNextBlock;OriginalNextBlockFn }\n";

	BasicBlock *sampleBB = getBlockByCount(sampleFnName, sampleBlockNum);
	std::string SampleSuccLabel = DOTGraphTraits<const Function *>
	  ::getCompleteNodeLabel(sampleBB, sampleBB->getParent()); 

	dotHandle << "  SampleNextBlock  [shape=box,label=\""  \
	  << SampleSuccLabel << "\",style=filled,color=\"yellow\"];\n";
	dotHandle << "  Predecessor1 -> SampleNextBlock;\n";
	dotHandle << "  SampleNextBlock -> SampleNextBlockFn " \
	  << " [style=dotted, arrowhead=odot, arrowsize=1];\n";
	dotHandle << "  SampleNextBlockFn [shape=plaintext,label=\""  \
	  << sampleFnName << "\"];\n";
	dotHandle << "  { rank=same; SampleNextBlock;SampleNextBlockFn }\n";



	dotHandle << "}\n";
	dotHandle.close();

	GVizUtils::dot2ps(dotFile, psFile);
	unlink(dotFile.c_str());
	GVizUtils::launchGV(psFile);

	return false;
}
char GVizTraceDiff::ID = 0;


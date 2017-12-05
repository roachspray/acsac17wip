#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/CallSite.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/DOTGraphTraits.h"
#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/Support/CommandLine.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>


using namespace llvm;

#include "GraphBlock.h"

static cl::opt<std::string> FunctionName("pskin-function-name",
  cl::desc("Name of function you want block from"), cl::init("main"),
  cl::Required);

static cl::opt<unsigned> BlockNumber("pskin-block-number",
  cl::desc("Starting at 0 the block you want"), cl::init(0),
  cl:Required);

static cl::opt<unsigned> DotFileName("pskin-dot-file",
  cl::desc("Name of output file"), cl::init("block.dot"));

void
GraphBlock::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesCFG();
}

bool
GraphBlock::runOnModule(Module &M)
{
	Function *fn = NULL;

	fn = M.getFunction(FunctionName);
	if (fn == NULL) {
		errs() << "<GB> focus function specified is not in module\n";
		return false;
	}


	unsigned bcnt = std::distance(fn->begin(), fn->end());
	if (bcnt == 0 || bcnt <= BlockNumber) {
		errs() << "<GB> the focus block specified is not valid for function\n";
		return false;
	}
	bcnt = BlockNumber;
	auto bit = fn->begin();
	bcnt--;
	while (bcnt != 0) {
		++bit;
		--bcnt;
	}
	
	BasicBlock *bb = &*bit;

	std::ofstream dotHandle;
	dotHandle.open(DotFileName); // XXX
	dotHandle << "digraph BasicBlockGraph {\n";
	std::string NodeLabel = DOTGraphTraits<const Function *>
	  ::getCompleteNodeLabel(bb, bb->getParent());
	dotHandle << "  BlockToWhack [shape=box,label=\"" << NodeLabel << "\"];\n";
    dotHandle << "  BlockToWhack -> BlockToWhackFn " \
      << " [style=dotted, arrowhead=odot, arrowsize=1];\n";
    dotHandle << "  BlockToWhackFn [shape=plaintext,label=\""  \
      << bb->getParent()->getName().str() << "\"];\n";
    dotHandle << "  { rank=same; BlockToWhack;BlockToWhackFn}\n";


	unsigned bubba = 1;
	for (auto pi = pred_begin(bb); pi != pred_end(bb); ++pi) {
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
	for (auto pi = succ_begin(bb); pi != succ_end(bb); ++pi) {
		BasicBlock *succBB = *pi;	
		std::string SuccLabel = DOTGraphTraits<const Function *>
	  	  ::getCompleteNodeLabel(succBB, succBB->getParent());
		std::string nn = "Succ" + std::to_string(bubba);
        std::string nnF = "SuccF" + std::to_string(bubba);

		dotHandle << "  " << nn << " [shape=box,label=\""  \
		  << SuccLabel << "\"];\n";
		dotHandle << "  BlockToWhack -> " << nn << ";\n";
        dotHandle << "  " << nnF << " [shape=plaintext,label=\""  \
          << succBB->getParent()->getName().str() << "\"];\n";
        dotHandle << "  { rank=same; " << nn << ";" << nnF << " }\n";

		++bubba;
	}
	dotHandle << "}\n";
	dotHandle.close();
	return false;
}
char GraphBlock::ID = 0;
static RegisterPass<GraphBlock> XX("graph-block-N-of-function",
  "simply dump the DOT form of a single block of a given function");

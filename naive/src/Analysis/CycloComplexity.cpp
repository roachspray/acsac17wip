/*
 * This just does function-level cyclomatic complexity
 * and puts into stdout and to .dot file for graphalafa.
 */ 

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/CallSite.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace llvm;

#include "CycloComplexity.h"

void
CycloComplexity::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesCFG();
}

static void
addNode(std::ofstream *handle, std::string name, std::string color,
  std::string label)
{
	*handle << "  " << name << " [shape=box,label=\""  \
	  << label << "\",color=\"" << color << "\"];\n";
	return;
}

static void
addEdge(std::ofstream *handle, std::string from, std::string to)
{
	*handle << "  " << from << " -> " << to << ";\n";
	return;
}

// How wrong is this?
static int 
calculateFunctionCyclomaticComplexity(Function *F)
{
	/*
	 * For a function in well-formed IR, there should be only
	 * 1 connected component.
	 * 
	 * M = E - N + 2C;
	 *
	 */
	unsigned nEdges = 0; 				// E
	unsigned nNodes = 0;				// N
	unsigned nConnectedComponents = 1;	// C

	nNodes = std::distance(F->begin(), F->end());
	for (BasicBlock &BB : *F) {
		BasicBlock *b = &BB;
		nEdges += std::distance(succ_begin(b), succ_end(b));
	}
	return (nEdges - nNodes + (2 * nConnectedComponents));
}

static std::string
cleanStringForDOT(std::string in)
{
	std::string chg = ".! ";
	char good = 'J';
	
	for (unsigned j = 0; j < in.size(); j++) {
		for (unsigned i = 0; i < chg.size(); i++) {
			if (in[j] == chg[i]) {
				in[j] = good;
			}
		}	
	}
	return in;
}
bool
CycloComplexity::runOnModule(Module &M)
{
	Function *cf;

	std::ofstream dotHandle;
	dotHandle.open(dotFile); // XXX
	dotHandle << "digraph FunctionCallGraph {\n";


	for (Function &F : M) {
		cf = &F;

		if (F.hasName() == false) {
			continue;
		}
		int cc = calculateFunctionCyclomaticComplexity(&F);
		std::string col;
		if (cc <= 5) {
			col = "green";
		} else if (cc > 5 && cc <= 10) {
			col = "yellow";
		} else {
			col = "red";
		}
		std::string fname = cleanStringForDOT(F.getName().str());
		addNode(&dotHandle, fname, col, fname + " cc: "+std::to_string(cc)); 
	}
	for (Function &F : M) {
		if (F.hasName() == false) {
			continue;
		}
		std::string callerName = cleanStringForDOT(F.getName().str());
		for (BasicBlock &B : F) {
			for (Instruction &I : B) {
				if (isa<CallInst>(&I) || isa<InvokeInst>(&I)) {
					CallSite cs(&I);
					Function *cf = cs.getCalledFunction();
					if (cf) {
						if (cf->hasName()) {	
							std::string calleeName = cleanStringForDOT(
							  cf->getName().str());
							addEdge(&dotHandle, callerName, calleeName);
						} else {
							std::string callee = "unkFunc_"  \
							  + std::to_string(noFnName);
							++noFnName;
							addEdge(&dotHandle, callerName, callee);
						}
					} else {
						addEdge(&dotHandle, callerName, "<indirect>");
					}
				}
			}
		}
	}
	dotHandle << "}\n";
	dotHandle.close();
	return false;
}
char CycloComplexity::ID = 0;
static RegisterPass<CycloComplexity> XX("cyclomatic", "");

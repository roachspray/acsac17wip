/*
 * Old stuff...
 *
 * Front end to the various passes, but this is not really used
 * anymore. At one point I was trying to put things all under
 * one PM, but it just started to feel too confining. So, broke
 * back out to shared libs. 
 *
 * These shared libs can be invoked in various ways, but a front
 * end to some can be found in the python/ code.
 *
 */

#include "llvm/LinkAllPasses.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Bitcode/BitcodeWriterPass.h"
#include "llvm-c/Core.h"
#include "llvm/Analysis/CallGraph.h"

#include <iostream>
#include <fstream>

using namespace llvm;

#include "Analysis/DumpUsersPass.h"
#include "Analysis/GraphBlock.h"
#include "Analysis/CycloComplexity.h"
#include "Analysis/GVizTraceDiff.h"
#include "Transform/TraceBBInject.h"
#include "TraceUtils.h"
#include "Transform/PruneBlocks.h"
#include "Transform/GVizPruneBlocks.h"
#include "Transform/ProbPruneBlocks.h"

/*
 * These are intended to be mutually exclusive but I don't have
 * that logic in.
 *
 * General use layout:
 *   Pruneskin <command> <input.bc> <output.bc>
 *
 *
 * Inject trace points into a target .bc logging to tracefile:
 *
 *   -pskin-inject-trace-points <tracefile> <in.bc> <out.bc>
 * 
 *
 * Perform naive pruning with tracefile. Optionally specify a maximum
 * call depth for pruning. Optionally specifiy a list of functions we
 * want to look at for return-value-check based branching. Optionally
 * make this run in a (non-GUI) interactive mode.
 *
 *     -pskin-naive-prune <tracefile> [-pskin-naive-prune-depth <N>]  \
 *        [-pskin-probe-call-functions <file>]  \
 *        [-pskin-interactive]
 *
 *
 * Similar to the naive pruning, but allows for interactive that will
 * also display a postscript file rendering the block flow of interest.
 * You may also specifiy depth. You may also specify a file listing
 * functions for return-value-check based branching.
 *
 *     -pskin-gviz-prune <tracefile> [-pskin-gviz-prune-depth <N>]  \
 *        [-pskin-probe-call-functions <file>] 
 *
 * 
 * Generate basic .dot file given a function name and block number. The
 * block number is assuming a few of the passes (registered early with
 * the PassManager have run.
 *
 *     -pskin-dot-block <function,blknum>
 *
 *
 * Prune blocks based on probability and a tracefile. You may optionally
 * specify a maximum call depth to prune at. You may optionally specify
 * which algorithm and initialization parameters to use. Currently the only
 * two support methods are: NoOpPrPune,foo or DecreasingWeightedCoinFlip,mean
 *     
 *     -pskin-pr-prune <tracefile> [-pskin-pr-prune-depth <N>]  \
 *        [-pskin-pr-prune-fn <alg,init...>] 
 * 
 *
 * 
 * Generate a .dot file with functions colored according to complexity value.
 *
 *	   -pskin-cyclomatic <dotfile> <input.bc> <output.bc>
 *
 *
 * Hacked up helper to compare two trace logs... the goal being to help one
 * understand where the traces diverge from the original program and the
 * slice program.
 *
 *     -pskin-orig-trace <trace.log> -psking-sample-trace <trace.log> 
 *
 *
 */
cl::opt<std::string> InputBitcodeFile(cl::Positional, cl::desc("<input.bc>"),
  cl::Required);
cl::opt<std::string> OutputBitcodeFile(cl::Positional, cl::desc("<output.bc>"),
  cl::Required);
cl::opt<std::string> NaivePrune("pskin-naive-prune",
  cl::desc("Perform naive pruning based on input trace file"), cl::init(""));
cl::opt<unsigned> NaivePruneDepth("pskin-naive-prune-depth",
  cl::desc("Max depth on naive pruning"), cl::init(1));
cl::opt<bool> DumpUsers("pskin-dump-users", 
  cl::desc("Dump Value User information"), cl::init(false));
cl::opt<bool> InteractiveMode("pskin-interactive", cl::init(false));
cl::opt<std::string> DotBlock("pskin-dot-block", cl::init(""));
cl::opt<std::string> GVizPrune("pskin-gviz-prune",
  cl::desc("Perform graphviz aided pruning based on input trace file"), cl::init(""));
cl::opt<unsigned> GVizPruneDepth("pskin-gviz-prune-depth",
  cl::desc("Max depth on graphviz aided pruning"), cl::init(1));

cl::opt<std::string> ProbeCallFunctions("pskin-probe-call-functions",
  cl::desc("Specify file with function names to be ret-check functions"),
  cl::init(""));

cl::opt<std::string> ProbPrune("pskin-pr-prune",
  cl::desc("Perform probabilistic pruning (defaults to Naive)"), cl::init(""));
cl::opt<unsigned> ProbPruneDepth("pskin-pr-prune-depth",
  cl::desc("Max depth on probabalistic pruning"), cl::init(1));
cl::opt<std::string> ProbTypePrune("pskin-pr-prune-fn",
  cl::desc("Specify probabalistic pruning type and mean, etc"), cl::init(""));

cl::opt<std::string> OrigTrace("pskin-orig-trace", cl::desc("fo"),
  cl::init(""));
cl::opt<std::string> SampleTrace("pskin-sample-trace", cl::desc("fo"),
  cl::init(""));
cl::opt<unsigned> PrecBlockCount("pskin-prec-block-count", cl::desc("fo"),
  cl::init(1));

cl::opt<std::string> Cyclomatic("pskin-cyclomatic",
  cl::desc("DOT file cfg with colord on cyclomatic complexify"), cl::init(""));

int
main(int argc, char **argv)
{
	std::unique_ptr<Module> mod;
	legacy::PassManager	passManager;
	SMDiagnostic err;
	std::error_code ec;
	raw_fd_ostream *outputStream;

	cl::ParseCommandLineOptions(argc, argv);

	std::cout << "<C> Reading input bitcode file: " << InputBitcodeFile << "\n";
	mod = parseIRFile(InputBitcodeFile, err,
	  *unwrap(LLVMGetGlobalContext()));
	if (mod == nullptr) {
		std::cout << "<C> parseIRFile: " << err.getMessage().str() << "\n";
		return -1;
	}
#ifdef	OLDANDINTHEWAY
	std::cout << "<C> Adding mem2reg pass.\n";
	passManager.add(createPromoteMemoryToRegisterPass());
	std::cout << "<C> Adding constant propagation passes.\n";
	passManager.add(createConstantPropagationPass());
	passManager.add(createIPConstantPropagationPass());
#endif	// !


	if (DotBlock != "") {
		unsigned bn = DotBlock.find(",", 0);
		// XXX
		std::string fname = DotBlock.substr(0, bn);
		std::string bnum = DotBlock.substr(bn+1, DotBlock.size());		
		unsigned bnumV = std::stoi(bnum);
		GraphBlock *gb = new GraphBlock();
		gb->setFocusFunction(fname);
		gb->setFocusBlock(bnumV);
		gb->setDotFile(fname + "__" + bnum + ".dot");
		passManager.add(gb);

	}

		TraceBBInject *tbbi = new TraceBBInject();
		tbbi->setTraceFile(InjectTracePoints);
		passManager.add(tbbi);
	} else if (NaivePrune != "") {
		std::cout << "<C> Adding graph rip pass.\n";
		PruneBlocks *pb = new PruneBlocks();
		pb->setTraceFile(NaivePrune);
		pb->setPruneDepth(NaivePruneDepth);	
		pb->setInteractive(InteractiveMode);

		if (ProbeCallFunctions != "") {
			std::ifstream fileHandle(ProbeCallFunctions);
			std::string pcfName;
			while (std::getline(fileHandle, pcfName)) {
				std::cout << "Will look for: " << pcfName << "\n";
				pb->addProbeCallFunction(pcfName);
			}
		}

		passManager.add(pb);
	} else if (GVizPrune != "") {
		std::cout << "<C> Adding graphviz prune pass.\n";
		GVizPruneBlocks *pb = new GVizPruneBlocks();
		pb->setTraceFile(GVizPrune);
		pb->setPruneDepth(GVizPruneDepth);	
		passManager.add(pb);
	} else if (ProbPrune != "") {
		std::cout << "<C> Adding prob'prune pass.\n";
		ProbPruneBlocks *pb = new ProbPruneBlocks();
		pb->setTraceFile(ProbPrune);
		pb->setPruneDepth(ProbPruneDepth);	
		pb->setPrPrune(ProbTypePrune);
		passManager.add(pb);
	} else if (OrigTrace != "" && SampleTrace != "") {
		std::cout << "<C> Adding Trace Diff pass - orig:green, sample:yellow\n";
		GVizTraceDiff *pb = new GVizTraceDiff();
		pb->setOrigTraceFile(OrigTrace);
		pb->setSampleTraceFile(SampleTrace);
		if (PrecBlockCount > 1) {
			pb->setPrecedingBlockCount(PrecBlockCount);
		}
		passManager.add(pb);
	} else {
		if (DumpUsers) {
			std::cout << "<C> Adding dump users pass\n";
			passManager.add(new DumpUsersPass());
		}
		if (Cyclomatic != "") {
			std::cout << "<C> Adding cyclomatic complexity cfg dot pass\n";
			CycloComplexity *ccp = new CycloComplexity();
			ccp->setDotFile(Cyclomatic);
			passManager.add(ccp);
		}
	}

	std::cout << "<C> Adding bitcode writer pass\n";
	outputStream = new raw_fd_ostream(OutputBitcodeFile, ec, sys::fs::F_None);
	passManager.add(createBitcodeWriterPass(*outputStream, false, true));

	std::cout << "<C> Running passes\n";
	passManager.run(*mod.get());
	std::cout << "<C> Ran passes\n";
	outputStream->close();
	std::cout << "<C> Finished...\n";
	return 0;
}

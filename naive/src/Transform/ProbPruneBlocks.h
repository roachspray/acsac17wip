#ifndef __PROBPRUNEBLOCKS_H
#define	__PROBPRUNEBLOCKS_H

#include "BBlockRef.h"
#include "PrPrune.h"

struct ProbPruneBlocks : ModulePass {
private:
	std::string	traceFile;
	std::string focusFunction;
	unsigned pruneDepth;

	std::set<TraceEntry>	traceEntries;
	std::set<std::string>	prunedFunctions;

	std::set<BBlockRef>  keepBlocks;
	std::set<BBlockRef> removeBlocks;

	Module	*mod;
	Function *fun;
	PrPrune	*prPrune;

	void parseFocusFuncForBlocks(unsigned depth);
	bool setFocusFunc(unsigned depth);

public:
	static char ID;
	ProbPruneBlocks() : ModulePass(ID), traceFile("") { }
	virtual bool runOnModule(Module &);
//	virtual void getAnalysisUsage(AnalysisUsage &) const;

	void	setPrPrune(std::string fromcmdline) {
		auto ci = fromcmdline.find(",", 0);
		auto a = fromcmdline.substr(0, ci);
		std::string m = "";
		if (ci != std::string::npos) {
			m = fromcmdline.substr(ci+1, fromcmdline.size());
		}
		prPrune = PrPrune::get(a, m);
	}

	void	setTraceFile(std::string f) { traceFile = f; }
	void	setFocusFunction(std::string f) { focusFunction = f; }
	void	setPruneDepth(unsigned d) { pruneDepth = d; }
};
#endif

#ifndef _GVIZPRUNEBLOCKS_H
#define	_GVIZPRUNEBLOCKS_H

#include "BBlockRef.h"

struct GVizPruneBlocks : ModulePass {
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

	void parseFocusFuncForBlocks();
	bool setFocusFunc(unsigned d);

	int viewPruneBlock(BasicBlock *pruneBB);

public:
	static char ID;
	GVizPruneBlocks() : ModulePass(ID), traceFile("") { }
	virtual bool runOnModule(Module &);
//	virtual void getAnalysisUsage(AnalysisUsage &) const;

	void	setTraceFile(std::string f) { traceFile = f; }
	void	setFocusFunction(std::string f) { focusFunction = f; }
	void	setPruneDepth(unsigned d) { pruneDepth = d; }
};
#endif

#ifndef _PRUNEBLOCKS_H
#define	_PRUNEBLOCKS_H

#include <list>

#include "BBlockRef.h"

struct PruneBlocks : ModulePass {
private:

	std::set<TraceEntry>	traceEntries;
	std::set<std::string>	prunedFunctions;

	std::list<BBlockRef>  keepBlocks;
	std::list<BBlockRef> removeBlocks;

	Module	*mod;

	std::string currentFocusFunctionName;
	Function *currentFocusFunction;

	unsigned parseFocusFuncForBlocks();
	bool setFocusFunc(unsigned d);

	/* Move this at some point */
	std::vector<std::string> probeCallRetCompareNames = {
		"fork",
		"close",
		"wait",
		"read",
		"ioctl",
		"write",
		"fwrite",
		"malloc"
	};
	std::set<Function *> probeCallRetCompareFunctions;
	 // I think .... I do not know to what extent we want to go here

public:
	static char ID;
	PruneBlocks() : ModulePass(ID) {}

	virtual bool runOnModule(Module &);
//	virtual void getAnalysisUsage(AnalysisUsage &) const;

	void	setTraceFile(std::string f) { }
	void	setFocusFunction(std::string f) { currentFocusFunctionName = f; }

	void	addProbeCallFunction(std::string n) {
		probeCallRetCompareNames.push_back(n);
	}

	bool	isCallRetSuccored(Function *f) const {
		auto s = probeCallRetCompareFunctions.find(f);
		if (s == probeCallRetCompareFunctions.end()) {
			return false;
		}
		return true;
	}
};
#endif

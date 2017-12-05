#ifndef _PRUNEHELP_H
#define	_PRUNEHELP_H

#include "BBlockRef.h"
#include <list>
#include <set>

class PruneHelp {

public:
	static void tryCleanseBranchInst(BBlockRef *bbRef, BasicBlock *predBB,
	  BranchInst *bi, std::list<BBlockRef> removeBlocks,
	   std::set<Function *> checkRetFns);
	static void tryCleanseIndirectBrInst(BBlockRef *bbRef, IndirectBrInst *ci,
	  std::list<BBlockRef> removeBlocks);
	static void tryCleanseCatchReturnInst(BBlockRef *bbRef, CatchReturnInst *ci,
	  std::list<BBlockRef> removeBlocks);
	static void tryCleanseCatchSwitchInst(BBlockRef *bbRef, CatchSwitchInst *ci,
	  std::list<BBlockRef> removeBlocks);
	static void tryCleanseCleanupReturnInst(BBlockRef *bbRef,
	  CleanupReturnInst *ci, std::list<BBlockRef> removeBlocks);
	static void tryCleanseInvokeInst(BBlockRef *bbRef, InvokeInst *ci,
	  std::list<BBlockRef> removeBlocks);
	static void tryCleanseSwitchInst(BBlockRef *bbRef, SwitchInst *ci,
	  std::list<BBlockRef> removeBlocks);

	static unsigned removeUnusedFunctions(Module *M,
	  std::set<std::string> whitelist);

	static void removeUnusedBasicBlocks(Module *M,
	  std::set<std::string> whitelist);

	static Value *pullThroughCastInst(Value *v);

	static bool repairPHINodes(Function *f, std::list<BBlockRef> p);
};
#endif

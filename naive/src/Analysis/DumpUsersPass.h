#ifndef __DUMPUSERSPASS_H
#define	__DUMPUSERSPASS_H

struct DumpUsersPass : public ModulePass {
	static char ID;

	DumpUsersPass() : ModulePass(ID) {}
	virtual bool runOnModule(Module &);
   	virtual void getAnalysisUsage(AnalysisUsage &) const;

private:
};

#endif

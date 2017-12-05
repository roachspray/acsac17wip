#ifndef __NOPREDBLOCKS_H
#define	__NOPREDBLOCKS_H

struct NoPredBlocks : public ModulePass {
	static char ID;

	NoPredBlocks() : ModulePass(ID) {}
	virtual bool runOnModule(Module &);
   	virtual void getAnalysisUsage(AnalysisUsage &) const;

private:
};

#endif

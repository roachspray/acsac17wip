#ifndef __GRAPHBLOCK_H
#define	__GRAPHBLOCK_H

struct GraphBlock : public ModulePass {
	static char ID;

	GraphBlock() : ModulePass(ID) { }
	virtual bool runOnModule(Module &);
   	virtual void getAnalysisUsage(AnalysisUsage &) const;


};

#endif

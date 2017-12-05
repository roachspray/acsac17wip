#ifndef _UNLABELMDINJECTED_H
#define	_UNLABELMDINJECTED_H

struct UnlabelMDInjected : public ModulePass {
	static char ID;

	UnlabelMDInjected() : ModulePass(ID) {}
	virtual bool runOnModule(Module &);

};
#endif

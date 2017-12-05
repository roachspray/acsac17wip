#ifndef _LABELMDINJECT_H
#define	_LABELMDINJECT_H

struct LabelMDInject : public ModulePass {
	static char ID;

	LabelMDInject() : ModulePass(ID) {}
	virtual bool runOnModule(Module &);

};
#endif

#ifndef _TRACEBBINJECT_H
#define	_TRACEBBINJECT_H

struct TraceBBInject : public ModulePass {
	static char ID;

	TraceBBInject() : ModulePass(ID) {}
	virtual bool runOnModule(Module &);

};
#endif

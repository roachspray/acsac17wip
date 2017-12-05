#ifndef __CYCLOCOMPLEXITY_H
#define	__CYCLOCOMPLEXITY_H

struct CycloComplexity : public ModulePass {
	static char ID;

	CycloComplexity() : ModulePass(ID), noFnName(0) { }
	virtual bool runOnModule(Module &);
   	virtual void getAnalysisUsage(AnalysisUsage &) const;
	void setDotFile(std::string f)  {  dotFile = f; }

private:
	std::string dotFile;
	unsigned noFnName;
};

#endif

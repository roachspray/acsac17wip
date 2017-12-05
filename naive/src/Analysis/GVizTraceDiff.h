#ifndef _GVIZTRACEDIFF_H
#define	_GVIZTRACEDIFF_H


struct GVizTraceDiff : ModulePass {
private:
	std::string	origTraceFile;
	std::string	sampleTraceFile;
	unsigned precedingBlockCount;

	Module	*mod;
	Function *fun;

public:
	static char ID;
	GVizTraceDiff() : ModulePass(ID), origTraceFile(""),
	  sampleTraceFile(""), precedingBlockCount(1) {
	} 
	virtual bool runOnModule(Module &);
//	virtual void getAnalysisUsage(AnalysisUsage &) const;

	void	setOrigTraceFile(std::string f) { origTraceFile = f; }
	void	setSampleTraceFile(std::string f) { sampleTraceFile = f; }
	void	setPrecedingBlockCount(unsigned p) { precedingBlockCount = p; }
	BasicBlock *getBlockByCount(std::string predFnName, unsigned predBlockNum);
};
#endif

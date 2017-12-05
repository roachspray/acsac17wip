#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/CallSite.h"
#include "llvm/Support/raw_ostream.h"


using namespace llvm;

#include "DumpUsersPass.h"

void
DumpUsersPass::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.setPreservesCFG();
}

void
dumpUses(Value *v)
{
	for (auto ui = v->use_begin(); ui != v->use_end(); ++ui) {
		Use *u = &*ui;
		User *usr = u->getUser();
		errs() << "User:\n";
		usr->dump();
		errs()  << "----\n";
	}
}
bool
DumpUsersPass::runOnModule(Module &M)
{

	for (Function &F : M) {
		errs() << "Function:\n";
		F.dump();
		errs() << "--------:\n";
		errs() << "Fn Users:\n";
		dumpUses(&F);
		errs() << "Fn Users end\n";
		errs() << "BasicBlocks for this Function:\n";
		for (BasicBlock &BB : F) {
			BB.dump();
			errs() << "--------:\n";
			errs() << "BB Users:\n";
			dumpUses(&BB);
			errs() << "BB Users end\n";
			errs() << "Instructions for this Block:\n";
			for (Instruction &Inst: BB) {
				Inst.dump();
				errs() << "--------:\n";
				errs() << "Inst Users:\n";
				dumpUses(&Inst);
				errs() << "Inst Users end\n";
			}
		}
	}
	return false;
}
char DumpUsersPass::ID = 0;
static RegisterPass<DumpUsersPass> XX("dumpuserspass", "");

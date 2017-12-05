/*
 * Goal here is just some static functions that
 * can be used as pruning IR helpers.
 *
 */

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/SymbolTableListTraits.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

// Like i said.. ugly as fuhhhh
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <set>

using namespace llvm;

#include "../TraceUtils.h"
#include "PruneHelp.h"

/*
 * bbRef  - BasicBlock want to remove
 * predBB - Predecessor to the basic block references in bbRef
 * bi     - BranchInst that is the TerminatorInst of predBB BasicBlock
 * retCheckFns - set of functions we want the cmp/br kept for.
 *
 */
void
PruneHelp::tryCleanseBranchInst(BBlockRef *bbRef, BasicBlock *predBB,
  BranchInst *bi, std::list<BBlockRef> removeBlocks,
  std::set<Function *> retCheckFns)
{
	BasicBlock *bb = bbRef->getBB();
	BasicBlock *br1 = bi->getSuccessor(0);
	Module *mod = bb->getParent()->getParent();	// in fn, in mod

	/*
	 * In the unconditional case we have:
	 *     br %bbi
	 *
 	 * One would think that the block in which the unconditional branch
	 * exists would be unreachable, hmm? Something to think about...
	 * and possibly improve upon.
	 *
	 */
	if (bi->isUnconditional()) {
		new UnreachableInst(mod->getContext(), bi);
		// Unreachable anyway, so... don't think we need to whack the CmpInst
		// May want to assert no users
		bi->eraseFromParent();
		return;
	}

	
	BasicBlock *br2 = bi->getSuccessor(1);
	if (br1 == br2 && br1 == bb) {
		new UnreachableInst(mod->getContext(), bi);
		bi->eraseFromParent();
		return;
	}
	if (br1 == bb) {
		br1 = br2;
	}

	/*
	 * Determine if the condition branched upon is a CmpInst.
	 * Is this always the case? TBH, I'm not thinking clearly,
	 * so am not sure, but doubt.
	 *
	 * If it is CmpInst, then we want to check if it's a compare
	 * between a constant and the return value of a function of
	 * ``interest''.  Most of these functions like read() or 
	 * something where we /care/ about the return value check for
	 * accuracy purposes (fuzzing accuracy).
	 *
	 */
	Value *condition = bi->getCondition();
	CmpInst *conCmp = dyn_cast<CmpInst>(condition);
	if (conCmp) {
		Value *o0 = conCmp->getOperand(0);
		Value *o1 = conCmp->getOperand(1);

		o0 = PruneHelp::pullThroughCastInst(o0);
		o1 = PruneHelp::pullThroughCastInst(o1);

#ifdef DEBUG
		o0->dump();
		o1->dump();
#endif
		// SHould probably make a function :P	
		if (Constant *cv = dyn_cast<Constant>(o0)) {
			Function *calledFn = NULL;
			if (CallInst *ci = dyn_cast<CallInst>(o1)) {
				calledFn = ci->getCalledFunction();
			} else if (InvokeInst *ci = dyn_cast<InvokeInst>(o1)) {
				calledFn = ci->getCalledFunction();
			}
			if (calledFn) {
				auto s = retCheckFns.find(calledFn);
				if (s != retCheckFns.end()) {
					errs() << "skipped: branch removal: check-ret: " << calledFn << "\n";
					return;
				}
			}
		}
		if (Constant *cv = dyn_cast<Constant>(o1)) {
			Function *calledFn = NULL;
			if (CallInst *ci = dyn_cast<CallInst>(o0)) {
				calledFn = ci->getCalledFunction();
			} else if (InvokeInst *ci = dyn_cast<InvokeInst>(o0)) {
				calledFn = ci->getCalledFunction();
			}
			if (calledFn) {
				auto s = retCheckFns.find(calledFn);
				if (s != retCheckFns.end()) {
					errs() << "skipped: branch removal: check-ret: " << calledFn << "\n";
					return;
				}
			}
		}
	}

	/*
	 * Insert new, unconditional branch before bi, then kill bi.
	 */
	BranchInst::Create(br1, bi);
	bi->eraseFromParent();

	/*
	 * Determine if the compare Value is used elsewhere and axe it, if not.
	 */
	if (conCmp && conCmp->user_empty()) {
		conCmp->eraseFromParent();
	} 
	return;
}

/*
 * bbRef - Reference to block we want to axe
 * bi    - Branch trying to clean up
 *
 */
void
PruneHelp::tryCleanseIndirectBrInst(BBlockRef *bbRef, IndirectBrInst *bi, 
  std::list<BBlockRef> removeBlocks)
{
	BasicBlock *bb = bbRef->getBB();
	Module *mod = bb->getParent()->getParent();	// in fn, in mod

	unsigned ns = bi->getNumSuccessors();
	while (ns > 1) {
		--ns;
		BasicBlock *k = bi->getSuccessor(ns);
		if (k == bb) {
			bi->removeDestination(ns);
		}
	}
	if (ns == 1) {
		BasicBlock *sBB = bi->getSuccessor(0);
		if (sBB == bb) {
			new UnreachableInst(mod->getContext(), bi);
			bi->eraseFromParent();
			return;
		}
		BBlockRef t(sBB, 0);
		auto e = removeBlocks.begin();
		for (; e != removeBlocks.end(); ++e) {
			BBlockRef locRef = *e;
			if (locRef == t) {
				new UnreachableInst(mod->getContext(), bi);
				bi->eraseFromParent();
				return;
			}
		}	
	}
	return;
}

/*
 * bbRef - Ref to block we want to axe.
 * ci    - CatchReturnInst we want to clean
 *
 */
void
PruneHelp::tryCleanseCatchReturnInst(BBlockRef *bbRef, CatchReturnInst *ci, 
  std::list<BBlockRef> removeBlocks)
{

	BasicBlock *bb = bbRef->getBB();
	Module *mod = bb->getParent()->getParent();	// in fn, in mod
	// These have single successors... so we should mark as unreachable

	assert(ci->getSuccessor() == bb && "Failed CatchReturnInst");
	new UnreachableInst(mod->getContext(), ci);
	ci->eraseFromParent();
	return;
}

/*
 * bbRef - Ref to block we want to axe.
 * ci    - CatchSwitchInst we want to clean
 *
 */
void
PruneHelp::tryCleanseCatchSwitchInst(BBlockRef *bbRef, CatchSwitchInst *ci,
  std::list<BBlockRef> removeBlocks)
{
	BasicBlock *bb = bbRef->getBB();
	Module *mod = bb->getParent()->getParent();	// in fn, in mod
	/*
	 * This is just one example of something to reason on versus wielding axe.
	 * But, we're using an axe right now, so just note that.
	 * FURTHER. I do not know how reasonable it is to remove this Instruction.
	 */
	BasicBlock *unwindBB = ci->getUnwindDest();
	if (unwindBB == bb) {
		new UnreachableInst(mod->getContext(), ci);
		ci->eraseFromParent();
		return;
	}
	return;
}

/*
 * bbRef - Ref to block we want to axe.
 * ci    - CleanupReturnInst we want to clean
 *
 */
void
PruneHelp::tryCleanseCleanupReturnInst(BBlockRef *bbRef, CleanupReturnInst *ci,
  std::list<BBlockRef> removeBlocks)
{
	BasicBlock *bb = bbRef->getBB();
	Module *mod = bb->getParent()->getParent();	// in fn, in mod
	// This has optional successor.
	assert(ci->hasUnwindDest() && ci->getUnwindDest() == bb && "CleanupReturnInst failed");
	new UnreachableInst(mod->getContext(), ci);
	ci->eraseFromParent();
	return;
}

/*
 * bbRef - Ref to block we want to axe.
 * ci    - InvokeInst we want to clean
 *
 */
void
PruneHelp::tryCleanseInvokeInst(BBlockRef *bbRef, InvokeInst *ci, 
  std::list<BBlockRef> removeBlocks)
{
	BasicBlock *bb = bbRef->getBB();
	Module *mod = bb->getParent()->getParent();	// in fn, in mod

	// Check the normal label first.. if it's bb, then we replace invoke????
	// This is all a bit sketchy.
	BasicBlock *normalBB = ci->getNormalDest();
	BasicBlock *unwindBB = ci->getUnwindDest();
	// XXX what about the successors list as well????
	// I am guessing that is a list of exception handling blocks
	if (normalBB == bb) {
		// normal ret is the bad block, let's just try to whack things for now.
		new UnreachableInst(mod->getContext(), ci);
		ci->eraseFromParent();
		return;
	}
	BasicBlock *invBB = ci->getParent();
	BBlockRef t(invBB, 0);
	for (auto e  = removeBlocks.begin(); e!=removeBlocks.end(); ++e) {
		BBlockRef locRef = *e;
		if (locRef == t) {
			new UnreachableInst(mod->getContext(), ci);
			ci->eraseFromParent();
			break;
		}
	}	
	return;
}

/*
 * bbRef - Ref to block we want to axe.
 * ci    - SwitchInst we want to clean
 *
 */
void
PruneHelp::tryCleanseSwitchInst(BBlockRef *bbRef, SwitchInst *ci,
  std::list<BBlockRef> removeBlocks)
{
	BasicBlock *bb = bbRef->getBB();
	Module *mod = bb->getParent()->getParent();	// in fn, in mod

	/*
	 * Either we're a default and/or one of the cases
	 * If we're the default, this is a problem for now.. because
	 * we should check to see if this block was even reached.
	 * if it was not, we can whack this entirely.
	 * if it was, then we cannot be so brutal.. and skip.
	 *
	 */
	if (bb == ci->getDefaultDest()) {
		BasicBlock *switchBB = ci->getParent();
		BBlockRef t(switchBB, 0);
		for (auto e = removeBlocks.begin(); e != removeBlocks.end(); ++e) {
			BBlockRef locRef = *e;
			if (locRef == t) {
				new UnreachableInst(mod->getContext(), ci);
				ci->eraseFromParent();
				break;
			}
		}	
		return;
	} // Otherwise, we must go through each successor
	ConstantInt *consI = ci->findCaseDest(bb);
	while (consI != NULL) {
		ci->removeCase(ci->findCaseValue(consI));
		consI = ci->findCaseDest(bb);
	}
}

unsigned
PruneHelp::removeUnusedFunctions(Module *M, std::set<std::string> whitelist)
{
	unsigned functions_removed = 0;
	std::vector<Function *> notCalled;

	for (Function &F : *M) {
		if (F.isIntrinsic()) {
			continue;
		}
		if (F.hasName()) {
			errs() << "checking removeUnusedFunction: " << F.getName().str() << "\n";
			if (F.isDeclaration()) {
				errs() << "..it's a declarataion\n";
				continue;
			}
			auto k = whitelist.find(F.getName().str());
			if (k != whitelist.end()) {
				errs() << " ..it's white listed\n";
				continue;
			}
			if (F.user_empty()) { // && !F.isIntrinsic()) {
				errs() << "...its empty... adding to removal list\n";
				notCalled.push_back(&F);
			} else {
				errs() << "..it's still used..not removing..listing user->dump:\n";
				for (auto u = F.user_begin(); u != F.user_end(); ++u) {
					User *uu = *u;
					uu->dump();	
				}
				errs() << "..done dumping users\n";
			}
		}
	}
	for (auto k = notCalled.begin(); k != notCalled.end(); ++k) {
		Function *F = *k;
		errs() << "removingFunction in PruneHelp(): " << F->getName().str() << "\n";
		F->deleteBody();
		++functions_removed;
	}
	return functions_removed;
}

void
PruneHelp::removeUnusedBasicBlocks(Module *M, std::set<std::string> whitelist)
{
	std::vector<BasicBlock *> delBB;
	for (Function &F : *M) {
		if (F.hasName()) {
			auto k = whitelist.find(F.getName().str());
			if (k != whitelist.end()) {
				continue;
			}
		}
		for (BasicBlock &B : F) {
			if (B.user_empty()) {
				delBB.push_back(&B);
			}
		}
	}
	for (auto k = delBB.begin(); k != delBB.end(); ++k) {
		BasicBlock *B = *k;
		B->eraseFromParent();
	}
}

/*
 * Value *v that might be a TruncInst, ZExtInst or other effective
 * no-op (to us), and return first non-no-op (reverse) in the use
 * chain.
 *
 */
Value *
PruneHelp::pullThroughCastInst(Value *v)
{
	Value *ret = v;

	while (1) {
		if (CastInst *i = dyn_cast<CastInst>(ret)) {
			ret = i->getOperand(0);
			continue;
		}
		break;
	}
	return ret;
}

/*
 * Here we are concerned with making it so that there is never a 
 * larger number of incoming blocks for any PHI node in comparison
 * with the block predecessor list.
 *
 * F - function to look at PHI nodes for
 * removeBlocks
 */
bool
PruneHelp::repairPHINodes(Function *F, std::list<BBlockRef> removeBlocks)
{
	bool changed = false;
	std::vector<BasicBlock *> fullDeadBlocks;	

	bool entryBlock = true;
	for (BasicBlock &B : *F) {
		unsigned predCnt = std::distance(pred_begin(&B), pred_end(&B));
		// XXX
		if (entryBlock == false && predCnt == 0) {
			fullDeadBlocks.push_back(&B);
			continue;
		}

		for (Instruction &I : B) {
			if (isa<PHINode>(&I) == false) {
				continue;
			}
			PHINode *pn = cast<PHINode>(&I);
			unsigned nIncoming = pn->getNumIncomingValues();
			if (nIncoming > predCnt) {
				errs() << "repairPhiNodes: #incoming > #predecessors: loop and axe\n";

				std::vector<BasicBlock *> notIncomingAnymore;
				while (nIncoming > 0) {
					--nIncoming;

					/*
					 * Just remove incoming value/block pairs for those which are no
					 * longer predecessors.
					 */	
					bool incomingIsStillPred = false;
					BasicBlock *incomingBlock = pn->getIncomingBlock(nIncoming);
					for (auto spi = pred_begin(&B); spi != pred_end(&B); ++spi) {
						BasicBlock *itBB = *spi;
						if (itBB == incomingBlock) {
							incomingIsStillPred = true;
							break;
						}
					}
					if (incomingIsStillPred == false) {
						notIncomingAnymore.push_back(incomingBlock);
					}
				}
				for (auto nia = notIncomingAnymore.begin();
				  nia != notIncomingAnymore.end(); ++nia) {
					BasicBlock *niaBB = *nia;
					pn->removeIncomingValue(niaBB, true);
					changed = true;
				}
			}
		}
		errs() << "repairPHINodes: post fix bb.dump():\n";
		B.dump();
		entryBlock = false;
	}

	// I half wonder if I should be handling this else where....
	for (auto i = fullDeadBlocks.begin(); i != fullDeadBlocks.end(); ++i) {
		BasicBlock *bb = *i;
		for (auto refi = removeBlocks.begin(); refi != removeBlocks.end(); ++refi) {
			BBlockRef *r = &*refi;
			if (r->getBB() == bb && !r->isRemoved()) {
				r->setRemoved(true);
				break;
			}
		}
		// Delete regardless of finding it in our target removeBlocks list.
		// Possibly we should have this be a check for if we tried to
		// remove a block we wanted to keep
		DeleteDeadBlock(bb);
		changed = true;
	}
	return changed;
}

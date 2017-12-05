
/*
 * Adds decls for trays_init and trays_log and
 * inject calls to trays_init on entry point and
 * trays_log to the first-non-phinode instruction
 * in the BasicBlock. However you implement
 * libtrays is up to you, but the current API is
 * limited.
 *
 * Obviously this is all very basic and will impact
 * performance :-P
 *
 */
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/SymbolTableListTraits.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

#include "TraceBBInject.h"

static cl::opt<std::string> TraceLogFile("pskin-log-file",
  cl::desc("Specify trace log file"), cl::init("default_trace.log"),
  cl::Required);

static cl::opt<bool> TraceLogPC("pskin-log-pc",
  cl::init(false), cl::desc("log PC at cmp"));

bool
TraceBBInject::runOnModule(Module &M)
{
	LLVMContext &ctx = M.getContext();

	/*
	 * void trays_init(char *)
	 */
	Constant *libtraysInit = M.getOrInsertFunction("trays_init",
	  Type::getVoidTy(ctx),
	  Type::getInt8PtrTy(ctx),
	  NULL);

	/*
	 * void trays_log(char *, uint64_t)
	 */
	Constant *libtraysLog = M.getOrInsertFunction("trays_log",
	  Type::getVoidTy(ctx),
	  Type::getInt8PtrTy(ctx),
	  Type::getInt64Ty(ctx),
	  NULL);

	/*
	 * void trays_log_pc()
	 */
	Constant *libtraysLogPC = M.getOrInsertFunction("trays_log_pc",
	  Type::getVoidTy(ctx),
	  NULL);

	/*
	 * void trays_log_function_entry(char *)
	 *
	 * Gets PC via __builtin_return_address().
	 */
	Constant *libtraysLogEntry = M.getOrInsertFunction(
	  "trays_log_function_entry",
	  Type::getVoidTy(ctx),
	  Type::getInt8PtrTy(ctx),
	  NULL);

	/*
	 * void trays_log_function_exit(char *, uint64_t)
	 */
	Constant *libtraysLogExit = M.getOrInsertFunction("trays_log_function_exit",
	  Type::getVoidTy(ctx),
	  Type::getInt8PtrTy(ctx),
	  Type::getInt64Ty(ctx),
	  NULL);

	// Should this always be CallConv::C ???
	Function *libtraysLogPCFunction = cast<Function>(libtraysLogPC);
	libtraysLogPCFunction->setCallingConv(CallingConv::C);

	Function *libtraysInitFunction = cast<Function>(libtraysInit);
	libtraysInitFunction->setCallingConv(CallingConv::C);

	Function *libtraysLogFunction = cast<Function>(libtraysLog);
	libtraysLogFunction->setCallingConv(CallingConv::C);

	Function *libtraysLogEntryFunction = cast<Function>(libtraysLogEntry);
	libtraysLogEntryFunction->setCallingConv(CallingConv::C);

	Function *libtraysLogExitFunction = cast<Function>(libtraysLogExit);
	libtraysLogExitFunction->setCallingConv(CallingConv::C);

	/*
	 * Assume main is the entry point to what we are doing and 
	 * do any library initialization at the start of it.
	 * It's not necessary, but we make a BasicBlock for it.
	 */
	Function *mainFn = M.getFunction("main");
	if (mainFn == NULL) {
		errs() << "FATAL: No main function\n";
		return false;
	}
	/*
	 * Add a new block to main, at the start. We init libtrays there.
	 */
	BasicBlock *mainEntry = &mainFn->getEntryBlock();
	BasicBlock *initTraysBlock = BasicBlock::Create(ctx, "initLibTrays",
	  mainFn, mainEntry);
	IRBuilder<> builder(initTraysBlock);
	PointerType *t = Type::getInt8PtrTy(ctx, 0);
	Value *fileArg = ConstantPointerNull::get(t);
	Constant *fileNameStr = ConstantDataArray::getString(ctx, TraceLogFile,
	  true);
	GlobalVariable *gvFileNameStr = new GlobalVariable(M,
		fileNameStr->getType(), true,
		GlobalValue::PrivateLinkage, fileNameStr);
	gvFileNameStr->setName("_pskin_trace_file");
	gvFileNameStr->setAlignment(1);
	ArrayType *fsAt = ArrayType::get(Type::getInt8Ty(ctx),
		TraceLogFile.size() + 1);
	std::vector<Value *> fIdxs;
	fIdxs.push_back(
	  Constant::getIntegerValue(Type::getInt32Ty(ctx), APInt(32, 0, false))
	);
	fIdxs.push_back(
	  Constant::getIntegerValue(Type::getInt32Ty(ctx), APInt(32, 0, false))
	);
	Constant *useGvFName = ConstantExpr::getInBoundsGetElementPtr(fsAt,
	  gvFileNameStr, fIdxs);

//		useGvFName->dump();
	fileArg = useGvFName;

	builder.CreateCall(libtraysInitFunction,
	  { fileArg },
	  Twine::createNull(),
	  nullptr);
	builder.CreateBr(mainEntry);

	for (Function &F : M) {
		if (F.isDeclaration() || F.isIntrinsic()) {
			continue;
		}
		if (!F.hasName()) {	
			continue;
		}
		std::string fname = F.getName().str();
		BasicBlock *mainFirst = fname == "main" ? &F.front() : NULL;
		unsigned bbcnt = 0;

		for (BasicBlock &BB : F) {
			if (mainFirst && mainFirst == &BB) {
				continue;
			}
			// Increment after because we added a block to init libtrays
			++bbcnt;

			/* recall we always have a dummy instruction */
			Instruction *dummyIns = BB.getFirstNonPHI();
			Instruction *terminatorIns= BB.getTerminator();
			BasicBlock::iterator Boo(dummyIns);
			++Boo;
			Instruction *putBefore = &*Boo;

			// Create global that contains the char * string we want to log
/*
 * WE HAVE ISSUES WITH GEP HERE.. WHY?... but you're like there, dood, so ... all you did was fix newlines and then it 
 * hosed out. So, no idea wassaup thath
 */
			std::string blkname = cast<MDString>(dummyIns->getMetadata("fnblk")->getOperand(0))->getString().str();
			Constant *logMsg = ConstantDataArray::getString(ctx,
			  blkname, true);
			GlobalVariable *gvMsg = new GlobalVariable(M, logMsg->getType(),
			  true, GlobalValue::PrivateLinkage, logMsg);
			gvMsg->setName(blkname + "_str");
			gvMsg->setAlignment(1);
			ArrayType *at = ArrayType::get(Type::getInt8Ty(ctx),
			  blkname.size()+1);
			std::vector<Value *> idxs;
			idxs.push_back(
		 	  Constant::getIntegerValue(Type::getInt32Ty(ctx),
			  APInt(32, 0, false))
			);
			idxs.push_back(
			  Constant::getIntegerValue(Type::getInt32Ty(ctx),
			  APInt(32, 0, false))
			);
/*
			if (at != cast<SequentialType>(gvMsg[0].getType()->getScalarType())->getElementType()) {
				errs() << "Mismatch between GEP pointee pointer types! XXX\n";
				at->dump();
				gvMsg->dump();
				exit(-1);
			}
 */
			Constant *useGv = ConstantExpr::getInBoundsGetElementPtr(at,
			  gvMsg, idxs);
			if (bbcnt == 1) {
				CallInst::Create(libtraysLogEntryFunction, 
				  { useGv },
				  Twine::createNull(),
				  putBefore);
			} else {
				if (!isa<ReturnInst>(terminatorIns)) {
					CallInst::Create(libtraysLogFunction, 
					  { useGv, dummyIns },
					  Twine::createNull(),
					  putBefore); 
				}
			}

			// No continue from entry block because could just be 1 block.
			if (isa<ReturnInst>(terminatorIns)) {
				Value *v;
				if (bbcnt == 1) {
					v = Constant::getIntegerValue(Type::getInt64Ty(ctx),
					  APInt(64, 0, false));
				} else {
					v = dummyIns;
				}
				CallInst::Create(libtraysLogExitFunction, 
  				  { useGv, v },
				  Twine::createNull(),
				  terminatorIns); // dummyIns is first Instruction in BasicBlock
			}

		}
	}
	if (TraceLogPC) {
		for (Function &F : M) {
			for (BasicBlock &BB : F) {
				for (Instruction &I : BB) {
					if (isa<CmpInst>(I)) {
						CallInst::Create(libtraysLogPCFunction, 
						  Twine::createNull(),
						  &I);
					}
				}
			}
		}
	}
	return true; // Assume we did add something... come on now.. just assume.
}

char TraceBBInject::ID = 0;
static RegisterPass<TraceBBInject> XX("pskin-inject", 
  "inject libtrays into target bitcode");



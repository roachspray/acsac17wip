#ifndef _PRUNESTATS_H
#define	_PRUNESTATS_H
#include <iostream>
class FocusFunctionStats {
	std::string functionName;
	unsigned depth;
	unsigned blockCount;
	unsigned removedBlockCount;
	unsigned wantRemoveBlockCount;
	unsigned wantKeepBlockCount;

public:
	FocusFunctionStats(std::string f, unsigned d) :
	  functionName(f), depth(d), blockCount(0),
	  removedBlockCount(0), wantRemoveBlockCount(0),
	  wantKeepBlockCount(0) {}

	void setBlockCount(unsigned b) {
		blockCount = b;
	}
	unsigned getBlockCount() const {
		return blockCount;
	}
	unsigned getDepth() const {
		return depth;
	}
	std::string getFunctionName() const {
		return functionName;
	}
	void setRemovedBlockCount(unsigned c) {
		removedBlockCount = c;
	}
	unsigned getRemovedBlockCount() const {
		return removedBlockCount;
	}

	void setWantRemoveBlockCount(unsigned c) {
		wantRemoveBlockCount = c;
	}
	unsigned getWantRemoveBlockCount() const {
		return wantRemoveBlockCount;
	}
	void setWantKeepBlockCount(unsigned c) {
		wantKeepBlockCount = c;
	}
	unsigned getWantKeepBlockCount() const {
		return wantKeepBlockCount;
	}
	void print() const {
		std::cout << "focus function: " << functionName << "\n";
		std::cout << "  depth called: " << depth << "\n";
		std::cout << "  total blocks: " << blockCount << "\n";
		std::cout << "     want kept: " << wantKeepBlockCount << "\n";
		std::cout << "  want removed: " << wantRemoveBlockCount << "\n";
		std::cout << " total removed: " << removedBlockCount << "\n";
	}
};

class ModuleLevelStats {
	unsigned functionCount;
	unsigned totalRemovedBlockCount;
	unsigned totalRemovedFunctionCount;
	unsigned maxDepth;

public:
	ModuleLevelStats(unsigned md) : functionCount(0), totalRemovedBlockCount(0),
	  totalRemovedFunctionCount(0), maxDepth(md) {}

	void setFunctionCount(unsigned c) {
		functionCount = c;
	}

	unsigned getFunctionCount() const {
		return functionCount;
	}

	void setTotalRemovedBlockCount(unsigned c) {
		totalRemovedBlockCount = c;
	}

	unsigned getTotalRemovedBlockCount() const {
		return totalRemovedBlockCount;
	}
	void setTotalRemovedFunctionCount(unsigned c) {
		totalRemovedFunctionCount = c;
	}
	unsigned getTotalRemovedFunctionCount() const {
		return totalRemovedFunctionCount;
	}
	void print() const {
		std::cout << "              max depth: " << maxDepth << "\n";
		std::cout << "    number of functions: " << functionCount << "\n";
		std::cout << "   total removed blocks: " << totalRemovedBlockCount << "\n";
		std::cout << "total removed functions: " << totalRemovedFunctionCount << "\n";
	}
};
#endif

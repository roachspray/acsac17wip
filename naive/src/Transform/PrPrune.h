#ifndef	__PRPRUNE_H
#define	__PRPRUNE_H

class PrPrune {
public:
	static PrPrune *get(std::string pf, std::string initMaterial);
	virtual bool shouldPrune(unsigned depth, unsigned blockCount) = 0;
};

// Put these elsewhere, but whatever.
class NoOpPrPrune : public PrPrune {
public:
	NoOpPrPrune() { }
	virtual bool shouldPrune(unsigned depth, unsigned blockCount);
};

class DecreasingWeightedCoinFlip : public PrPrune {
	float mean;
	long int weightedBarrier;

public:
	DecreasingWeightedCoinFlip(float mmean) : mean(mmean) {
		srandom(time(NULL));	// Weak, but we do not care.
	}
	virtual bool shouldPrune(unsigned depth, unsigned blockCount);
};

#endif

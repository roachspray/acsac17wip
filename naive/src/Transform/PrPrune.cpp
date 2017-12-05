/*
 * Super lame.
 */

#include <stdlib.h>
#include <limits.h>

#include <string>
#include "PrPrune.h"


bool
NoOpPrPrune::shouldPrune(unsigned depth, unsigned blockCount)
{
	return true;
}

/*
 * If random value is under the barrier, we return True.
 * As depth increases, as blockCount increases, less likely to prune.
 */
bool
DecreasingWeightedCoinFlip::shouldPrune(unsigned depth, unsigned blockCount)
{
	long int rv = random();
	long int scaleRandMax;

	/* Not pretty. Follows no rules. Prolly goes negative :D Enjoy! ;-; */
	// XXX
	scaleRandMax = RAND_MAX - (depth - 1)*10 - (depth - 1)*(blockCount * 4);
	weightedBarrier = (long int)(mean * scaleRandMax);
	
	if (rv < weightedBarrier) {
		return true;
	}
	return false;
}

/* TEMPLATE THIS SHIT */
// Not calling pdf bc not sure if i will adhere to standard pdf definition
// This whole init material is a string is f'd.
// I should template or something, I had that going efore, but it was not
// on the right path, so I just whacked it.
PrPrune *
PrPrune::get(std::string pf, std::string initMaterial) {
	if (pf == "") {
		return NULL;
	}

	if (pf == "NoOpPrPrune") {
		return (PrPrune *)new NoOpPrPrune();
	}

	if (pf == "DecreasingWeightedCoinFlip") {
		return (PrPrune *)new DecreasingWeightedCoinFlip(
		  std::stof(initMaterial, 0)
		);
	}
	return NULL;
}

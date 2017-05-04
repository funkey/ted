#ifndef TED_EVALUATION_RAND_INDEX_H__
#define TED_EVALUATION_RAND_INDEX_H__

#include <imageprocessing/ImageStack.h>
#include "RandIndexErrors.h"

class RandIndex {

public:

	RandIndex(bool ignoreBackground = false);

	RandIndexErrors compute(const ImageStack& groundTruth, const ImageStack& reconstruction);

private:

	uint64_t getNumAgreeingPairs(
			const ImageStack& stack1,
			const ImageStack& stack2,
			uint64_t& numLocations,
			uint64_t& numSameComponentPairs1,
			uint64_t& numSameComponentPairs2,
			uint64_t& numSameComponentPairs12);

	// do not count statistics for pixels that belong to the background
	bool _ignoreBackground;
};

#endif // TED_EVALUATION_RAND_INDEX_H__


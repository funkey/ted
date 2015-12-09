#ifndef TED_EVALUATION_RAND_INDEX_H__
#define TED_EVALUATION_RAND_INDEX_H__

#include <pipeline/all.h>
#include <imageprocessing/ImageStack.h>
#include "RandIndexErrors.h"

class RandIndex : public pipeline::SimpleProcessNode<> {

public:

	/**
	 * Create a new evaluator.
	 *
	 * @param headerOnly
	 *              If set to true, no error will be computed, only the header 
	 *              information in Errors::errorHeader() will be set.
	 */
	RandIndex(bool headerOnly = false);

private:

	void updateOutputs();

	size_t getNumAgreeingPairs(
			const ImageStack& stack1,
			const ImageStack& stack2,
			size_t& numLocations,
			size_t& numSameComponentPairs1,
			size_t& numSameComponentPairs2,
			size_t& numSameComponentPairs12);

	// input image stacks
	pipeline::Input<ImageStack> _reconstruction;
	pipeline::Input<ImageStack> _groundTruth;

	pipeline::Output<RandIndexErrors> _errors;

	// do not count statistics for pixels that belong to the background
	bool _ignoreBackground;

	bool _headerOnly;
};

#endif // TED_EVALUATION_RAND_INDEX_H__


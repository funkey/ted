#ifndef TED_EVALUATION_VARIATION_OF_INFORMATION_H__
#define TED_EVALUATION_VARIATION_OF_INFORMATION_H__

#include <pipeline/all.h>
#include <imageprocessing/ImageStack.h>
#include "VariationOfInformationErrors.h"

class VariationOfInformation : public pipeline::SimpleProcessNode<> {

	typedef std::map<size_t, double>                    LabelProb;
	typedef std::map<std::pair<size_t, size_t>, double> JointLabelProb;

public:

	/**
	 * Create a new evaluator.
	 *
	 * @param headerOnly
	 *              If set to true, no error will be computed, only the header 
	 *              information in Errors::errorHeader() will be set.
	 */
	VariationOfInformation(bool headerOnly = false, bool ignoreBackground = false);

private:

	void updateOutputs();

	// input image stacks
	pipeline::Input<ImageStack> _reconstruction;
	pipeline::Input<ImageStack> _groundTruth;

	pipeline::Output<VariationOfInformationErrors> _errors;

	// label probabilities
	LabelProb      _p1;
	LabelProb      _p2;
	JointLabelProb _p12;

	// do not count statistics for pixels that belong to the background
	bool _ignoreBackground;

	bool _headerOnly;
};

#endif // TED_EVALUATION_VARIATION_OF_INFORMATION_H__


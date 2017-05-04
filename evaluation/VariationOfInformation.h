#ifndef TED_EVALUATION_VARIATION_OF_INFORMATION_H__
#define TED_EVALUATION_VARIATION_OF_INFORMATION_H__

#include <imageprocessing/ImageStack.h>
#include "VariationOfInformationErrors.h"

class VariationOfInformation {

	typedef std::map<size_t, double>                    LabelProb;
	typedef std::map<std::pair<size_t, size_t>, double> JointLabelProb;

public:

	VariationOfInformation(bool ignoreBackground = false);

	VariationOfInformationErrors compute(const ImageStack& groundTruth, const ImageStack& reconstruction);

private:

	// label probabilities
	LabelProb      _p1;
	LabelProb      _p2;
	JointLabelProb _p12;

	// do not count statistics for pixels that belong to the background
	bool _ignoreBackground;

	bool _headerOnly;
};

#endif // TED_EVALUATION_VARIATION_OF_INFORMATION_H__


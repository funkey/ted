#ifndef TED_EVALUATION_VARIATION_OF_INFORMATION_ERRORS_H__
#define TED_EVALUATION_VARIATION_OF_INFORMATION_ERRORS_H__

#include <sstream>
#include <iomanip>

class VariationOfInformationErrors {

public:

	VariationOfInformationErrors() :
			_splitEntropy(0),
			_mergeEntropy(0) {}

	/**
	 * Set the conditional entropy H(A|B), where A is the reconstruction label 
	 * distribution and B is the ground truth label distribution.
	 */
	void setSplitEntropy(double splitEntropy) { _splitEntropy = splitEntropy; }

	/**
	 * Set the conditional entropy H(B|A), where A is the reconstruction label 
	 * distribution and B is the ground truth label distribution.
	 */
	void setMergeEntropy(double mergeEntropy) { _mergeEntropy = mergeEntropy; }

	/**
	 * Get the conditional entropy H(A|B), where A is the reconstruction label 
	 * distribution and B is the ground truth label distribution.
	 */
	double getSplitEntropy() { return _splitEntropy; }

	/**
	 * Get the conditional entropy H(B|A), where A is the reconstruction label 
	 * distribution and B is the ground truth label distribution.
	 */
	double getMergeEntropy() { return _mergeEntropy; }

	/**
	 * Get the total entropy, i.e., the variation of information.
	 */
	double getEntropy() { return _splitEntropy + _mergeEntropy; }

private:

	double _splitEntropy;
	double _mergeEntropy;
};

#endif // TED_EVALUATION_VARIATION_OF_INFORMATION_ERRORS_H__


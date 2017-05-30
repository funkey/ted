#ifndef TED_EVALUATION_SKELETON_TOLERANCE_FUNCTION_H__
#define TED_EVALUATION_SKELETON_TOLERANCE_FUNCTION_H__

#include "DistanceToleranceFunction.h"

/**
 * Specialization of a distance tolerance function that operates on skeleton 
 * ground truth. The distance tolerance criterion specifies how far away a 
 * skeleton is allowed to be from the reconstruction without causing an error.
 */
class SkeletonToleranceFunction : public DistanceToleranceFunction {

public:

	SkeletonToleranceFunction(
			float distanceThreshold,
			size_t gtBackgroundLabel) :
		DistanceToleranceFunction(
				distanceThreshold,
				false), /* don't allow background appearance */
		_gtBackgroundLabel(gtBackgroundLabel),
		_ignoreLabel((size_t)(-1)) {}

//private:

	//// for the skeleton criterion, each cell is allowed to be relabeled
	//virtual void findRelabelCandidates(const std::vector<float>& maxBoundaryDistances);

	//bool isSkeletonCell(unsigned int cellIndex);

	size_t _gtBackgroundLabel;

	// label used for non-skeleton cells, which should be ignored
	size_t _ignoreLabel;
};

#endif // TED_EVALUATION_SKELETON_TOLERANCE_FUNCTION_H__


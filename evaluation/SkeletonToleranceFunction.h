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
			size_t backgroundLabel = 0) :
		DistanceToleranceFunction(
				distanceThreshold,
				true, /* have background label */
				backgroundLabel) {}

private:

	// for the skeleton criterion, each cell is allowed to be relabeled
	virtual void findRelabelCandidates(const std::vector<float>& maxBoundaryDistances);

	bool isSkeletonCell(unsigned int cellIndex);
};

#endif // TED_EVALUATION_SKELETON_TOLERANCE_FUNCTION_H__


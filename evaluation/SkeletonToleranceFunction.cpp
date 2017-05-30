#include "SkeletonToleranceFunction.h"
#include <util/Logger.h>

logger::LogChannel skeletontolerancelog("skeletontolerancelog", "[SkeletonToleranceFunction] ");

void
SkeletonToleranceFunction::findRelabelCandidates(const std::vector<float>& maxBoundaryDistances) {

	LOG_DEBUG(skeletontolerancelog) << "finding relabel candidates..." << std::endl;

	_relabelCandidates.clear();
	for (unsigned int cellIndex = 0; cellIndex < maxBoundaryDistances.size(); cellIndex++) {

		if (isSkeletonCell(cellIndex)) {

			// add all skeleton cells to the relabel candidates
			_relabelCandidates.push_back(cellIndex);

		} else {

			// non-sekelton cells are hard-wired to the ignore label, there is 
			// nothing to do for them
			cell_t& cell = (*_cells)[cellIndex];
			cell.setReconstructionLabel(_ignoreLabel);
			cell.setGroundTruthLabel(_ignoreLabel);
		}
	}
	registerPossibleMatch(_ignoreLabel, _ignoreLabel);

	LOG_DEBUG(skeletontolerancelog) << "done" << std::endl;
}

bool
SkeletonToleranceFunction::isSkeletonCell(unsigned int cellIndex) {

	// a cell is a skeleton cell, if its ground truth label is not the 
	// background
	return (*_cells)[cellIndex].getGroundTruthLabel() != _gtBackgroundLabel;
}

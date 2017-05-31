#include "SkeletonToleranceFunction.h"
#include <util/Logger.h>

logger::LogChannel skeletontolerancelog("skeletontolerancelog", "[SkeletonToleranceFunction] ");

void
SkeletonToleranceFunction::initializeCellLabels(std::shared_ptr<Cells> cells) {

	LOG_DEBUG(skeletontolerancelog) << "intializing cells..." << std::endl;

	for (unsigned int cellIndex = 0; cellIndex < cells->size(); cellIndex++) {

		Cell<size_t>& cell = (*cells)[cellIndex];

		// not a skeleton cell?
		if (cell.getGroundTruthLabel() == _gtBackgroundLabel) {

			// non-sekeleton cells are hard-wired to the ignore label, there is 
			// nothing to do for them
			cell.setReconstructionLabel(_ignoreLabel);
			cell.setGroundTruthLabel(_ignoreLabel);
		}

		cell.addPossibleLabel(cell.getReconstructionLabel());
	}

}

std::vector<size_t>
SkeletonToleranceFunction::findRelabelCandidates(
		std::shared_ptr<Cells> cells,
		const ImageStack& recLabels,
		const ImageStack& gtLabels) {

	LOG_DEBUG(skeletontolerancelog) << "finding relabel candidates..." << std::endl;

	std::vector<size_t> relabelCandidates;
	for (unsigned int cellIndex = 0; cellIndex < cells->size(); cellIndex++) {

		Cell<size_t>& cell = (*cells)[cellIndex];

		// a skeleton cell?
		if (cell.getGroundTruthLabel() != _gtBackgroundLabel) {

			// add all skeleton cells to the relabel candidates
			relabelCandidates.push_back(cellIndex);

		} else {

			// non-sekeleton cells are hard-wired to the ignore label, there is 
			// nothing to do for them
			cell.setReconstructionLabel(_ignoreLabel);
			cell.setGroundTruthLabel(_ignoreLabel);
		}
	}

	LOG_DEBUG(skeletontolerancelog) << "done" << std::endl;

	return relabelCandidates;
}

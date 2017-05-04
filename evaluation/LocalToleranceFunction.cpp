#include "LocalToleranceFunction.h"

void
LocalToleranceFunction::clear() {

	_groundTruthLabels.clear();
	_reconstructionLabels.clear();
	_cells = std::make_shared<std::vector<cell_t> >();
	_possibleGroundTruthMatches.clear();
	_possibleReconstructionMatches.clear();
	_cellsByRecToGtLabel.clear();
}

std::set<size_t>&
LocalToleranceFunction::getReconstructionLabels() {

	return _reconstructionLabels;
}

std::set<size_t>&
LocalToleranceFunction::getGroundTruthLabels() {

	return _groundTruthLabels;
}

void
LocalToleranceFunction::registerPossibleMatch(size_t gtLabel, size_t recLabel) {

	_possibleGroundTruthMatches[gtLabel].insert(recLabel);
	_possibleReconstructionMatches[recLabel].insert(gtLabel);
	_groundTruthLabels.insert(gtLabel);
	_reconstructionLabels.insert(recLabel);
}

std::set<size_t>&
LocalToleranceFunction::getPossibleMatchesByGt(size_t gtLabel) {

	return _possibleGroundTruthMatches[gtLabel];
}

std::set<size_t>&
LocalToleranceFunction::getPossibleMathesByRec(size_t recLabel) {

	return _possibleReconstructionMatches[recLabel];
}

#include <algorithm>

#include <inference/LinearConstraints.h>
#include <inference/LinearObjective.h>
#include <inference/LinearSolverBackend.h>
#include <inference/SolverFactory.h>
#include <inference/Solution.h>
#include <util/exceptions.h>
#include <util/Logger.h>
#include "TolerantEditDistance.h"
#include "DistanceToleranceFunction.h"
#include "SkeletonToleranceFunction.h"
#include "Cells.h"

logger::LogChannel tedlog("tedlog", "[TolerantEditDistance] ");

TolerantEditDistance::TolerantEditDistance(const Parameters& parameters) :
	_parameters(parameters) {

	if (_parameters.fromSkeleton){

		_toleranceFunction = std::unique_ptr<LocalToleranceFunction>(
				new SkeletonToleranceFunction(
						_parameters.distanceThreshold,
						_parameters.gtBackgroundLabel));

		LOG_ALL(tedlog) << "created TolerantEditDistance for skeleton ground-truth" << std::endl;

	} else {

		_toleranceFunction = std::unique_ptr<LocalToleranceFunction>(
				new DistanceToleranceFunction(
						_parameters.distanceThreshold,
						_parameters.allowBackgroundAppearance,
						_parameters.recBackgroundLabel));

		LOG_ALL(tedlog) << "created TolerantEditDistance for volumetric ground-truth" << std::endl;
	}
}

TolerantEditDistanceErrors
TolerantEditDistance::compute(const ImageStack& groundTruth, const ImageStack& reconstruction) {

	reset(groundTruth, reconstruction);

	std::shared_ptr<Cells> cells = _toleranceFunction->extractCells(groundTruth, reconstruction);

	minimizeErrors(*cells);

	correctReconstruction(*cells);

	return findErrors(cells);
}

void
TolerantEditDistance::reset(const ImageStack& groundTruth, const ImageStack& reconstruction) {

	if (groundTruth.size() != reconstruction.size())
		BOOST_THROW_EXCEPTION(SizeMismatchError() << error_message("ground truth and reconstruction have different size") << STACK_TRACE);

	if (groundTruth.height() != reconstruction.height() || groundTruth.width() != reconstruction.width())
		BOOST_THROW_EXCEPTION(SizeMismatchError() << error_message("ground truth and reconstruction have different size") << STACK_TRACE);

	_depth  = groundTruth.size();
	_width  = groundTruth.width();
	_height = groundTruth.height();

	_indicatorVarsByRecLabel.clear();
	_indicatorVarsByGtToRecLabel.clear();
	_matchVars.clear();
	_labelingByVar.clear();
	_alternativeIndicators.clear();
	_correctedReconstruction.clear();
	_splitLocations.clear();
	_mergeLocations.clear();
	_fpLocations.clear();
	_fnLocations.clear();
}

void
TolerantEditDistance::minimizeErrors(const Cells& cells) {

	std::set<size_t> reconstructionLabels;
	std::set<size_t> groundTruthLabels;
	std::map<size_t, std::set<size_t>> possibleMatchesByGt;
	std::map<size_t, std::set<size_t>> possibleMatchesByRec;

	for (const auto& cell : cells) {

		reconstructionLabels.insert(cell.getReconstructionLabel());
		groundTruthLabels.insert(cell.getGroundTruthLabel());

		for (size_t l : cell.getPossibleLabels()) {

			possibleMatchesByGt[cell.getGroundTruthLabel()].insert(cell.getReconstructionLabel());
			possibleMatchesByRec[cell.getReconstructionLabel()].insert(cell.getGroundTruthLabel());
		}
	}

	LOG_DEBUG(tedlog)
			<< "found "
			<< groundTruthLabels.size()
			<< " ground truth labels and "
			<< reconstructionLabels.size()
			<< " reconstruction labels"
			<< std::endl;

	LinearConstraints constraints;

	// the default are binary variables
	std::map<unsigned int, VariableType> specialVariableTypes;

	// introduce indicators for each cell and each possible label of that cell
	unsigned int var = 0;
	for (unsigned int cellIndex = 0; cellIndex < cells.size(); cellIndex++) {

		const Cell<size_t>& cell = cells[cellIndex];

		// first indicator variable for this cell
		unsigned int begin = var;

		//// one variable for the default label
		//assignIndicatorVariable(var++, cellIndex, cell.getGroundTruthLabel(), cell.getReconstructionLabel());

		// one variable for each possible label
		for (size_t l : cell.getPossibleLabels()) {

			unsigned int ind = var++;
			assignIndicatorVariable(ind, cellIndex, cell.getGroundTruthLabel(), l);

			if (l != cell.getReconstructionLabel())
				_alternativeIndicators.push_back(std::make_pair(ind, cell.size()));
		}

		// last +1 indicator variable for this cell
		unsigned int end = var;

		if (begin == end)
			UTIL_THROW_EXCEPTION(Exception, "cell " << cellIndex << " has no possible labels");

		// every cell needs to have a label
		LinearConstraint constraint;
		for (unsigned int i = begin; i < end; i++)
			constraint.setCoefficient(i, 1.0);
		constraint.setRelation(Equal);
		constraint.setValue(1);
		constraints.add(constraint);

		//LOG_ALL(tedlog) << constraint << std::endl;
	}
	_numIndicatorVars = var;

	//LOG_ALL(tedlog) << "adding constraints to ensure that rec labels don't disappear" << std::endl;

	// labels can not disappear
	for (size_t recLabel : reconstructionLabels) {

		LinearConstraint constraint;
		for (unsigned int v : getIndicatorsByRec(recLabel))
			constraint.setCoefficient(v, 1.0);
		constraint.setRelation(GreaterEqual);
		constraint.setValue(1);
		constraints.add(constraint);

		//LOG_ALL(tedlog) << constraint << std::endl;
	}

	// introduce indicators for each match of ground truth label to 
	// reconstruction label
	for (size_t gtLabel : groundTruthLabels)
		for (size_t recLabel : possibleMatchesByGt[gtLabel])
			assignMatchVariable(var++, gtLabel, recLabel);

	// cell label selection activates match
	for (size_t gtLabel : groundTruthLabels) {
		for (size_t recLabel : possibleMatchesByGt[gtLabel]) {

			unsigned int matchVar = getMatchVariable(gtLabel, recLabel);

			// no assignment of gtLabel to recLabel -> match is zero
			LinearConstraint noMatchConstraint;

			for (unsigned int v : getIndicatorsGtToRec(gtLabel, recLabel)) {

				noMatchConstraint.setCoefficient(v, 1);

				// at least one assignment of gtLabel to recLabel -> match is 
				// one
				LinearConstraint matchConstraint;
				matchConstraint.setCoefficient(matchVar, 1);
				matchConstraint.setCoefficient(v, -1);
				matchConstraint.setRelation(GreaterEqual);
				matchConstraint.setValue(0);
				constraints.add(matchConstraint);

				//LOG_ALL(tedlog) << matchConstraint << std::endl;
			}

			noMatchConstraint.setCoefficient(matchVar, -1);
			noMatchConstraint.setRelation(GreaterEqual);
			noMatchConstraint.setValue(0);
			constraints.add(noMatchConstraint);

			//LOG_ALL(tedlog) << noMatchConstraint << std::endl;
		}
	}

	// introduce split number for each ground truth label

	unsigned int splitBegin = var;

	for (size_t gtLabel : groundTruthLabels) {

		unsigned int splitVar = var++;

		//LOG_ALL(tedlog) << "adding split var " << splitVar << " for gt label " << gtLabel << std::endl;

		specialVariableTypes[splitVar] = Integer;

		LinearConstraint positive;
		positive.setCoefficient(splitVar, 1);
		positive.setRelation(GreaterEqual);
		positive.setValue(0);
		constraints.add(positive);

		LinearConstraint numSplits;
		numSplits.setCoefficient(splitVar, 1);
		for (size_t recLabel : possibleMatchesByGt[gtLabel])
			numSplits.setCoefficient(getMatchVariable(gtLabel, recLabel), -1);
		numSplits.setRelation(Equal);
		numSplits.setValue(-1);
		constraints.add(numSplits);

		//LOG_ALL(tedlog) << positive << std::endl;
		//LOG_ALL(tedlog) << numSplits << std::endl;
	}

	unsigned int splitEnd = var;

	// introduce total split number

	_splits = var++;
	specialVariableTypes[_splits] = Integer;

	//LOG_ALL(tedlog) << "adding total split var " << _splits << std::endl;

	LinearConstraint sumOfSplits;
	sumOfSplits.setCoefficient(_splits, 1);
	for (unsigned int i = splitBegin; i < splitEnd; i++)
		sumOfSplits.setCoefficient(i, -1);
	sumOfSplits.setRelation(Equal);
	sumOfSplits.setValue(0);
	constraints.add(sumOfSplits);

	//LOG_ALL(tedlog) << sumOfSplits << std::endl;

	// introduce merge number for each reconstruction label

	unsigned int mergeBegin = var;

	for (size_t recLabel : reconstructionLabels) {

		unsigned int mergeVar = var++;

		//LOG_ALL(tedlog) << "adding merge var " << mergeVar << " for rec label " << recLabel << std::endl;

		specialVariableTypes[mergeVar] = Integer;

		LinearConstraint positive;
		positive.setCoefficient(mergeVar, 1);
		positive.setRelation(GreaterEqual);
		positive.setValue(0);
		constraints.add(positive);

		LinearConstraint numMerges;
		numMerges.setCoefficient(mergeVar, 1);
		for (size_t gtLabel : possibleMatchesByRec[recLabel])
			numMerges.setCoefficient(getMatchVariable(gtLabel, recLabel), -1);
		numMerges.setRelation(Equal);
		numMerges.setValue(-1);
		constraints.add(numMerges);

		//LOG_ALL(tedlog) << positive << std::endl;
		//LOG_ALL(tedlog) << numMerges << std::endl;
	}

	unsigned int mergeEnd = var;

	// introduce total merge number

	_merges = var++;
	specialVariableTypes[_merges] = Integer;

	//LOG_ALL(tedlog) << "adding total merge var " << _splits << std::endl;

	LinearConstraint sumOfMerges;
	sumOfMerges.setCoefficient(_merges, 1);
	for (unsigned int i = mergeBegin; i < mergeEnd; i++)
		sumOfMerges.setCoefficient(i, -1);
	sumOfMerges.setRelation(Equal);
	sumOfMerges.setValue(0);
	constraints.add(sumOfMerges);

	//LOG_ALL(tedlog) << sumOfMerges << std::endl;

	// create objective

	LinearObjective objective(var);

	// we want to minimize the number of split and merges
	objective.setCoefficient(_splits, 1);
	objective.setCoefficient(_merges, 1);
	// however, if there are multiple equal solutions, we prefer the ones with 
	// the least changes -- therefore, we add a small value for each of those 
	// variables that can not sum up to one and therefor does not change the 
	// number of splits and merges
	unsigned int ind;
	size_t cellSize;
	double volumeSize = _width*_height*_depth;
	for (auto& p : _alternativeIndicators) {
	
		ind = p.first;
		cellSize = p.second;

		objective.setCoefficient(ind, static_cast<double>(cellSize)/(volumeSize + 1));
	}
	objective.setSense(Minimize);

	// solve

	SolverFactory factory;
	std::unique_ptr<LinearSolverBackend> solver = std::unique_ptr<LinearSolverBackend>(factory.createLinearSolverBackend());

	solver->initialize(var, Binary, specialVariableTypes);
	solver->setObjective(objective);
	solver->setConstraints(constraints);

	std::string msg;
	if (!solver->solve(_solution, msg)) {

		LOG_ERROR(tedlog) << "Optimal solution NOT found: " << msg << std::endl;
	}
}

void
TolerantEditDistance::correctReconstruction(const Cells& cells) {

	// prepare output image

	for (unsigned int i = 0; i < _depth; i++) {

		_correctedReconstruction.add(std::make_shared<Image>(_width, _height, 0.0));
	}

	// read solution

	for (unsigned int i = 0; i < _numIndicatorVars; i++) {

		if (_solution[i]) {

			unsigned int  cellIndex  = _labelingByVar[i].first;
			size_t        recLabel   = _labelingByVar[i].second;
			const Cell<size_t>& cell = cells[cellIndex];

			for (const Cell<size_t>::Location& l : cell)
				(*_correctedReconstruction[l.z])(l.x, l.y) = recLabel;
		}
	}
}

TolerantEditDistanceErrors
TolerantEditDistance::findErrors(std::shared_ptr<Cells> pcells) {

	const Cells& cells = *pcells;

	TolerantEditDistanceErrors errors;
	if (_parameters.reportFPsFNs)
		errors = TolerantEditDistanceErrors(_parameters.gtBackgroundLabel, _parameters.recBackgroundLabel);

	// prepare error location image stack

	for (unsigned int i = 0; i < _depth; i++) {

		// initialize with gray (no cell label)
		_splitLocations.add(std::make_shared<Image>(_width, _height, 0.33));
		_mergeLocations.add(std::make_shared<Image>(_width, _height, 0.33));
		_fpLocations.add(std::make_shared<Image>(_width, _height, 0.33));
		_fnLocations.add(std::make_shared<Image>(_width, _height, 0.33));
	}

	// prepare error data structure

	errors.setCells(pcells);

	// fill error data structure

	for (unsigned int i = 0; i < _numIndicatorVars; i++) {

		if (_solution[i]) {

			unsigned int cellIndex = _labelingByVar[i].first;
			size_t       recLabel  = _labelingByVar[i].second;

			errors.addMapping(cellIndex, recLabel);
		}
	}

	// fill error location image stack

	// all cells that changed label within tolerance

	// all cells that split the ground truth
	for (size_t gtLabel : errors.getSplitLabels())
		for (const auto& errorCells : errors.getSplitCells(gtLabel))
			for (unsigned int cellIndex : errorCells.second)
				for (const Cell<size_t>::Location& l : cells[cellIndex])
					(*_splitLocations[l.z])(l.x, l.y) = errorCells.first;

	// all cells that split the reconstruction
	for (size_t recLabel : errors.getMergeLabels())
		for (const auto& errorCells : errors.getMergeCells(recLabel))
			for (unsigned int cellIndex : errorCells.second)
				for (const Cell<size_t>::Location& l : cells[cellIndex])
					(*_mergeLocations[l.z])(l.x, l.y) = errorCells.first;

	if (_parameters.reportFPsFNs) {

		// all cells that are false positives
		for (const auto& errorCells : errors.getFalsePositiveCells())
			if (errorCells.first != _parameters.recBackgroundLabel) {
				for (unsigned int cellIndex : errorCells.second)
					for (const Cell<size_t>::Location& l : cells[cellIndex])
						(*_fpLocations[l.z])(l.x, l.y) = errorCells.first;
			}

		// all cells that are false negatives
		for (const auto& errorCells : errors.getFalseNegativeCells())
			if (errorCells.first != _parameters.gtBackgroundLabel) {
				for (unsigned int cellIndex : errorCells.second)
					for (const Cell<size_t>::Location& l : cells[cellIndex])
						(*_fnLocations[l.z])(l.x, l.y) = errorCells.first;
			}
	}

	errors.setInferenceTime(_solution.getTime());
	errors.setNumVariables(_solution.size());

	return errors;
}

void
TolerantEditDistance::assignIndicatorVariable(unsigned int var, unsigned int cellIndex, size_t gtLabel, size_t recLabel) {

	//LOG_ALL(tedlog) << "adding indicator var " << var << " to assign label " << recLabel << " to cell " << cellIndex << std::endl;

	_indicatorVarsByRecLabel[recLabel].push_back(var);
	_indicatorVarsByGtToRecLabel[gtLabel][recLabel].push_back(var);

	_labelingByVar[var] = std::make_pair(cellIndex, recLabel);
}

std::vector<unsigned int>&
TolerantEditDistance::getIndicatorsByRec(size_t recLabel) {

	return _indicatorVarsByRecLabel[recLabel];
}

std::vector<unsigned int>&
TolerantEditDistance::getIndicatorsGtToRec(size_t gtLabel, size_t recLabel) {

	return _indicatorVarsByGtToRecLabel[gtLabel][recLabel];
}

void
TolerantEditDistance::assignMatchVariable(unsigned int var, size_t gtLabel, size_t recLabel) {

	//LOG_ALL(tedlog) << "adding indicator var " << var << " to match gt label " << gtLabel << " to rec label " << recLabel << std::endl;

	_matchVars[gtLabel][recLabel] = var;
}

unsigned int
TolerantEditDistance::getMatchVariable(size_t gtLabel, size_t recLabel) {

	return _matchVars[gtLabel][recLabel];
}

#include <algorithm>

#include <boost/range/adaptors.hpp>
#include <boost/timer/timer.hpp>
#include <boost/tuple/tuple.hpp>
#include <vigra/multi_labeling.hxx>

#include <inference/LinearConstraints.h>
#include <inference/LinearObjective.h>
#include <inference/LinearSolver.h>
#include <pipeline/Value.h>
#include <util/exceptions.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include "TolerantEditDistance.h"
#include "DistanceToleranceFunction.h"
#include "SkeletonToleranceFunction.h"

logger::LogChannel tedlog("tedlog", "[TolerantEditDistance] ");

util::ProgramOption optionGroundTruthFromSkeletons(
		util::_module           = "evaluation",
		util::_long_name        = "groundTruthFromSkeletons",
		util::_description_text = "Indicates that the ground-truth consists of skeletons only.");

util::ProgramOption optionToleranceDistanceThreshold(
		util::_module           = "evaluation",
		util::_long_name        = "maxBoundaryShift",
		util::_description_text = "The maximum allowed distance for a boundary shift in image stack units. The default number of units per voxel is 1.0. "
		                          "This can be changed by placing a file META in the image stack directory, with the keys 'resX=<..>', "
		                          "'resY=<...>', and 'resZ=<...>', which give the number of units per edge of a voxel.",
		util::_default_value    = 10);

util::ProgramOption optionHaveBackgroundLabel(
		util::_module           = "evaluation",
		util::_long_name        = "haveBackgroundLabel",
		util::_description_text = "Indicates that there is a background label with a default value of 0.",
		util::_default_value    = false);

util::ProgramOption optionGroundTruthBackgroundLabel(
		util::_module           = "evaluation",
		util::_long_name        = "groundTruthBackgroundLabel",
		util::_description_text = "The value of the ground-truth background label.",
		util::_default_value    = 0.0);

util::ProgramOption optionReconstructionBackgroundLabel(
		util::_module           = "evaluation",
		util::_long_name        = "reconstructionBackgroundLabel",
		util::_description_text = "The value of the reconstruction background label.",
		util::_default_value    = 0.0);

TolerantEditDistance::TolerantEditDistance(bool headerOnly) :
	_haveBackgroundLabel(optionHaveBackgroundLabel || optionGroundTruthFromSkeletons),
	_gtBackgroundLabel(optionGroundTruthBackgroundLabel),
	_recBackgroundLabel(optionReconstructionBackgroundLabel),
	_correctedReconstruction(new ImageStack()),
	_splitLocations(new ImageStack()),
	_mergeLocations(new ImageStack()),
	_fpLocations(new ImageStack()),
	_fnLocations(new ImageStack()),
	_errors(_haveBackgroundLabel ? new TolerantEditDistanceErrors(_gtBackgroundLabel, _recBackgroundLabel) : new TolerantEditDistanceErrors()),
	_headerOnly(headerOnly) {

	if (optionHaveBackgroundLabel) {
		LOG_ALL(tedlog) << "started TolerantEditDistance with background label" << std::endl;
	} else {
		LOG_ALL(tedlog) << "started TolerantEditDistance without background label" << std::endl;
	}

	if (!_headerOnly) {

		registerInput(_groundTruth, "ground truth");
		registerInput(_reconstruction, "reconstruction");

		registerOutput(_correctedReconstruction, "corrected reconstruction");
		registerOutput(_splitLocations, "splits");
		registerOutput(_mergeLocations, "merges");
		registerOutput(_fpLocations, "false positives");
		registerOutput(_fnLocations, "false negatives");
	}

	registerOutput(_errors, "errors");

	if (optionGroundTruthFromSkeletons)
		_toleranceFunction = new SkeletonToleranceFunction(optionToleranceDistanceThreshold.as<float>(), _recBackgroundLabel);
	else
		_toleranceFunction = new DistanceToleranceFunction(optionToleranceDistanceThreshold.as<float>(), _haveBackgroundLabel, _recBackgroundLabel);
}

TolerantEditDistance::~TolerantEditDistance() {

	delete _toleranceFunction;
}

void
TolerantEditDistance::updateOutputs() {

	if (_headerOnly)
		return;

	clear();

	extractCells();

	findBestCellLabels();

	correctReconstruction();

	findErrors();
}

void
TolerantEditDistance::clear() {

	_toleranceFunction->clear();
	_indicatorVarsByRecLabel.clear();
	_indicatorVarsByGtToRecLabel.clear();
	_matchVars.clear();
	_labelingByVar.clear();
	_alternativeIndicators.clear();
	_errors->clear();
	_correctedReconstruction->clear();
	_splitLocations->clear();
	_mergeLocations->clear();
	_fpLocations->clear();
	_fnLocations->clear();
}

void
TolerantEditDistance::extractCells() {

	//boost::timer::auto_cpu_timer timer(std::cout, "\textractCells():\t\t\t\t%ws\n");

	if (_groundTruth->size() != _reconstruction->size())
		BOOST_THROW_EXCEPTION(SizeMismatchError() << error_message("ground truth and reconstruction have different size") << STACK_TRACE);

	if (_groundTruth->height() != _reconstruction->height() || _groundTruth->width() != _reconstruction->width())
		BOOST_THROW_EXCEPTION(SizeMismatchError() << error_message("ground truth and reconstruction have different size") << STACK_TRACE);

	_depth  = _groundTruth->size();
	_width  = _groundTruth->width();
	_height = _groundTruth->height();

	LOG_ALL(tedlog) << "extracting cells in " << _width << "x" << _height << "x" << _depth << " volume" << std::endl;

	vigra::MultiArray<3, std::pair<size_t, size_t> > gtAndRec(vigra::Shape3(_width, _height, _depth));
	vigra::MultiArray<3, unsigned int>             cellIds(vigra::Shape3(_width, _height, _depth));

	// prepare gt and rec image

	for (unsigned int z = 0; z < _depth; z++) {

		boost::shared_ptr<Image> gt  = (*_groundTruth)[z];
		boost::shared_ptr<Image> rec = (*_reconstruction)[z];

		for (unsigned int x = 0; x < _width; x++)
			for (unsigned int y = 0; y < _height; y++) {

				size_t gtLabel  = (*gt)(x, y);
				size_t recLabel = (*rec)(x, y);

				gtAndRec(x, y, z) = std::make_pair(gtLabel, recLabel);
			}
	}

	// find connected components in gt and rec image
	cellIds = 0;
	_numCells = vigra::labelMultiArray(gtAndRec, cellIds);

	LOG_DEBUG(tedlog) << "found " << _numCells << " cells" << std::endl;

	_toleranceFunction->setResolution(
			_reconstruction->getResolutionX(),
			_reconstruction->getResolutionY(),
			_reconstruction->getResolutionZ());

	// let tolerance function extract cells from that
	_toleranceFunction->extractCells(
			_numCells,
			cellIds,
			*_reconstruction,
			*_groundTruth);

	LOG_ALL(tedlog)
			<< "found "
			<< _toleranceFunction->getGroundTruthLabels().size()
			<< " ground truth labels and "
			<< _toleranceFunction->getReconstructionLabels().size()
			<< " reconstruction labels"
			<< std::endl;
}

void
TolerantEditDistance::findBestCellLabels() {

	//boost::timer::auto_cpu_timer timer(std::cout, "\tfindBestCellLabels():\t\t\t%ws\n");

	pipeline::Value<LinearConstraints>      constraints;
	pipeline::Value<LinearSolverParameters> parameters;

	// the default are binary variables
	parameters->setVariableType(Binary);

	// introduce indicators for each cell and each possible label of that cell
	unsigned int var = 0;
	for (unsigned int cellIndex = 0; cellIndex < _toleranceFunction->getCells()->size(); cellIndex++) {

		cell_t& cell = (*_toleranceFunction->getCells())[cellIndex];

		// first indicator variable for this cell
		unsigned int begin = var;

		// one variable for the default label
		assignIndicatorVariable(var++, cellIndex, cell.getGroundTruthLabel(), cell.getReconstructionLabel());

		// one variable for each alternative
		foreach (size_t l, cell.getAlternativeLabels()) {

			unsigned int ind = var++;
			_alternativeIndicators.push_back(std::make_pair(ind, cell.size()));
			assignIndicatorVariable(ind, cellIndex, cell.getGroundTruthLabel(), l);
		}

		// last +1 indicator variable for this cell
		unsigned int end = var;

		// every cell needs to have a label
		LinearConstraint constraint;
		for (unsigned int i = begin; i < end; i++)
			constraint.setCoefficient(i, 1.0);
		constraint.setRelation(Equal);
		constraint.setValue(1);
		constraints->add(constraint);

		LOG_ALL(tedlog) << constraint << std::endl;
	}
	_numIndicatorVars = var;

	LOG_ALL(tedlog) << "adding constraints to ensure that rec labels don't disappear" << std::endl;

	// labels can not disappear
	foreach (size_t recLabel, _toleranceFunction->getReconstructionLabels()) {

		LinearConstraint constraint;
		foreach (unsigned int v, getIndicatorsByRec(recLabel))
			constraint.setCoefficient(v, 1.0);
		constraint.setRelation(GreaterEqual);
		constraint.setValue(1);
		constraints->add(constraint);

		LOG_ALL(tedlog) << constraint << std::endl;
	}

	// introduce indicators for each match of ground truth label to 
	// reconstruction label
	foreach (size_t gtLabel, _toleranceFunction->getGroundTruthLabels())
		foreach (size_t recLabel, _toleranceFunction->getPossibleMatchesByGt(gtLabel))
			assignMatchVariable(var++, gtLabel, recLabel);

	// cell label selection activates match
	foreach (size_t gtLabel, _toleranceFunction->getGroundTruthLabels()) {
		foreach (size_t recLabel, _toleranceFunction->getPossibleMatchesByGt(gtLabel)) {

			unsigned int matchVar = getMatchVariable(gtLabel, recLabel);

			// no assignment of gtLabel to recLabel -> match is zero
			LinearConstraint noMatchConstraint;

			foreach (unsigned int v, getIndicatorsGtToRec(gtLabel, recLabel)) {

				noMatchConstraint.setCoefficient(v, 1);

				// at least one assignment of gtLabel to recLabel -> match is 
				// one
				LinearConstraint matchConstraint;
				matchConstraint.setCoefficient(matchVar, 1);
				matchConstraint.setCoefficient(v, -1);
				matchConstraint.setRelation(GreaterEqual);
				matchConstraint.setValue(0);
				constraints->add(matchConstraint);

				LOG_ALL(tedlog) << matchConstraint << std::endl;
			}

			noMatchConstraint.setCoefficient(matchVar, -1);
			noMatchConstraint.setRelation(GreaterEqual);
			noMatchConstraint.setValue(0);
			constraints->add(noMatchConstraint);

			LOG_ALL(tedlog) << noMatchConstraint << std::endl;
		}
	}

	// introduce split number for each ground truth label

	unsigned int splitBegin = var;

	foreach (size_t gtLabel, _toleranceFunction->getGroundTruthLabels()) {

		unsigned int splitVar = var++;

		LOG_ALL(tedlog) << "adding split var " << splitVar << " for gt label " << gtLabel << std::endl;

		parameters->setVariableType(splitVar, Integer);

		LinearConstraint positive;
		positive.setCoefficient(splitVar, 1);
		positive.setRelation(GreaterEqual);
		positive.setValue(0);
		constraints->add(positive);

		LinearConstraint numSplits;
		numSplits.setCoefficient(splitVar, 1);
		foreach (size_t recLabel, _toleranceFunction->getPossibleMatchesByGt(gtLabel))
			numSplits.setCoefficient(getMatchVariable(gtLabel, recLabel), -1);
		numSplits.setRelation(Equal);
		numSplits.setValue(-1);
		constraints->add(numSplits);

		LOG_ALL(tedlog) << positive << std::endl;
		LOG_ALL(tedlog) << numSplits << std::endl;
	}

	unsigned int splitEnd = var;

	// introduce total split number

	_splits = var++;
	parameters->setVariableType(_splits, Integer);

	LOG_ALL(tedlog) << "adding total split var " << _splits << std::endl;

	LinearConstraint sumOfSplits;
	sumOfSplits.setCoefficient(_splits, 1);
	for (unsigned int i = splitBegin; i < splitEnd; i++)
		sumOfSplits.setCoefficient(i, -1);
	sumOfSplits.setRelation(Equal);
	sumOfSplits.setValue(0);
	constraints->add(sumOfSplits);

	LOG_ALL(tedlog) << sumOfSplits << std::endl;

	// introduce merge number for each reconstruction label

	unsigned int mergeBegin = var;

	foreach (size_t recLabel, _toleranceFunction->getReconstructionLabels()) {

		unsigned int mergeVar = var++;

		LOG_ALL(tedlog) << "adding merge var " << mergeVar << " for rec label " << recLabel << std::endl;

		parameters->setVariableType(mergeVar, Integer);

		LinearConstraint positive;
		positive.setCoefficient(mergeVar, 1);
		positive.setRelation(GreaterEqual);
		positive.setValue(0);
		constraints->add(positive);

		LinearConstraint numMerges;
		numMerges.setCoefficient(mergeVar, 1);
		foreach (size_t gtLabel, _toleranceFunction->getPossibleMathesByRec(recLabel))
			numMerges.setCoefficient(getMatchVariable(gtLabel, recLabel), -1);
		numMerges.setRelation(Equal);
		numMerges.setValue(-1);
		constraints->add(numMerges);

		LOG_ALL(tedlog) << positive << std::endl;
		LOG_ALL(tedlog) << numMerges << std::endl;
	}

	unsigned int mergeEnd = var;

	// introduce total merge number

	_merges = var++;
	parameters->setVariableType(_merges, Integer);

	LOG_ALL(tedlog) << "adding total merge var " << _splits << std::endl;

	LinearConstraint sumOfMerges;
	sumOfMerges.setCoefficient(_merges, 1);
	for (unsigned int i = mergeBegin; i < mergeEnd; i++)
		sumOfMerges.setCoefficient(i, -1);
	sumOfMerges.setRelation(Equal);
	sumOfMerges.setValue(0);
	constraints->add(sumOfMerges);

	LOG_ALL(tedlog) << sumOfMerges << std::endl;

	// create objective

	pipeline::Value<LinearObjective> objective(var);

	// we want to minimize the number of split and merges
	objective->setCoefficient(_splits, 1);
	objective->setCoefficient(_merges, 1);
	// however, if there are multiple equal solutions, we prefer the ones with 
	// the least changes -- therefore, we add a small value for each of those 
	// variables that can not sum up to one and therefor does not change the 
	// number of splits and merges
	unsigned int ind;
	size_t cellSize;
	double volumeSize = _width*_height*_depth;
	foreach (boost::tie(ind, cellSize), _alternativeIndicators)
		objective->setCoefficient(ind, static_cast<double>(cellSize)/(volumeSize + 1));
	objective->setSense(Minimize);

	// solve

	pipeline::Process<LinearSolver> solver;

	solver->setInput("objective", objective);
	solver->setInput("linear constraints", constraints);
	solver->setInput("parameters", parameters);

	_solution = solver->getOutput("solution");
}

void
TolerantEditDistance::findErrors() {

	//boost::timer::auto_cpu_timer timer(std::cout, "\tfindErrors():\t\t\t\t%ws\n");

	// prepare error location image stack

	for (unsigned int i = 0; i < _depth; i++) {

		// initialize with gray (no cell label)
		_splitLocations->add(boost::make_shared<Image>(_width, _height, 0.33));
		_mergeLocations->add(boost::make_shared<Image>(_width, _height, 0.33));
		_fpLocations->add(boost::make_shared<Image>(_width, _height, 0.33));
		_fnLocations->add(boost::make_shared<Image>(_width, _height, 0.33));
	}

	// prepare error data structure

	_errors->setCells(_toleranceFunction->getCells());

	// fill error data structure

	for (unsigned int i = 0; i < _numIndicatorVars; i++) {

		if ((*_solution)[i]) {

			unsigned int cellIndex = _labelingByVar[i].first;
			size_t        recLabel  = _labelingByVar[i].second;

			_errors->addMapping(cellIndex, recLabel);
		}
	}

	//LOG_USER(tedlog) << "error counts from Errors data structure:" << std::endl;
	//LOG_USER(tedlog) << "num splits: " << _errors->getNumSplits() << std::endl;
	//LOG_USER(tedlog) << "num merges: " << _errors->getNumMerges() << std::endl;
	//LOG_USER(tedlog) << "num false positives: " << _errors->getNumFalsePositives() << std::endl;
	//LOG_USER(tedlog) << "num false negatives: " << _errors->getNumFalseNegatives() << std::endl;

	// fill error location image stack

	// all cells that changed label within tolerance

	// all cells that split the ground truth
	size_t gtLabel;
	typedef TolerantEditDistanceErrors::cell_map_t::mapped_type::value_type mapping_t;
	foreach (gtLabel, _errors->getSplitLabels())
		foreach (const mapping_t& cells, _errors->getSplitCells(gtLabel))
			foreach (unsigned int cellIndex, cells.second)
				foreach (const cell_t::Location& l, (*_toleranceFunction->getCells())[cellIndex])
					(*(*_splitLocations)[l.z])(l.x, l.y) = cells.first;

	// all cells that split the reconstruction
	size_t recLabel;
	foreach (recLabel, _errors->getMergeLabels())
		foreach (const mapping_t& cells, _errors->getMergeCells(recLabel))
			foreach (unsigned int cellIndex, cells.second)
				foreach (const cell_t::Location& l, (*_toleranceFunction->getCells())[cellIndex])
					(*(*_mergeLocations)[l.z])(l.x, l.y) = cells.first;

	if (_haveBackgroundLabel) {

		// all cells that are false positives
		foreach (const mapping_t& cells, _errors->getFalsePositiveCells())
			if (cells.first != _recBackgroundLabel) {
				foreach (unsigned int cellIndex, cells.second)
					foreach (const cell_t::Location& l, (*_toleranceFunction->getCells())[cellIndex])
						(*(*_fpLocations)[l.z])(l.x, l.y) = cells.first;
			}

		// all cells that are false negatives
		foreach (const mapping_t& cells, _errors->getFalseNegativeCells())
			if (cells.first != _gtBackgroundLabel) {
				foreach (unsigned int cellIndex, cells.second)
					foreach (const cell_t::Location& l, (*_toleranceFunction->getCells())[cellIndex])
						(*(*_fnLocations)[l.z])(l.x, l.y) = cells.first;
			}
	}

	_errors->setInferenceTime(_solution->getTime());
	_errors->setNumVariables(_solution->size());
}

void
TolerantEditDistance::correctReconstruction() {

	//boost::timer::auto_cpu_timer timer(std::cout, "\tcorrectReconstruction():\t\t%ws\n");

	// prepare output image

	for (unsigned int i = 0; i < _depth; i++) {

		_correctedReconstruction->add(boost::make_shared<Image>(_width, _height, 0.0));
	}

	// read solution

	for (unsigned int i = 0; i < _numIndicatorVars; i++) {

		if ((*_solution)[i]) {

			unsigned int cellIndex = _labelingByVar[i].first;
			size_t        recLabel  = _labelingByVar[i].second;
			cell_t&      cell      = (*_toleranceFunction->getCells())[cellIndex];

			foreach (const cell_t::Location& l, cell)
				(*(*_correctedReconstruction)[l.z])(l.x, l.y) = recLabel;
		}
	}
}

void
TolerantEditDistance::assignIndicatorVariable(unsigned int var, unsigned int cellIndex, size_t gtLabel, size_t recLabel) {

	LOG_ALL(tedlog) << "adding indicator var " << var << " to assign label " << recLabel << " to cell " << cellIndex << std::endl;

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

	LOG_ALL(tedlog) << "adding indicator var " << var << " to match gt label " << gtLabel << " to rec label " << recLabel << std::endl;

	_matchVars[gtLabel][recLabel] = var;
}

unsigned int
TolerantEditDistance::getMatchVariable(size_t gtLabel, size_t recLabel) {

	return _matchVars[gtLabel][recLabel];
}

#ifndef TED_EVALUATION_TOLERANT_EDIT_DISTANCE_H__
#define TED_EVALUATION_TOLERANT_EDIT_DISTANCE_H__

#include <imageprocessing/ImageStack.h>
#include <pipeline/SimpleProcessNode.h>
#include <pipeline/Value.h>
#include <inference/Solution.h>
#include "LocalToleranceFunction.h"
#include "TolerantEditDistanceErrors.h"
#include "Cell.h"

class TolerantEditDistance : public pipeline::SimpleProcessNode<> {

public:

	/**
	 * Create a new evaluator.
	 *
	 * @param headerOnly
	 *              If set to true, no error will be computed, only the header 
	 *              information in Errors::errorHeader() will be set.
	 */
	TolerantEditDistance(bool headerOnly);

	~TolerantEditDistance();

private:

	typedef LocalToleranceFunction::cell_t cell_t;

	void updateOutputs();

	void clear();

	void extractCells();

	void findBestCellLabels();

	void findErrors();

	void correctReconstruction();

	void assignIndicatorVariable(unsigned int var, unsigned int cellIndex, size_t gtLabel, size_t recLabel);

	std::vector<unsigned int>& getIndicatorsByRec(size_t recLabel);

	std::vector<unsigned int>& getIndicatorsGtToRec(size_t gtLabel, size_t recLabel);

	void assignMatchVariable(unsigned int var, size_t gtLabel, size_t recLabel);

	unsigned int getMatchVariable(size_t gtLabel, size_t recLabel);

	// is there a background label?
	bool _haveBackgroundLabel;

	// the optional background labels of the ground truth and reconstruction
	size_t _gtBackgroundLabel;
	size_t _recBackgroundLabel;

	pipeline::Input<ImageStack> _groundTruth;
	pipeline::Input<ImageStack> _reconstruction;

	pipeline::Output<ImageStack> _correctedReconstruction;
	pipeline::Output<ImageStack> _splitLocations;
	pipeline::Output<ImageStack> _mergeLocations;
	pipeline::Output<ImageStack> _fpLocations;
	pipeline::Output<ImageStack> _fnLocations;
	pipeline::Output<TolerantEditDistanceErrors> _errors;

	// the local tolerance function to use
	LocalToleranceFunction* _toleranceFunction;

	// the extends of the ground truth and reconstruction
	unsigned int _width, _height, _depth;

	// the number of cells
	unsigned int _numCells;

	// reconstruction label indicators by reconstruction label
	std::map<size_t, std::vector<unsigned int> > _indicatorVarsByRecLabel;

	// reconstruction label indicators by groundtruth label x reconstruction 
	// label
	std::map<size_t, std::map<size_t, std::vector<unsigned int> > > _indicatorVarsByGtToRecLabel;

	// (cell index, new label) by indicator variable
	std::map<unsigned int, std::pair<unsigned int, size_t> > _labelingByVar;

	// map from ground truth label x reconstruction label to match variable
	std::map<size_t, std::map<size_t, unsigned int> > _matchVars;

	// the number of indicator variables in the ILP
	unsigned int _numIndicatorVars;

	// indicators for alternative cell labels, and the corresponding cell size
	std::vector<std::pair<unsigned int, size_t> > _alternativeIndicators;

	// the ILP variables for the number of splits and merges
	unsigned int _splits;
	unsigned int _merges;

	// the solution of the ILP
	pipeline::Value<Solution> _solution;

	bool _headerOnly;
};

#endif // TED_EVALUATION_TOLERANT_EDIT_DISTANCE_H__


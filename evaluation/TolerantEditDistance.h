#ifndef TED_EVALUATION_TOLERANT_EDIT_DISTANCE_H__
#define TED_EVALUATION_TOLERANT_EDIT_DISTANCE_H__

#include <imageprocessing/ImageStack.h>
#include <inference/Solution.h>
#include "LocalToleranceFunction.h"
#include "TolerantEditDistanceErrors.h"
#include "Cell.h"

class TolerantEditDistance {

public:

	TolerantEditDistance(
			bool fromSkeleton = false,
			unsigned int distanceThreshold = 10,
			float gtBackgroundLabel = 0.0,
			bool haveBackground = true,
			float recBackgroundLabel = 0.0);

	~TolerantEditDistance();

	TolerantEditDistanceErrors compute(const ImageStack& groundTruth, const ImageStack& reconstruction);

	const ImageStack& getCorrectedReconstruction() { return _correctedReconstruction; }

private:

	typedef LocalToleranceFunction::cell_t cell_t;

	void clear();

	void extractCells(const ImageStack& groundTruth, const ImageStack& reconstruction);

	void findBestCellLabels();

	TolerantEditDistanceErrors findErrors();

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

	ImageStack _correctedReconstruction;
	ImageStack _splitLocations;
	ImageStack _mergeLocations;
	ImageStack _fpLocations;
	ImageStack _fnLocations;

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
	Solution _solution;
};

#endif // TED_EVALUATION_TOLERANT_EDIT_DISTANCE_H__


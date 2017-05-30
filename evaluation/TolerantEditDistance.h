#ifndef TED_EVALUATION_TOLERANT_EDIT_DISTANCE_H__
#define TED_EVALUATION_TOLERANT_EDIT_DISTANCE_H__

#include <imageprocessing/ImageStack.h>
#include <inference/Solution.h>
#include "LocalToleranceFunction.h"
#include "TolerantEditDistanceErrors.h"

class TolerantEditDistance {

public:

	struct Parameters {

		Parameters() :
			fromSkeleton(false),
			distanceThreshold(10),
			reportFPsFNs(false),
			allowBackgroundAppearance(false),
			gtBackgroundLabel(0),
			recBackgroundLabel(0) {}

		/**
		* True if the ground-truth consists of skeletons. In this case, the 
		* ground-truth background label (default 0) will be ignored.
		*/
		bool fromSkeleton;

		/**
		 * By how much boundaries in the reconstruction are allowed to be shifted.
		 */
		unsigned int distanceThreshold;

		/**
		 * Whether background labels should be treated differently: If true, 
		 * splits and merges involving background labels are counted as false 
		 * positives and false negatives, respectively. Defaults to false.
		 */
		bool reportFPsFNs;

		/**
		* If true, background can be created by shifting a boundary in opposite 
		* directions, thus effectively letting new background parts appear.
		*/
		bool allowBackgroundAppearance;

		/**
		 * The background labels in the respective image stacks. Defaults to 0.
		 */
		float gtBackgroundLabel;
		float recBackgroundLabel;
	};

	TolerantEditDistance(const Parameters& parameters = Parameters());

	/**
	 * Compute errors for the given ground-truth and reconstruction.
	 */
	TolerantEditDistanceErrors compute(const ImageStack& groundTruth, const ImageStack& reconstruction);

	/**
	 * After a call to compute(), get a corrected version of the reconstruction, 
	 * which was chosen to be as close as possible to the ground-truth.
	 */
	const ImageStack& getCorrectedReconstruction() { return _correctedReconstruction; }

private:

	void reset(const ImageStack& groundTruth, const ImageStack& reconstruction);

	void minimizeErrors(const Cells& cells);

	void correctReconstruction(const Cells& cells);

	TolerantEditDistanceErrors findErrors(std::shared_ptr<Cells> cells);

	void assignIndicatorVariable(unsigned int var, unsigned int cellIndex, size_t gtLabel, size_t recLabel);

	std::vector<unsigned int>& getIndicatorsByRec(size_t recLabel);

	std::vector<unsigned int>& getIndicatorsGtToRec(size_t gtLabel, size_t recLabel);

	void assignMatchVariable(unsigned int var, size_t gtLabel, size_t recLabel);

	unsigned int getMatchVariable(size_t gtLabel, size_t recLabel);

	Parameters _parameters;

	ImageStack _correctedReconstruction;
	ImageStack _splitLocations;
	ImageStack _mergeLocations;
	ImageStack _fpLocations;
	ImageStack _fnLocations;

	// the local tolerance function to use
	std::unique_ptr<LocalToleranceFunction> _toleranceFunction;

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


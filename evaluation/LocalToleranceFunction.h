#ifndef TED_EVALUATION_LOCAL_TOLERANCE_FUNCTION_H__
#define TED_EVALUATION_LOCAL_TOLERANCE_FUNCTION_H__

#include <vector>
#include <set>
#include <map>

#include <boost/shared_ptr.hpp>

#include <imageprocessing/ImageStack.h>
#include "Cell.h"

#include <vigra/multi_array.hxx>

/**
 * Superclass of local tolerance functions, i.e., functions, that assign relabel
 * alternatives to each cell independently.
 */
class LocalToleranceFunction {

public:

	typedef Cell<float>                             cell_t;
	typedef boost::shared_ptr<std::vector<cell_t> > cells_t;

	LocalToleranceFunction() :
		_resolutionX(1.0),
		_resolutionY(1.0),
		_resolutionZ(1.0) {}

	virtual ~LocalToleranceFunction() {}

	/**
	 * Set the number of units for each edge of a voxel.
	 */
	void setResolution(float resX, float resY, float resZ) {

		_resolutionX = resX;
		_resolutionY = resY;
		_resolutionZ = resZ;
	}

	/**
	 * Clear all extracted cells and supplemental data structures.
	 */
	void clear();

	/**
	 * Extract cells from the given cell label image and find all alternative 
	 * labels for them.
	 * 
	 * @param numCells
	 *             The number of cells in the cell label image.
	 * @param cellLabels
	 *             A volume with a cell id at each location.
	 * @param recLabels
	 *             A corresponding image stack with the original reconstruction 
	 *             labels at each location.
	 * @param gtLabels
	 *             A corresponding image stack with the ground-truth labels at 
	 *             each location.
	 */
	virtual void extractCells(
			unsigned int numCells,
			const vigra::MultiArray<3, unsigned int>& cellLabels,
			const ImageStack& recLabels,
			const ImageStack& gtLabels) = 0;

	/**
	 * Get all the cells that have been extracted.
	 */
	cells_t getCells() { return _cells; }

	/**
	 * Get all the ground truth labels.
	 */
	std::set<float>& getGroundTruthLabels();

	/**
	 * Get all the reconstruction labels.
	 */
	std::set<float>& getReconstructionLabels();

	/**
	 * Get all reconstruction labels that might be assigned to a given 
	 * ground-truth label.
	 */
	std::set<float>& getPossibleMatchesByGt(float gtLabel);

	/**
	 * Get all ground-truth labels that might be assigned to a given 
	 * reconstruction label.
	 */
	std::set<float>& getPossibleMathesByRec(float recLabel);

protected:

	void registerPossibleMatch(float gtLabel, float recLabel);

	// all extracted cells
	cells_t _cells;

	// the size of one voxel
	float _resolutionX;
	float _resolutionY;
	float _resolutionZ;

private:

	// set of all ground truth labels
	std::set<float> _groundTruthLabels;

	// set of all reconstruction labels
	std::set<float> _reconstructionLabels;

	// all possible label matchings, from ground truth to reconstruction
	std::map<float, std::set<float> > _possibleGroundTruthMatches;

	// all possible label matchings, from reconstruction to ground truth
	std::map<float, std::set<float> > _possibleReconstructionMatches;

	// map from rec labels to maps from gt label to cell indices
	std::map<float, std::map<float, std::vector<unsigned int> > > _cellsByRecToGtLabel;
};

#endif // TED_EVALUATION_LOCAL_TOLERANCE_FUNCTION_H__


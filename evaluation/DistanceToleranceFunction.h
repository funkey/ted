#ifndef TED_EVALUATION_DISTANCE_TOLERANCE_FUNCTION_H__
#define TED_EVALUATION_DISTANCE_TOLERANCE_FUNCTION_H__

#include "LocalToleranceFunction.h"

class DistanceToleranceFunction : public LocalToleranceFunction {

public:

	/**
	 * @param distanceThreshold
	 *              By how much boundaries in the reconstruction are allowed to 
	 *              be shifted.
	 * @param allowBackgroundAppearance
	 *              If true, background can be created by shifting a boundary in 
	 *              opposite directions, thus effectively letting new background 
	 *              parts appear.
	 * @param recBackgroundLabel
	 *              The background label.
	 */
	DistanceToleranceFunction(
			float distanceThreshold,
			bool allowBackgroundAppearance,
			size_t recBackgroundLabel = 0);

	virtual void findPossibleCellLabels(
			std::shared_ptr<Cells> cells,
			const ImageStack& recLabels,
			const ImageStack& gtLabels) override;

protected:

	/**
	 * Initialize cells, before an expensive search for possible labels. This is 
	 * the last chance to change a cell's reconstruction label.
	 *
	 * Can be refined in subclasses.
	 */
	virtual void initializeCellLabels(std::shared_ptr<Cells> cells);

	/**
	 * Find cells that can potentially be relabelled. This is mainly to exlude 
	 * cells from consideration to speed things up.
	 *
	 * Can be refined in subclasses.
	 */
	virtual std::vector<size_t> findRelabelCandidates(
			std::shared_ptr<Cells> cells,
			const ImageStack& recLabels,
			const ImageStack& gtLabels);

	bool _allowBackgroundAppearance;
	size_t _recBackgroundLabel;

private:

	// create a b/w image of reconstruction label changes
	void createBoundaryMap(const ImageStack& recLabels);

	// find all offset locations for the given distance threshold
	std::vector<Cell<size_t>::Location> createNeighborhood();

	// search for all relabeling alternatives for the given cell and 
	// neighborhood
	std::set<size_t> getAlternativeLabels(
			const Cell<size_t>& cell,
			const std::vector<Cell<size_t>::Location>& neighborhood,
			const ImageStack& recLabels);

	// test, whether a voxel is surrounded by at least one other voxel with a 
	// different label
	bool isBoundaryVoxel(int x, int y, int z, const ImageStack& stack);

	// the distance threshold in nm
	float _maxDistanceThreshold;

	// the distance threshold in pixels for each direction
	int _maxDistanceThresholdX;
	int _maxDistanceThresholdY;
	int _maxDistanceThresholdZ;

	// the extends of the ground truth and reconstruction
	unsigned int _width, _height, _depth;
	float _resolutionX, _resolutionY, _resolutionZ;

	vigra::MultiArray<3, bool> _boundaryMap;
};

#endif // TED_EVALUATION_DISTANCE_TOLERANCE_FUNCTION_H__


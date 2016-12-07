#ifndef TED_EVALUATION_DISTANCE_TOLERANCE_FUNCTION_H__
#define TED_EVALUATION_DISTANCE_TOLERANCE_FUNCTION_H__

#include "LocalToleranceFunction.h"

class DistanceToleranceFunction : public LocalToleranceFunction {

public:

	DistanceToleranceFunction(
			float distanceThreshold,
			bool haveBackgroundLabel,
			size_t backgroundLabel = 0);

	void extractCells(
			unsigned int numCells,
			const vigra::MultiArray<3, unsigned int>& cellLabels,
			const ImageStack& recLabels,
			const ImageStack& gtLabels);

protected:

	virtual void findRelabelCandidates(const std::vector<float>& maxBoundaryDistances);

	std::vector<unsigned int> _relabelCandidates;

	bool _haveBackgroundLabel;
	size_t _backgroundLabel;

private:

	// find alternative cell labels
	void enumerateCellLabels(const ImageStack& recLabels);

	// create a b/w image of reconstruction label changes
	void createBoundaryMap(const ImageStack& recLabels);

	// create a distance2 image of boundary distances
	void createBoundaryDistanceMap();

	// find all offset locations for the given distance threshold
	std::vector<cell_t::Location> createNeighborhood();

	// search for all relabeling alternatives for the given cell and 
	// neighborhood
	std::set<size_t> getAlternativeLabels(
			const cell_t& cell,
			const std::vector<cell_t::Location>& neighborhood,
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

	vigra::MultiArray<3, bool>  _boundaryMap;
	vigra::MultiArray<3, float> _boundaryDistance2;
};

#endif // TED_EVALUATION_DISTANCE_TOLERANCE_FUNCTION_H__


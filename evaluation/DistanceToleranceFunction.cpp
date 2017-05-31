#include "DistanceToleranceFunction.h"
#include <util/Logger.h>
#include <vigra/multi_distance.hxx>
//#include <vigra/multi_impex.hxx>

logger::LogChannel distancetolerancelog("distancetolerancelog", "[DistanceToleranceFunction] ");

DistanceToleranceFunction::DistanceToleranceFunction(
		float distanceThreshold,
		bool allowBackgroundAppearance,
		size_t recBackgroundLabel) :
	_allowBackgroundAppearance(allowBackgroundAppearance),
	_recBackgroundLabel(recBackgroundLabel),
	_maxDistanceThreshold(distanceThreshold) {}

void
DistanceToleranceFunction::findPossibleCellLabels(
		std::shared_ptr<Cells> cells,
		const ImageStack& recLabels,
		const ImageStack& gtLabels) {

	initializeCellLabels(cells);

	_depth  = gtLabels.size();
	_width  = gtLabels.width();
	_height = gtLabels.height();
	_resolutionX = gtLabels.getResolutionX();
	_resolutionY = gtLabels.getResolutionY();
	_resolutionZ = gtLabels.getResolutionZ();

	createBoundaryMap(recLabels);

	// limit analysis to promising relabel candidates
	std::vector<size_t> relabelCandidates = findRelabelCandidates(cells, recLabels, gtLabels);

	_maxDistanceThresholdX = std::min(_width,  (unsigned int)round(_maxDistanceThreshold/_resolutionX));
	_maxDistanceThresholdY = std::min(_height, (unsigned int)round(_maxDistanceThreshold/_resolutionY));
	_maxDistanceThresholdZ = std::min(_depth,  (unsigned int)round(_maxDistanceThreshold/_resolutionZ));

	LOG_DEBUG(distancetolerancelog)
			<< "distance thresholds in pixels (x, y, z) are ("
			<< _maxDistanceThresholdX << ", "
			<< _maxDistanceThresholdY << ", "
			<< _maxDistanceThresholdZ << ")" << std::endl;

	LOG_DEBUG(distancetolerancelog) << "there are " << relabelCandidates.size() << " cells that can be relabeled" << std::endl;

	if (relabelCandidates.size() == 0)
		return;

	LOG_DEBUG(distancetolerancelog) << "creating distance threshold neighborhood" << std::endl;

	// list of all location offsets within threshold distance
	std::vector<Cell<size_t>::Location> neighborhood = createNeighborhood();

	LOG_DEBUG(distancetolerancelog) << "there are " << neighborhood.size() << " pixels in the neighborhood for a threshold of " << _maxDistanceThreshold << std::endl;

	// for each cell
	int i = 0;
	for (unsigned int index : relabelCandidates) {

		i++;
		LOG_DEBUG(distancetolerancelog)
				<< logger::delline << "processing cell "
				<< i << "/" << relabelCandidates.size()
				<< std::flush;

		Cell<size_t>& cell = (*cells)[index];

		LOG_ALL(distancetolerancelog)
				<< "processing cell " << index
				<< " (rec label " << cell.getReconstructionLabel() << ")"
				<< " (gt label " << cell.getGroundTruthLabel() << ")"
				<< std::endl;

		std::set<size_t> alternativeLabels = getAlternativeLabels(cell, neighborhood, recLabels);

		// if there are alternatives, include the background label as well (since a 
		// background label can be created between two foreground labels -- 
		// sufficient condition is that the cell is covered by another cell of 
		// different label, which is the case when there is at least one 
		// alternative)
		if (_allowBackgroundAppearance)
			if (alternativeLabels.size() > 0 && cell.getReconstructionLabel() != _recBackgroundLabel)
				alternativeLabels.insert(_recBackgroundLabel);

		// for each alternative label
		for (size_t recLabel : alternativeLabels)
			cell.addPossibleLabel(recLabel);
	}

	LOG_DEBUG(distancetolerancelog) << std::endl;

	//for (const Cell<size_t>& cell : *_cells) {

		//LOG_ALL(distancetolerancelog) << "cell with GT label " << cell.getGroundTruthLabel() << " can map to: ";
		//LOG_ALL(distancetolerancelog) << cell.getReconstructionLabel() << " [original] ";
		//for (size_t recLabel : cell.getAlternativeLabels())
			//LOG_ALL(distancetolerancelog) << recLabel << " ";
		//LOG_ALL(distancetolerancelog) << std::endl;
	//}
}

void
DistanceToleranceFunction::initializeCellLabels(std::shared_ptr<Cells> cells) {

	// every cell can at least keep it's original label
	for (auto& cell : *cells)
		cell.addPossibleLabel(cell.getReconstructionLabel());
}

std::vector<size_t>
DistanceToleranceFunction::findRelabelCandidates(
		std::shared_ptr<Cells> cells,
		const ImageStack& recLabels,
		const ImageStack& gtLabels) {

	vigra::Shape3 shape(_width, _height, _depth);
	vigra::MultiArray<3, float> boundaryDistance2(shape);

	float pitch[3];
	pitch[0] = _resolutionX;
	pitch[1] = _resolutionY;
	pitch[2] = _resolutionZ;

	// compute l2 distance for each pixel to boundary
	LOG_DEBUG(distancetolerancelog) << "computing boundary distances" << std::endl;
	vigra::separableMultiDistSquared(
			_boundaryMap,
			boundaryDistance2,
			true /* background */,
			pitch);

	size_t numCells = cells->size();

	// the maximum boundary distance of any location for each cell
	std::vector<float> maxBoundaryDistances(numCells, 0);

	for (int cellIndex = 0; cellIndex < numCells; cellIndex++)
		for (const auto& l : (*cells)[cellIndex])
			maxBoundaryDistances[cellIndex] = std::max(
					maxBoundaryDistances[cellIndex],
					boundaryDistance2(l.x, l.y, l.z)
			);

	std::vector<size_t> relabelCandidates;
	for (unsigned int cellIndex = 0; cellIndex < maxBoundaryDistances.size(); cellIndex++)
		if (maxBoundaryDistances[cellIndex] <= _maxDistanceThreshold*_maxDistanceThreshold)
			relabelCandidates.push_back(cellIndex);

	return relabelCandidates;
}

void
DistanceToleranceFunction::createBoundaryMap(const ImageStack& recLabels) {

	vigra::Shape3 shape(_width, _height, _depth);
	_boundaryMap.reshape(shape);

	// create boundary map
	LOG_DEBUG(distancetolerancelog) << "creating boundary map of size " << shape << std::endl;
	_boundaryMap = 0.0f;
	for (unsigned int x = 0; x < _width; x++)
		for (unsigned int y = 0; y < _height; y++)
			for (unsigned int z = 0; z < _depth; z++)
				if (isBoundaryVoxel(x, y, z, recLabels))
					_boundaryMap(x, y, z) = 1.0f;
}

bool
DistanceToleranceFunction::isBoundaryVoxel(int x, int y, int z, const ImageStack& stack) {

	// voxels at the volume borders are always boundary voxels
	if (x == 0 || x == (int)_width - 1)
		return true;
	if (y == 0 || y == (int)_height - 1)
		return true;
	// in z only if there are multiple sections
	if (_depth > 1 && (z == 0 || z == (int)_depth - 1))
		return true;

	size_t center = (*stack[z])(x, y);

	if (x > 0)
		if ((*stack[z])(x - 1, y) != center)
			return true;
	if (x < (int)_width - 1)
		if ((*stack[z])(x + 1, y) != center)
			return true;
	if (y > 0)
		if ((*stack[z])(x, y - 1) != center)
			return true;
	if (y < (int)_height - 1)
		if ((*stack[z])(x, y + 1) != center)
			return true;
	if (z > 0)
		if ((*stack[z - 1])(x, y) != center)
			return true;
	if (z < (int)_depth - 1)
		if ((*stack[z + 1])(x, y) != center)
			return true;

	return false;
}

std::vector<Cell<size_t>::Location>
DistanceToleranceFunction::createNeighborhood() {

	std::vector<Cell<size_t>::Location> thresholdOffsets;

	// quick check first: test on all three axes -- if they contain all covering
	// labels already, we can abort iterating earlier in getAlternativeLabels()

	for (int z = 1; z <= _maxDistanceThresholdZ; z++) {

		thresholdOffsets.push_back(Cell<size_t>::Location(0, 0,  z));
		thresholdOffsets.push_back(Cell<size_t>::Location(0, 0, -z));
	}
	for (int y = 1; y <= _maxDistanceThresholdY; y++) {

		thresholdOffsets.push_back(Cell<size_t>::Location(0,  y, 0));
		thresholdOffsets.push_back(Cell<size_t>::Location(0, -y, 0));
	}
	for (int x = 1; x <= _maxDistanceThresholdX; x++) {

		thresholdOffsets.push_back(Cell<size_t>::Location( x, 0, 0));
		thresholdOffsets.push_back(Cell<size_t>::Location(-x, 0, 0));
	}

	for (int z = -_maxDistanceThresholdZ; z <= _maxDistanceThresholdZ; z++)
		for (int y = -_maxDistanceThresholdY; y <= _maxDistanceThresholdY; y++)
			for (int x = -_maxDistanceThresholdX; x <= _maxDistanceThresholdX; x++) {

				// axis locations have been added already, center is not needed
				if ((x == 0 && y == 0) || (x == 0 && z == 0) || (y == 0 && z == 0))
					continue;

				// is it within threshold distance?
				if (
						x*_resolutionX*x*_resolutionX +
						y*_resolutionY*y*_resolutionY +
						z*_resolutionZ*z*_resolutionZ <= _maxDistanceThreshold*_maxDistanceThreshold)

					thresholdOffsets.push_back(Cell<size_t>::Location(x, y, z));
			}

	return thresholdOffsets;
}

std::set<size_t>
DistanceToleranceFunction::getAlternativeLabels(
		const Cell<size_t>& cell,
		const std::vector<Cell<size_t>::Location>& neighborhood,
		const ImageStack& recLabels) {

	size_t cellLabel = cell.getReconstructionLabel();

	// counts for each neighbor label, how often it was found while iterating 
	// over the cells locations
	std::map<size_t, unsigned int> counts;

	// the number of cell locations visited so far
	unsigned int numVisited = 0;

	// the maximal number of alternative labels, starts with number of labels 
	// found at first location and decreases whenever one label was not found
	unsigned int maxAlternativeLabels = 0;

	// for each location i in that cell
	for (const Cell<size_t>::Location& i : cell) {

		// all the labels in the neighborhood of i
		std::set<size_t> neighborhoodLabels;

		numVisited++;

		// the number of complete neighbor labels that have been seen at the 
		// current location
		unsigned int numComplete = 0;

		// for all locations within the neighborhood, get alternative labels
		for (const Cell<size_t>::Location& n : neighborhood) {

			Cell<size_t>::Location j(i.x + n.x, i.y + n.y, i.z + n.z);

			// are we leaving the image?
			if (j.x < 0 || j.x >= (int)_width || j.y < 0 || j.y >= (int)_height || j.z < 0 || j.z >= (int)_depth)
				continue;

			// is this a boundary?
			if (!_boundaryMap(j.x, j.y, j.z))
				continue;

			// now we have found a boundary pixel within our neighborhood
			size_t label = (*(recLabels)[j.z])(j.x, j.y);

			// count how often we see a neighbor label the first time
			if (label != cellLabel) {

				bool firstTime = neighborhoodLabels.insert(label).second;

				// seen the first time for the current location i
				if (firstTime)
					// is a potential alternative label (covers all visited 
					// locations so far)
					if (++counts[label] == numVisited) {

						numComplete++;

						// if we have seen all the possible complete labels 
						// already, there is no need to search further for the 
						// current location i
						if (numComplete == maxAlternativeLabels)
							break;
					}
			}
		}

		// the number of labels that we have seen for each location visited so 
		// far is the maximal possible number of alternative labels
		maxAlternativeLabels = numComplete;

		// none of the neighbor labels covers the cell
		if (maxAlternativeLabels == 0)
			break;
	}

	std::set<size_t> alternativeLabels;

	// collect all neighbor labels that we have seen for every location of the 
	// cell
	size_t label;
	unsigned int count;
	for (auto& p : counts) {
		label = p.first;
		count = p.second;
		if (count == cell.size())
			alternativeLabels.insert(label);
	}

	return alternativeLabels;
}

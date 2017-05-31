#include <vigra/multi_labeling.hxx>
#include <util/Logger.h>
#include "LocalToleranceFunction.h"

logger::LogChannel localtolerancefunctionlog("localtolerancefunctionlog", "[LocalToleranceFunction] ");

std::shared_ptr<Cells>
LocalToleranceFunction::extractCells(
		const ImageStack& groundTruth,
		const ImageStack& reconstruction) {

	size_t depth  = groundTruth.size();
	size_t width  = groundTruth.width();
	size_t height = groundTruth.height();

	LOG_ALL(localtolerancefunctionlog) << "extracting cells in " << width << "x" << height << "x" << depth << " volume" << std::endl;

	vigra::MultiArray<3, std::pair<size_t, size_t>> gtAndRec(vigra::Shape3(width, height, depth));
	vigra::MultiArray<3, unsigned int>              cellIds(vigra::Shape3(width, height, depth));

	// prepare gt and rec image

	for (unsigned int z = 0; z < depth; z++) {

		std::shared_ptr<const Image> gt  = groundTruth[z];
		std::shared_ptr<const Image> rec = reconstruction[z];

		for (unsigned int x = 0; x < width; x++)
			for (unsigned int y = 0; y < height; y++) {

				size_t gtLabel  = (*gt)(x, y);
				size_t recLabel = (*rec)(x, y);

				gtAndRec(x, y, z) = std::make_pair(gtLabel, recLabel);
			}
	}

	// find connected components in gt and rec image

	cellIds = 0;
	size_t numCells = vigra::labelMultiArray(gtAndRec, cellIds, vigra::IndirectNeighborhood);

	LOG_DEBUG(localtolerancefunctionlog) << "found " << numCells << " cells" << std::endl;

	// extract cells from connected components

	std::shared_ptr<Cells> cells = std::make_shared<Cells>(numCells);

	for (unsigned int z = 0; z < depth; z++) {

		std::shared_ptr<const Image> gt  = groundTruth[z];
		std::shared_ptr<const Image> rec = reconstruction[z];

		for (unsigned int x = 0; x < width; x++)
			for (unsigned int y = 0; y < height; y++) {

				size_t gtLabel  = (*gt)(x, y);
				size_t recLabel = (*rec)(x, y);

				// argh, vigra starts counting at 1!
				unsigned int cellIndex = cellIds(x, y, z) - 1;

				(*cells)[cellIndex].add(Cell<size_t>::Location(x, y, z));
				(*cells)[cellIndex].setReconstructionLabel(recLabel);
				(*cells)[cellIndex].setGroundTruthLabel(gtLabel);
			}
	}

	// delegate label enumeration to subclasses
	findPossibleCellLabels(cells, groundTruth, reconstruction);

	return cells;
}

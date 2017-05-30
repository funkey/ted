#ifndef TED_EVALUATION_LOCAL_TOLERANCE_FUNCTION_H__
#define TED_EVALUATION_LOCAL_TOLERANCE_FUNCTION_H__

#include <vector>
#include <set>
#include <map>
#include <memory>

#include <imageprocessing/ImageStack.h>
#include "Cells.h"

#include <vigra/multi_array.hxx>

/**
 * Superclass of local tolerance functions, i.e., functions, that assign relabel
 * alternatives to each cell independently.
 */
class LocalToleranceFunction {

public:

	virtual ~LocalToleranceFunction() {}

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
	std::shared_ptr<Cells> extractCells(
			const ImageStack& gtLabels,
			const ImageStack& recLabels);

protected:

	/**
	 * Do be overwritten by subclasses.
	 */
	virtual void findPossibleCellLabels(
			std::shared_ptr<Cells> cells,
			const ImageStack& gtLabels,
			const ImageStack& recLabels) = 0;
};

#endif // TED_EVALUATION_LOCAL_TOLERANCE_FUNCTION_H__


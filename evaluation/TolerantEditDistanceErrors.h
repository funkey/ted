#ifndef TED_EVALUATION_TOLERANT_EDIT_DISTANCE_ERRORS_H__
#define TED_EVALUATION_TOLERANT_EDIT_DISTANCE_ERRORS_H__

#include "Cells.h"

/**
 * Representation of split and merge (and optionally false positive and false 
 * negative) errors between a ground truth and a reconstruction. All errors are 
 * evaluated based on a mapping of Cells from a ground truth label to a 
 * reconstruction label. A cell represents a set of image locations and is in 
 * this context the atomic unit to be labelled.
 */
class TolerantEditDistanceErrors {

public:

	typedef std::map<size_t, std::map<size_t, std::set<unsigned int> > > cell_map_t;

	/**
	 * Represents match between a ground-truth label and a reconstruction label, 
	 * after optimization. 'overlap' is the number of all voxels with 'gtLabel' 
	 * and 'recLabel'.
	 */
	struct Match {

		size_t gtLabel;
		size_t recLabel;
		size_t overlap;
	};

	struct SplitError {

		SplitError() :
			gtLabel(0),
			recLabel1(0),
			recLabel2(0),
			distance(0),
			location(0,0,0),
			size(0) {}

		void initFromCells(const Cell<size_t>& a, const Cell<size_t>& b) {

			gtLabel = a.getGroundTruthLabel();
			recLabel1 = a.getReconstructionLabel();
			recLabel2 = b.getReconstructionLabel();
		}

		std::pair<size_t, size_t> getErrorLabels() { return {recLabel1, recLabel2}; }

		// which GT label is split
		size_t gtLabel;

		// the two REC labels that split the GT
		size_t recLabel1;
		size_t recLabel2;

		// the minimal distance between the two REC labels
		double distance;

		// the location of the split, between the closest locations of the two 
		// REC labels
		Cell<size_t>::Location location;

		// the size of the split-off
		size_t size;
	};

	struct MergeError {

		MergeError() :
			recLabel(0),
			gtLabel1(0),
			gtLabel2(0),
			distance(0),
			location(0,0,0),
			size(0) {}

		void initFromCells(const Cell<size_t>& a, const Cell<size_t>& b) {

			recLabel = a.getReconstructionLabel();
			gtLabel1 = a.getGroundTruthLabel();
			gtLabel2 = b.getGroundTruthLabel();
		}

		std::pair<size_t, size_t> getErrorLabels() { return {gtLabel1, gtLabel2}; }

		// which REC label is merging
		size_t recLabel;

		// the two GT labels that are merged
		size_t gtLabel1;
		size_t gtLabel2;

		// the minimal distance between the two GT labels
		double distance;

		// the location of the split, between the closest locations of the two 
		// GT labels
		Cell<size_t>::Location location;

		// the size of the merge
		size_t size;
	};

	/**
	 * Create an empty errors data structure without using a background label, 
	 * i.e., without false positives and false negatives.
	 */
	TolerantEditDistanceErrors();

	/**
	 * Create an empty errors data structure for the given background labels.
	 *
	 * @param gtBackgroundLabel
	 *             The background label in the ground truth.
	 *
	 * @param recBackgroundLabel
	 *             The background label in the reconstruction.
	 */
	TolerantEditDistanceErrors(size_t gtBackgroundLabel, size_t recBackgroundLabel);

	/**
	 * Set the list of cells this errors data structure is working on. This has 
	 * to be done before calling addMapping() or getOverlap().
	 *
	 * @param cells
	 *             A list of cells (sets of image locations) that partitions the 
	 *             ground truth and reconstruction volumes. Each cell has a 
	 *             ground truth label and can be mapped via addMapping() to an 
	 *             arbitrary reconstruction label.
	 */
	void setCells(std::shared_ptr<Cells> cells);

	/**
	 * Clear the label mappings and error counts.
	 */
	void clear();

	/**
	 * Register a mapping from a cell to a reconstruction label.
	 * 
	 * @param cellIndex
	 *             The index of the cell in the cell list.
	 *
	 * @param recLabel
	 *             The reconstruction label of the cell.
	 */
	void addMapping(unsigned int cellIndex, size_t recLabel);

	/**
	 * Get all reconstruction labels that map to the given ground truth label.
	 */
	std::vector<size_t> getReconstructionLabels(size_t gtLabel);

	/**
	 * Get all ground truth labels that map to the given reconstruction label.
	 */
	std::vector<size_t> getGroundTruthLabels(size_t recLabel);

	/**
	 * Get the confusion matrix, i.e., matches between ground truth and 
	 * reconstruction.
	 */
	std::vector<Match> getMatches();

	/**
	 * Get the number of locations shared by the given ground truth and 
	 * reconstruction label.
	 */
	unsigned int getOverlap(size_t gtLabel, size_t recLabel);

	unsigned int getNumSplits();
	unsigned int getNumMerges();
	unsigned int getNumFalsePositives();
	unsigned int getNumFalseNegatives();

	/**
	 * Get the sum of all errors.
	 */
	unsigned int getNumErrors() {

		return getNumSplits() + getNumMerges() + getNumFalsePositives() + getNumFalseNegatives();
	}

	/**
	 * Get all ground truth labels that got split in the reconstruction.
	 */
	std::set<size_t> getSplitLabels();

	/**
	 * Get all reconstruction labels that merge multiple ground truh labels.
	 */
	std::set<size_t> getMergeLabels();

	/**
	 * Get all reconstruction labels that split the given ground truth label.
	 */
	std::set<size_t> getSplits(size_t gtLabel);

	/**
	 * Get all ground truth labels that the given reconstruction label merges.
	 */
	std::set<size_t> getMerges(size_t recLabel);

	/**
	 * Get all reconstruction labels that have no corresponding ground truth 
	 * label (i.e., map to the ground truth background).
	 */
	std::set<size_t> getFalsePositives();

	/**
	 * Get all ground truth labels that have no corresponding reconstruction 
	 * label (i.e., map to the reconstruction background).
	 */
	std::set<size_t> getFalseNegatives();

	/**
	 * Get all cells that split the given ground truth label.
	 */
	const cell_map_t::mapped_type& getSplitCells(size_t gtLabel);

	/**
	 * Get all cells that the given reconstruction label merges.
	 */
	const cell_map_t::mapped_type& getMergeCells(size_t recLabel);

	/**
	 * Get all cells that are false positives.
	 */
	const cell_map_t::mapped_type& getFalsePositiveCells();

	/**
	 * Get all cells that are false negatives.
	 */
	const cell_map_t::mapped_type& getFalseNegativeCells();

	/**
	 * Get a vector of all split errors, containing the locations and sizes of 
	 * the errors.
	 *
	 * The locations are between the two closest points of two GT regions that 
	 * are considered split by the reconstruction.
	 *
	 * For the sizes, the largest split region is considered a match, and all 
	 * other split regions contribute with their size. If, for example, GT label 
	 * A was split into B, C, and D, with overlaps of 1, 3, 2, respectively, 
	 * this function will return two split errors C-B and C-D with sizes 1 and 
	 * 2.
	 */
	std::vector<SplitError> getSplitErrors();

	/**
	 * Same as getSplitErrors, but for merges.
	 */
	std::vector<MergeError> getMergeErrors();

	/**
	 * Check whether a background label was considered for the TED errors. If 
	 * yes, some of the split and merge errors have an interpretation as false 
	 * positives and false negatives.
	 */
	bool hasBackgroundLabel() const { return _haveBackgroundLabel; }

	void setInferenceTime(double time) { _inferenceTime = time; }

	double getInferenceTime() const { return _inferenceTime; }

	void setNumVariables(int num) { _numVariables = num; }

	int getNumVariables() const { return _numVariables; }

private:

	void addEntry(cell_map_t& map, size_t a, size_t b, unsigned int v);

	void updateErrorCounts();

	void findSplits(
			const cell_map_t& cellMap,
			cell_map_t&       splits,
			unsigned int&     numSplits,
			unsigned int&     numFalsePositives,
			size_t             backgroundLabel);

	template <typename ErrorType>
	ErrorType computeError(
			const std::set<unsigned int>& cells1,
			const std::set<unsigned int>& cells2);

	// generic function to find split errors, can be used to find merge errors 
	// as well, if _merges and _cellsByRecToGtLabel are fed with MergeError as 
	// template argument
	template <typename ErrorType>
	std::vector<ErrorType> getGenericSplitErrors(
			cell_map_t& splits,
			cell_map_t& cellsByGtToRecLabel);

	// a list of cells partitioning the image
	std::shared_ptr<Cells> _cells;

	// sparse representation of groundtruth to reconstruction confusion matrix
	cell_map_t _cellsByRecToGtLabel;
	cell_map_t _cellsByGtToRecLabel;

	// subset of the confusion matrix without one-to-one mappings
	cell_map_t _splits;
	cell_map_t _merges;

	unsigned int _numSplits;
	unsigned int _numMerges;
	unsigned int _numFalsePositives;
	unsigned int _numFalseNegatives;

	bool _haveBackgroundLabel;

	size_t _gtBackgroundLabel;
	size_t _recBackgroundLabel;

	bool _dirty;

	double _inferenceTime;

	int _numVariables;
};

#endif // TED_EVALUATION_TOLERANT_EDIT_DISTANCE_ERRORS_H__


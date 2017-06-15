#include <queue>
#include <boost/timer/timer.hpp>
#include <boost/range/adaptors.hpp>
#include <util/exceptions.h>
#include <util/Logger.h>
#include "TolerantEditDistanceErrors.h"

logger::LogChannel errorslog("errorslog", "[TolerantEditDistanceErrors] ");

TolerantEditDistanceErrors::TolerantEditDistanceErrors() :
	_haveBackgroundLabel(false),
	_dirty(true) {

	clear();

	LOG_DEBUG(errorslog) << "created errors data structure without background label" << std::endl;
}

TolerantEditDistanceErrors::TolerantEditDistanceErrors(size_t gtBackgroundLabel, size_t recBackgroundLabel) :
	_haveBackgroundLabel(true),
	_gtBackgroundLabel(gtBackgroundLabel),
	_recBackgroundLabel(recBackgroundLabel),
	_dirty(true) {

	clear();

	LOG_DEBUG(errorslog) << "created errors data structure with background label" << std::endl;
}

void
TolerantEditDistanceErrors::setCells(std::shared_ptr<Cells> cells) {

	_cells = cells;
	clear();
}

void
TolerantEditDistanceErrors::clear() {

	_cellsByGtToRecLabel.clear();
	_cellsByRecToGtLabel.clear();

	_dirty = true;
}

void
TolerantEditDistanceErrors::addMapping(unsigned int cellIndex, size_t recLabel) {

	if (!_cells)
		BOOST_THROW_EXCEPTION(UsageError() << error_message("cells need to be set before using addMapping()") << STACK_TRACE);

	size_t gtLabel = (*_cells)[cellIndex].getGroundTruthLabel();

	addEntry(_cellsByRecToGtLabel, recLabel, gtLabel, cellIndex);
	addEntry(_cellsByGtToRecLabel, gtLabel, recLabel, cellIndex);

	_dirty = true;
}

std::vector<size_t>
TolerantEditDistanceErrors::getReconstructionLabels(size_t gtLabel) {

	std::vector<size_t> recLabels;

	for (auto& p : _cellsByGtToRecLabel[gtLabel])
		recLabels.push_back(p.first);

	return recLabels;
}

std::vector<size_t>
TolerantEditDistanceErrors::getGroundTruthLabels(size_t recLabel) {

	std::vector<size_t> gtLabels;

	for (auto& p : _cellsByRecToGtLabel[recLabel])
		gtLabels.push_back(p.first);

	return gtLabels;
}

std::vector<TolerantEditDistanceErrors::Match>
TolerantEditDistanceErrors::getMatches() {

	std::vector<Match> matches;

	for (auto& p_gt : _cellsByGtToRecLabel) {

		size_t gt_label = p_gt.first;

		for (auto& p_rec : p_gt.second) {

			size_t rec_label = p_rec.first;

			Match match;
			match.gtLabel = gt_label;
			match.recLabel = rec_label;
			match.overlap = getOverlap(gt_label, rec_label);
			matches.push_back(match);
		}
	}

	return matches;
}

unsigned int
TolerantEditDistanceErrors::getOverlap(size_t gtLabel, size_t recLabel) {

	if (!_cells)
		BOOST_THROW_EXCEPTION(UsageError() << error_message("cells need to be set before using getOverlap()") << STACK_TRACE);

	if (_cellsByGtToRecLabel.count(gtLabel) == 0 || _cellsByGtToRecLabel[gtLabel].count(recLabel) == 0)
		return 0;

	unsigned int overlap = 0;
	for (unsigned int cellIndex : _cellsByGtToRecLabel[gtLabel][recLabel])
		overlap += (*_cells)[cellIndex].size();

	return overlap;
}

unsigned int
TolerantEditDistanceErrors::getNumSplits() {

	updateErrorCounts();
	return _numSplits;
}

unsigned int
TolerantEditDistanceErrors::getNumMerges() {

	updateErrorCounts();
	return _numMerges;
}

unsigned int
TolerantEditDistanceErrors::getNumFalsePositives() {

	updateErrorCounts();
	return _numFalsePositives;
}

unsigned int
TolerantEditDistanceErrors::getNumFalseNegatives() {

	updateErrorCounts();
	return _numFalseNegatives;
}

std::set<size_t>
TolerantEditDistanceErrors::getMergeLabels() {

	updateErrorCounts();

	std::set<size_t> mergeLabels;
	for (auto& p : _merges)
		if (!_haveBackgroundLabel || p.first != _recBackgroundLabel)
			mergeLabels.insert(p.first);
	return mergeLabels;
}

std::set<size_t>
TolerantEditDistanceErrors::getSplitLabels() {

	updateErrorCounts();

	std::set<size_t> splitLabels;
	for (auto& p : _splits)
		if (!_haveBackgroundLabel || p.first != _gtBackgroundLabel)
			splitLabels.insert(p.first);
	return splitLabels;
}

std::set<size_t>
TolerantEditDistanceErrors::getSplits(size_t gtLabel) {

	std::set<size_t> splitLabels;

	for (const auto& cells : getSplitCells(gtLabel))
		splitLabels.insert(cells.first);

	return splitLabels;
}

std::set<size_t>
TolerantEditDistanceErrors::getMerges(size_t recLabel) {

	std::set<size_t> mergeLabels;

	for (const auto& cells : getMergeCells(recLabel))
		mergeLabels.insert(cells.first);

	return mergeLabels;
}

std::set<size_t>
TolerantEditDistanceErrors::getFalsePositives() {

	if (!_haveBackgroundLabel)
		BOOST_THROW_EXCEPTION(UsageError() << error_message("we don't have a background label -- cannot give false positives"));

	std::set<size_t> splitLabels;

	for (const auto& cells : getSplitCells(_gtBackgroundLabel))
		if (cells.first != _recBackgroundLabel)
			splitLabels.insert(cells.first);

	return splitLabels;
}

std::set<size_t>
TolerantEditDistanceErrors::getFalseNegatives() {

	if (!_haveBackgroundLabel)
		BOOST_THROW_EXCEPTION(UsageError() << error_message("we don't have a background label -- cannot give false negatives"));

	std::set<size_t> mergeLabels;

	for (const auto& cells : getMergeCells(_recBackgroundLabel))
		if (cells.first != _gtBackgroundLabel)
			mergeLabels.insert(cells.first);

	return mergeLabels;
}

const TolerantEditDistanceErrors::cell_map_t::mapped_type&
TolerantEditDistanceErrors::getSplitCells(size_t gtLabel) {

	updateErrorCounts();
	return _splits[gtLabel];
}

const TolerantEditDistanceErrors::cell_map_t::mapped_type&
TolerantEditDistanceErrors::getMergeCells(size_t recLabel) {

	updateErrorCounts();
	return _merges[recLabel];
}

const TolerantEditDistanceErrors::cell_map_t::mapped_type&
TolerantEditDistanceErrors::getFalsePositiveCells() {

	if (!_haveBackgroundLabel)
		BOOST_THROW_EXCEPTION(UsageError() << error_message("we don't have a background label -- cannot give false positives"));

	updateErrorCounts();
	return _splits[_gtBackgroundLabel];
}

const TolerantEditDistanceErrors::cell_map_t::mapped_type&
TolerantEditDistanceErrors::getFalseNegativeCells() {

	if (!_haveBackgroundLabel)
		BOOST_THROW_EXCEPTION(UsageError() << error_message("we don't have a background label -- cannot give false negatives"));

	updateErrorCounts();
	return _merges[_recBackgroundLabel];
}

std::vector<TolerantEditDistanceErrors::SplitError>
TolerantEditDistanceErrors::getSplitErrors() {

	return getGenericSplitErrors<SplitError>(_splits, _cellsByGtToRecLabel);
}

std::vector<TolerantEditDistanceErrors::MergeError>
TolerantEditDistanceErrors::getMergeErrors() {

	return getGenericSplitErrors<MergeError>(_merges, _cellsByRecToGtLabel);
}

template <typename ErrorType>
std::vector<ErrorType>
TolerantEditDistanceErrors::getGenericSplitErrors(
		cell_map_t& splits,
		cell_map_t& cellsByGtToRecLabel) {

	std::vector<ErrorType> mstSplitErrors;

	// for every GT label
	for (const auto& p : splits) {

		size_t gtLabel = p.first;

		// map from REC label x REC label to split error
		std::map<size_t, std::map<size_t, ErrorType>> splitErrors;

		size_t maxOverlapRecLabel = 0;
		size_t maxOverlap = 0;

		// for all pairs of labels that split gtLabel
		for (const auto& q : p.second) {

			size_t recLabel1 = q.first;
			std::set<unsigned int> splitCells1 = cellsByGtToRecLabel[gtLabel][recLabel1];

			// find max overlap REC label
			size_t overlap = 0;
			for (unsigned int cellIndex : splitCells1)
				overlap += (*_cells)[cellIndex].size();
			if (overlap >= maxOverlap) {

				maxOverlap = overlap;
				maxOverlapRecLabel = recLabel1;
			}

			// comput split errors to all other REC labels splitting the current 
			// GT label
			for (const auto& r : p.second) {

				size_t recLabel2 = r.first;

				// assert recLabel1 < recLabel2
				if (recLabel2 <= recLabel1)
					continue;

				std::set<unsigned int> splitCells2 = cellsByGtToRecLabel[gtLabel][recLabel2];

				ErrorType splitError = computeError<ErrorType>(splitCells1, splitCells2);
				splitErrors[recLabel1][recLabel2] = splitError;
				splitErrors[recLabel2][recLabel1] = splitError;
			}
		}

		auto cmp = [](const ErrorType& a, const ErrorType& b) { return a.distance > b.distance; };
		std::priority_queue<ErrorType, std::vector<ErrorType>, decltype(cmp)> mstBoundary(cmp);
		std::set<size_t> inTree;

		// start growing from the maxOverlapRecLabel
		inTree.insert(maxOverlapRecLabel);
		for (const auto& p : splitErrors[maxOverlapRecLabel])
			mstBoundary.push(p.second);

		// grow the MST
		while (mstBoundary.size() > 0) {

			// get the next cheapest edge
			ErrorType splitError = mstBoundary.top();
			mstBoundary.pop();

			// get the label to add
			auto p = splitError.getErrorLabels();
			size_t newLabel = inTree.count(p.first) ? p.second : p.first;

			// if already part of the tree, continue
			if (inTree.count(newLabel))
				continue;

			// get the overlap of this label with the GT label
			size_t overlap = 0;
			for (unsigned int cellIndex : cellsByGtToRecLabel[gtLabel][newLabel])
				overlap += (*_cells)[cellIndex].size();

			splitError.size = overlap;
			mstSplitErrors.push_back(splitError);

			for (const auto& p : splitErrors[newLabel])
				if (!inTree.count(p.first))
					mstBoundary.push(p.second);
		}
	}

	return mstSplitErrors;
}

template <typename ErrorType>
ErrorType
TolerantEditDistanceErrors::computeError(
		const std::set<unsigned int>& cells1,
		const std::set<unsigned int>& cells2) {

	if (cells1.size()*cells2.size() == 0)
		UTIL_THROW_EXCEPTION(SizeMismatchError, "can not find error location for empty set of cells");

	ErrorType error;

	double minDistance = std::numeric_limits<double>::infinity();
	Cell<size_t>::Location closest1(0,0,0);
	Cell<size_t>::Location closest2(0,0,0);

	bool initError = true;

	for (unsigned int i : cells1) {

		const Cell<size_t>& cell1 = (*_cells)[i];

		for (unsigned int j : cells2) {

			const Cell<size_t>& cell2 = (*_cells)[j];

			if (initError) {

				error.initFromCells(cell1, cell2);
				initError = false;
			}

			for (const Cell<size_t>::Location& l1 : cell1) {
				for (const Cell<size_t>::Location& l2 : cell2) {

					double distance2 = pow(l1.x-l2.x,2)*pow(l1.y-l2.y,2)*pow(l1.z-l2.z,2);
					if (distance2 <= minDistance) {

						closest1 = l1;
						closest2 = l2;
						minDistance = distance2;
					}
				}
			}
		}
	}

	error.distance = sqrt(minDistance);
	error.location = Cell<size_t>::Location(
			(closest1.x+closest2.x)*0.5,
			(closest1.y+closest2.y)*0.5,
			(closest1.z+closest2.z)*0.5);

	return error;
}

void
TolerantEditDistanceErrors::updateErrorCounts() {

	if (!_dirty)
		return;

	//boost::timer::auto_cpu_timer timer("\tTolerantEditDistanceErrors::updateErrorCounts():\t\t%w\n");

	_dirty = false;

	_numSplits = 0;
	_numMerges = 0;
	_numFalsePositives = 0;
	_numFalseNegatives = 0;
	_splits.clear();
	_merges.clear();

	findSplits(_cellsByGtToRecLabel, _splits, _numSplits, _numFalsePositives, _gtBackgroundLabel);
	findSplits(_cellsByRecToGtLabel, _merges, _numMerges, _numFalseNegatives, _recBackgroundLabel);
}

void
TolerantEditDistanceErrors::findSplits(
		const cell_map_t& cellMap,
		cell_map_t&       splits,
		unsigned int&     numSplits,
		unsigned int&     numFalsePositives,
		size_t             backgroundLabel) {

	typedef cell_map_t::value_type mapping_t;

	for (const mapping_t& i : cellMap) {

		unsigned int partners = i.second.size();

		// one-to-one mapping is okay
		if (partners == 1)
			continue;

		// remember the split
		splits[i.first] = i.second;

		// increase count
		if (_haveBackgroundLabel && i.first == backgroundLabel)
			numFalsePositives += partners - 1;
		else
			numSplits += partners - 1;
	}
}

void
TolerantEditDistanceErrors::addEntry(cell_map_t& map, size_t a, size_t b, unsigned int cellIndex) {

	map[a][b].insert(cellIndex);
}

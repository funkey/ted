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
TolerantEditDistanceErrors::setCells(cells_t cells) {

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

std::vector<std::pair<size_t,size_t>>
TolerantEditDistanceErrors::getMatches() {

	std::vector<std::pair<size_t,size_t>> matches;

	for (auto& p_gt : _cellsByGtToRecLabel) {

		size_t gt_label = p_gt.first;

		for (auto& p_rec : p_gt.second) {

			size_t rec_label = p_rec.first;
			matches.push_back(std::make_pair(gt_label, rec_label));
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

	typedef cell_map_t::mapped_type::value_type cells_t;
	for (const cells_t& cells : getSplitCells(gtLabel))
		splitLabels.insert(cells.first);

	return splitLabels;
}

std::set<size_t>
TolerantEditDistanceErrors::getMerges(size_t recLabel) {

	std::set<size_t> mergeLabels;

	typedef cell_map_t::mapped_type::value_type cells_t;
	for (const cells_t& cells : getMergeCells(recLabel))
		mergeLabels.insert(cells.first);

	return mergeLabels;
}

std::set<size_t>
TolerantEditDistanceErrors::getFalsePositives() {

	if (!_haveBackgroundLabel)
		BOOST_THROW_EXCEPTION(UsageError() << error_message("we don't have a background label -- cannot give false positives"));

	std::set<size_t> splitLabels;

	typedef cell_map_t::mapped_type::value_type cells_t;
	for (const cells_t& cells : getSplitCells(_gtBackgroundLabel))
		if (cells.first != _recBackgroundLabel)
			splitLabels.insert(cells.first);

	return splitLabels;
}

std::set<size_t>
TolerantEditDistanceErrors::getFalseNegatives() {

	if (!_haveBackgroundLabel)
		BOOST_THROW_EXCEPTION(UsageError() << error_message("we don't have a background label -- cannot give false negatives"));

	std::set<size_t> mergeLabels;

	typedef cell_map_t::mapped_type::value_type cells_t;
	for (const cells_t& cells : getMergeCells(_recBackgroundLabel))
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

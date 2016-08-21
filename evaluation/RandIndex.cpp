#include <util/Logger.h>
#include <util/exceptions.h>
#include <util/ProgramOptions.h>
#include "RandIndex.h"

util::ProgramOption optionRandIgnoreBackground(
		util::_module           = "evaluation",
		util::_long_name        = "randIgnoreBackground",
		util::_description_text = "For the computation of the RAND, do not consider background pixels in the ground truth.");

logger::LogChannel randindexlog("randindexlog", "[ResultEvaluator] ");

RandIndex::RandIndex(bool headerOnly) :
		_ignoreBackground(optionRandIgnoreBackground.as<bool>()),
		_headerOnly(headerOnly) {

	if (!_headerOnly) {

		registerInput(_reconstruction, "reconstruction");
		registerInput(_groundTruth, "ground truth");
	}

	registerOutput(_errors, "errors");
}

void
RandIndex::updateOutputs() {

	if (!_errors)
		_errors = new RandIndexErrors();

	if (_headerOnly)
		return;

	if (_reconstruction->size() != _groundTruth->size())
		BOOST_THROW_EXCEPTION(SizeMismatchError() << error_message("image stacks have different size") << STACK_TRACE);

	unsigned int depth = _reconstruction->size();

	if (depth == 0) {

		// rand index of 1 for empty images
		_errors->setNumPairs(1);
		_errors->setNumAggreeingPairs(1);
		return;
	}

	unsigned int width  = (*_reconstruction)[0]->width();
	unsigned int height = (*_reconstruction)[0]->height();

	size_t numLocations = depth*width*height;

	if (numLocations == 0) {

		// rand index of 1 for empty images
		_errors->setNumPairs(1);
		_errors->setNumAggreeingPairs(1);
		return;
	}

	size_t numGtSamePairs   = 0;
	size_t numRecSamePairs  = 0;
	size_t numBothSamePairs = 0;

	double numAgree = getNumAgreeingPairs(*_reconstruction, *_groundTruth, numLocations, numGtSamePairs, numRecSamePairs, numBothSamePairs);
	double numPairs = (static_cast<double>(numLocations)/2)*(static_cast<double>(numLocations) - 1);

	LOG_DEBUG(randindexlog) << "number of pairs is          " << numPairs << std::endl;;
	LOG_DEBUG(randindexlog) << "number of agreeing pairs is " << numAgree << std::endl;;

	double precision = (double)numBothSamePairs/numRecSamePairs;
	double recall    = (double)numBothSamePairs/numGtSamePairs;
	double fscore    = 2*(precision*recall)/(precision + recall);

	LOG_DEBUG(randindexlog) << "number of TPs is    " << numBothSamePairs << std::endl;
	LOG_DEBUG(randindexlog) << "number of TPs + FNs " << numGtSamePairs << std::endl;
	LOG_DEBUG(randindexlog) << "number of TPs + FPs " << numRecSamePairs << std::endl;
	LOG_DEBUG(randindexlog) << "1 - F-score is      " << (1.0 - fscore) << std::endl;

	_errors->setNumPairs(numPairs);
	_errors->setNumAggreeingPairs(numAgree);
	_errors->setAdaptedRandError(1.0 - fscore);
}

size_t
RandIndex::getNumAgreeingPairs(
		const ImageStack& stack1,
		const ImageStack& stack2,
		size_t& numLocations,
		size_t& numSameComponentPairs1,
		size_t& numSameComponentPairs2,
		size_t& numSameComponentPairs12) {

	// Implementation following algorith by Bjoern Andres:
	//
	// https://github.com/bjoern-andres/partition-comparison/blob/master/include/andres/partition-comparison.hxx

	typedef float                       Label;
	typedef std::pair<Label,    Label>  LabelPair;
	typedef std::map<LabelPair, size_t> ContingencyMatrix;
	typedef std::map<Label,     size_t> SumVector;

	ContingencyMatrix c;
	SumVector         a;
	SumVector         b;

	ImageStack::const_iterator image1 = stack1.begin();
	ImageStack::const_iterator image2 = stack2.begin();

	// for each image in the stacks
	for(; image1 != stack1.end(); image1++, image2++) {

		Image::iterator i1 = (*image1)->begin();
		Image::iterator i2 = (*image2)->begin();

		for (; i1 != (*image1)->end(); i1++, i2++) {

			if (_ignoreBackground && *i2 == 0) {

				numLocations--;
				continue;
			}

			c[std::make_pair(*i1, *i2)]++;
			a[*i1]++;
			b[*i2]++;
		}
	}

	LabelPair labelPair;
	Label     label;
	size_t    n;

	size_t A = 0;
	size_t B = numLocations*numLocations;

	numSameComponentPairs1 = 0;
	numSameComponentPairs2 = 0;

	foreach (boost::tie(labelPair, n), c) {

		A += n*(n-1);
		B += n*n;
		numSameComponentPairs12 += n*n;
	}

	foreach (boost::tie(label, n), a) {

		B -= n*n;
		numSameComponentPairs1 += n*n;
	}

	foreach (boost::tie(label, n), b) {

		B -= n*n;
		numSameComponentPairs2 += n*n;
	}

	return (A+B)/2;
}

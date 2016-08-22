#include <util/Logger.h>
#include <util/exceptions.h>
#include <util/ProgramOptions.h>
#include "RandIndex.h"

logger::LogChannel randindexlog("randindexlog", "[ResultEvaluator] ");

RandIndex::RandIndex(bool headerOnly, bool ignoreBackground) :
		_ignoreBackground(ignoreBackground),
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

	uint64_t numLocations = depth*width*height;

	if (numLocations == 0) {

		// rand index of 1 for empty images
		_errors->setNumPairs(1);
		_errors->setNumAggreeingPairs(1);
		return;
	}

	uint64_t numGtSamePairs   = 0;
	uint64_t numRecSamePairs  = 0;
	uint64_t numBothSamePairs = 0;

	double numAgree = getNumAgreeingPairs(*_reconstruction, *_groundTruth, numLocations, numGtSamePairs, numRecSamePairs, numBothSamePairs);
	double numPairs = (static_cast<double>(numLocations)/2)*(static_cast<double>(numLocations) - 1);

	LOG_DEBUG(randindexlog) << "number of pairs is          " << numPairs << std::endl;;
	LOG_DEBUG(randindexlog) << "number of agreeing pairs is " << numAgree << std::endl;;

	/* The following implements the scores as described in 
	 * http://journal.frontiersin.org/article/10.3389/fnana.2015.00142/full
	 * "Crowdsourcing the creation of image segmentation algorithms for 
	 * connectomics", Argenda-Carreras et. al., 2015
	 *
	 * In it, "rand split" is defined as the probability of two randomly chosen 
	 * pixels having the same label in GT and REC, given they have the same 
	 * label in GT.
	 *
	 *   • Here, we call that "recall". Suppose the "elements" we want to 
	 *     discover are pairs of pixels (x,y) that have the same label. There 
	 *     are numPairs pairs in total. "selected elements" are numRecSamePairs. 
	 *     All "relevant elements" are numGtSamePairs. "true positives" are 
	 *     numBothSamePairs, i.e., all pairs that are relevant and have been 
	 *     found in the reconstruction.
	 *   • p_ij in the paper is equal numBothSamePairs/numPairs.
	 *   • t_k in the paper is equal numGtSamePairs/numPairs.
	 *
	 * "rand merge" is defined analogously, given same labels in REC.
	 *
	 *   • Here, we call that "precision".
	 *   • p_ij in the paper is equal numBothSamePairs/numPairs.
	 *   • s_k in the paper is equal numRecSamePairs/numPairs.
	 *
	 * "f-score" in the paper is the harmonic mean of "rand split" and "rand 
	 * merge".
	 *
	 *   • Here, it is the harmonic mean of "precision" and "recall", i.e., the 
	 *     same.
	 *
	 * To obtain the counts, we ignore every pixel that has label 0 in GT, if 
	 * option ignoreBackground was set.
	 */

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

uint64_t
RandIndex::getNumAgreeingPairs(
		const ImageStack& stack1,
		const ImageStack& stack2,
		uint64_t& numLocations,
		uint64_t& numSameComponentPairs1,
		uint64_t& numSameComponentPairs2,
		uint64_t& numSameComponentPairs12) {

	// Implementation following algorith by Bjoern Andres:
	//
	// https://github.com/bjoern-andres/partition-comparison/blob/master/include/andres/partition-comparison.hxx

	typedef float                       Label;
	typedef std::pair<Label,    Label>  LabelPair;
	typedef std::map<LabelPair, uint64_t> ContingencyMatrix;
	typedef std::map<Label,     uint64_t> SumVector;

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
	uint64_t    n;

	uint64_t A = 0;
	uint64_t B = numLocations*numLocations;

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

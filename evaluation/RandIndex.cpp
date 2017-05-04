#include <util/Logger.h>
#include <util/exceptions.h>
#include "RandIndex.h"

logger::LogChannel randindexlog("randindexlog", "[ResultEvaluator] ");

RandIndex::RandIndex(bool ignoreBackground) :
		_ignoreBackground(ignoreBackground) {}

RandIndexErrors
RandIndex::compute(const ImageStack& groundTruth, const ImageStack& reconstruction) {

	if (reconstruction.size() != groundTruth.size())
		UTIL_THROW_EXCEPTION(SizeMismatchError, "image stacks have different size");

	unsigned int depth = reconstruction.size();

	RandIndexErrors errors;

	if (depth == 0) {

		// rand index of 1 for empty images
		errors.setNumPairs(1);
		errors.setNumAggreeingPairs(1);
		return errors;
	}

	unsigned int width  = reconstruction[0]->width();
	unsigned int height = reconstruction[0]->height();

	uint64_t numLocations = depth*width*height;

	if (numLocations == 0) {

		// rand index of 1 for empty images
		errors.setNumPairs(1);
		errors.setNumAggreeingPairs(1);
		return errors;
	}

	uint64_t numGtSamePairs   = 0;
	uint64_t numRecSamePairs  = 0;
	uint64_t numBothSamePairs = 0;

	double numAgree = getNumAgreeingPairs(reconstruction, groundTruth, numLocations, numGtSamePairs, numRecSamePairs, numBothSamePairs);
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
	 *   • Here, we call that "precision".
	 *   • p_ij in the paper is equal numBothSamePairs/numPairs.
	 *   • t_k in the paper is equal numGtSamePairs/numPairs.
	 *
	 * "rand merge" is defined analogously, given same labels in REC.
	 *
	 *   • Here, we call that "recall".
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
	 *
	 * To compute precision and recall, we suppose the "elements" we want to 
	 * discover are pairs of pixels (x,y) that have the same label. There are 
	 * numPairs pairs in total. "selected" are numRecSamePairs. All "relevant" 
	 * are numGtSamePairs. "tps" are numBothSamePairs, i.e., all pairs that are 
	 * relevant and have been found in the reconstruction.
	 */

	uint64_t selected = numRecSamePairs;
	uint64_t relevant = numGtSamePairs;
	uint64_t tps      = numBothSamePairs;

	double precision = (double)tps/selected;
	double recall    = (double)tps/relevant;
	double fscore    = 2*(precision*recall)/(precision + recall);

	LOG_DEBUG(randindexlog) << "number of TPs is    " << tps << std::endl;
	LOG_DEBUG(randindexlog) << "number of TPs + FNs " << relevant << std::endl;
	LOG_DEBUG(randindexlog) << "number of TPs + FPs " << selected << std::endl;
	LOG_DEBUG(randindexlog) << "1 - F-score is      " << (1.0 - fscore) << std::endl;

	errors.setNumPairs(numPairs);
	errors.setNumAggreeingPairs(numAgree);
	errors.setPrecision(precision);
	errors.setRecall(recall);
	errors.setAdaptedRandError(1.0 - fscore);

	return errors;
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

	typedef size_t                      Label;
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

	for (auto& p : c) {

		labelPair = p.first;
		n = p.second;

		A += n*(n-1);
		B += n*n;
		numSameComponentPairs12 += n*n;
	}

	for (auto& p : a) {

		label = p.first;
		n = p.second;

		B -= n*n;
		numSameComponentPairs1 += n*n;
	}

	for (auto& p : b) {

		label = p.first;
		n = p.second;

		B -= n*n;
		numSameComponentPairs2 += n*n;
	}

	return (A+B)/2;
}

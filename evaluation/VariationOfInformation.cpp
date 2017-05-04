#include <util/Logger.h>
#include <util/exceptions.h>
#include "VariationOfInformation.h"

logger::LogChannel variationofinformationlog("variationofinformationlog", "[VariationOfInformation] ");

VariationOfInformation::VariationOfInformation(bool ignoreBackground) :
		_ignoreBackground(ignoreBackground) {}

VariationOfInformationErrors
VariationOfInformation::compute(const ImageStack& groundTruth, const ImageStack& reconstruction) {

	if (reconstruction.size() != groundTruth.size())
		UTIL_THROW_EXCEPTION(SizeMismatchError, "image stacks have different size");

	// count label occurences

	_p1.clear();
	_p2.clear();
	_p12.clear();

	ImageStack::const_iterator i1  = reconstruction.begin();
	ImageStack::const_iterator i2  = groundTruth.begin();

	unsigned int n = 0;

	for (; i1 != reconstruction.end(); i1++, i2++) {

		if ((*i1)->size() != (*i2)->size())
			UTIL_THROW_EXCEPTION(SizeMismatchError, "images have different size");

		n += (*i1)->size();

		Image::iterator j1 = (*i1)->begin();
		Image::iterator j2 = (*i2)->begin();

		for (; j1 != (*i1)->end(); j1++, j2++) {

			if (_ignoreBackground && *j2 == 0) {

				n--;
				continue;
			}

			_p1[*j1]++;
			_p2[*j2]++;
			_p12[std::make_pair(*j1, *j2)]++;
		}
	}

	// normalize

	for (typename LabelProb::iterator i = _p1.begin(); i != _p1.end(); i++)
		i->second /= n;
	for (typename LabelProb::iterator i = _p2.begin(); i != _p2.end(); i++)
		i->second /= n;
	for (typename JointLabelProb::iterator i = _p12.begin(); i != _p12.end(); i++)
		i->second /= n;

	// compute information

	// H(stack 1)
	double H1 = 0.0;
	// H(stack 2)
	double H2 = 0.0;
	double I  = 0.0;

	for(typename LabelProb::const_iterator i = _p1.begin(); i != _p1.end(); i++)
		H1 -= i->second * std::log2(i->second);

	for(typename LabelProb::const_iterator i = _p2.begin(); i != _p2.end(); i++)
		H2 -= i->second * std::log2(i->second);

	for(typename JointLabelProb::const_iterator i = _p12.begin(); i != _p12.end(); i++) {

		const size_t j = i->first.first;
		const size_t k = i->first.second;

		const double pjk = i->second;
		const double pj  = _p1[j];
		const double pk  = _p2[k];

		I += pjk * std::log2( pjk / (pj*pk) );
	}

	// H(stack 1, stack2)
	double H12 = H1 + H2 - I;

	VariationOfInformationErrors errors;

	// We compare stack1 (reconstruction) to stack2 (groundtruth). Thus, the 
	// split entropy represents the number of splits of regions in stack2 in 
	// stack1, and the merge entropy the number of merges of regions in stack2 
	// in stack1.
	//
	// H(stack 1|stack 2) = H(stack 1, stack 2) - H(stack 2)
	//   (i.e., if I know the ground truth label, how much bits do I need to 
	//   infer the reconstructino label?)
	errors.setSplitEntropy(H12 - H2);
	// H(stack 2|stack 1) = H(stack 1, stack 2) - H(stack 1)
	//   (i.e., if I know the reconstruction label, how much bits do I need to 
	//   infer the groundtruth label?)
	errors.setMergeEntropy(H12 - H1);

	LOG_DEBUG(variationofinformationlog)
			<< "sum of conditional entropies is " << errors.getEntropy()
			<< ", which should be equal to " << (H1 + H2 - 2.0*I) << std::endl;

	return errors;
}

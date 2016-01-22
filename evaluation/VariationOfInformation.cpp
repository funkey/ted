#include <util/Logger.h>
#include <util/exceptions.h>
#include <util/ProgramOptions.h>
#include "VariationOfInformation.h"

util::ProgramOption optionVoiIgnoreBackground(
		util::_module           = "evaluation",
		util::_long_name        = "voiIgnoreBackground",
		util::_description_text = "For the computation of the VOI, do not consider background pixels in the ground truth.");

logger::LogChannel variationofinformationlog("variationofinformationlog", "[ResultEvaluator] ");

VariationOfInformation::VariationOfInformation(bool headerOnly) :
		_ignoreBackground(optionVoiIgnoreBackground.as<bool>()),
		_headerOnly(headerOnly) {

	if (!_headerOnly) {

		registerInput(_reconstruction, "reconstruction");
		registerInput(_groundTruth, "ground truth");
	}

	registerOutput(_errors, "errors");
}

void
VariationOfInformation::updateOutputs() {

	// set output
	if (!_errors)
		_errors = new VariationOfInformationErrors();

	if (_headerOnly)
		return;

	if (_reconstruction->size() != _groundTruth->size())
		BOOST_THROW_EXCEPTION(SizeMismatchError() << error_message("image stacks have different size") << STACK_TRACE);

	// count label occurences

	_p1.clear();
	_p2.clear();
	_p12.clear();

	ImageStack::const_iterator i1  = _reconstruction->begin();
	ImageStack::const_iterator i2  = _groundTruth->begin();

	unsigned int n = 0;

	for (; i1 != _reconstruction->end(); i1++, i2++) {

		if ((*i1)->size() != (*i2)->size())
			BOOST_THROW_EXCEPTION(SizeMismatchError() << error_message("images have different size") << STACK_TRACE);

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
		H1 -= i->second * std::log(i->second);

	for(typename LabelProb::const_iterator i = _p2.begin(); i != _p2.end(); i++)
		H2 -= i->second * std::log(i->second);

	for(typename JointLabelProb::const_iterator i = _p12.begin(); i != _p12.end(); i++) {

		const float j = i->first.first;
		const float k = i->first.second;

		const double pjk = i->second;
		const double pj  = _p1[j];
		const double pk  = _p2[k];

		I += pjk * std::log( pjk / (pj*pk) );
	}

	// H(stack 1, stack2)
	double H12 = H1 + H2 - I;

	// We compare stack1 (reconstruction) to stack2 (groundtruth). Thus, the 
	// split entropy represents the number of splits of regions in stack2 in 
	// stack1, and the merge entropy the number of merges of regions in stack2 
	// in stack1.
	//
	// H(stack 1|stack 2) = H(stack 1, stack 2) - H(stack 2)
	//   (i.e., if I know the ground truth label, how much bits do I need to 
	//   infer the reconstructino label?)
	_errors->setSplitEntropy(H12 - H2);
	// H(stack 2|stack 1) = H(stack 1, stack 2) - H(stack 1)
	//   (i.e., if I know the reconstruction label, how much bits do I need to 
	//   infer the groundtruth label?)
	_errors->setMergeEntropy(H12 - H1);

	LOG_DEBUG(variationofinformationlog)
			<< "sum of conditional entropies is " << _errors->getEntropy()
			<< ", which should be equal to " << (H1 + H2 - 2.0*I) << std::endl;
}

#include "ExtractGroundTruthLabels.h"
#include <vigra/multi_labeling.hxx>

ExtractGroundTruthLabels::ExtractGroundTruthLabels() {

	registerInput(_gtStack, "ground truth stack");
	registerOutput(_labelStack, "label stack");
}

void
ExtractGroundTruthLabels::updateOutputs() {

	unsigned int width  = _gtStack->width();
	unsigned int height = _gtStack->height();
	unsigned int depth  = _gtStack->size();

	_labelStack = new ImageStack();
	for (unsigned int d = 0; d < depth; d++)
		_labelStack->add(boost::make_shared<Image>(width, height));

	vigra::MultiArray<3, size_t> gtVolume(vigra::Shape3(width, height, depth));
	unsigned int z = 0;
	foreach (boost::shared_ptr<Image> image, *_gtStack) {

		gtVolume.bind<2>(z) = *image;
		z++;
	}

	vigra::MultiArray<3, size_t> gtLabels(vigra::Shape3(width, height, depth));
	vigra::labelMultiArrayWithBackground(
			gtVolume,
			gtLabels);

	z = 0;
	foreach (boost::shared_ptr<Image> image, *_labelStack) {

		static_cast<vigra::MultiArray<2, size_t>& >(*image) = gtLabels.bind<2>(z);
		z++;
	}
}

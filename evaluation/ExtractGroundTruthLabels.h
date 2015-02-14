#ifndef TED_EVAULATION_EXTRACT_GROUND_TRUTH_LABELS_H__
#define TED_EVAULATION_EXTRACT_GROUND_TRUTH_LABELS_H__

#include <pipeline/SimpleProcessNode.h>
#include <imageprocessing/ImageStack.h>

class ExtractGroundTruthLabels : public pipeline::SimpleProcessNode<> {

public:

	ExtractGroundTruthLabels();

private:

	void updateOutputs();

	pipeline::Input<ImageStack>  _gtStack;
	pipeline::Output<ImageStack> _labelStack;
};

#endif // TED_EVAULATION_EXTRACT_GROUND_TRUTH_LABELS_H__


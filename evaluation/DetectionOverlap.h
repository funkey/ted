#ifndef TED_DETECTION_OVERLAP_H__
#define TED_DETECTION_OVERLAP_H__

#include <pipeline/SimpleProcessNode.h>
#include <imageprocessing/ImageStack.h>
#include "DetectionOverlapErrors.h"

/**
 * An error measure that counts the number of TP, FP, and FN regions, based on 
 * inclusion of the ground truth centroid for each region. For TP, two area 
 * overlap measures are computed as well. See
 *
 *   C. Zhang, J. Yarkony, F. A. Hamprecht
 *   Cell detection and segmentation using correlation clustering
 *   MICCIA 2014
 *
 * for details.
 */
class DetectionOverlap : public pipeline::SimpleProcessNode<> {

public:

	DetectionOverlap();

private:

	void updateOutputs();

	// input image stacks
	pipeline::Input<ImageStack> _stack1;
	pipeline::Input<ImageStack> _stack2;

	pipeline::Output<DetectionOverlapErrors> _errors;
};

#endif // TED_DETECTION_OVERLAP_H__


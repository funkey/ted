#ifndef TED_DETECTION_OVERLAP_H__
#define TED_DETECTION_OVERLAP_H__

#include <util/point.hpp>
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
class DetectionOverlap {

public:

	DetectionOverlapErrors compute(const ImageStack& groundTruth, const ImageStack& reconstruction);

private:

	void getCenterPoints(
			const Image&                          image,
			std::map<size_t, util::point<float> >& centers,
			std::set<size_t>&                      labels,
			std::map<size_t, unsigned int>&        sizes);

	void getOverlaps(
			const Image& a,
			const Image& b,
			std::set<std::pair<size_t, size_t> >& overlapPairs,
			std::map<std::pair<size_t, size_t>, unsigned int>& overlapAreas,
			std::map<size_t, std::set<size_t> >& atob,
			std::map<size_t, std::set<size_t> >& btoa);
};

#endif // TED_DETECTION_OVERLAP_H__


#include "DetectionOverlap.h"

DetectionOverlap::DetectionOverlap() {

	registerInput(_stack1, "stack 1");
	registerInput(_stack2, "stack 2");
	registerOutput(_errors, "errors");
}

void
DetectionOverlap::updateOutputs() {

	// TODO:
	// • get centroids in GT (stack 1)
	// • get all overlapping REC regions
	//   -> non overlapping REC regions are FP
	//   -> GT without REC are FN
	// • find optimal matching minimizing center point distance
	//   -> matched REC regions are TP

	// for each TP region, get area overlap measures
}

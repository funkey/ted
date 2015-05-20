#include <vigra/seededregiongrowing.hxx>
#include <vigra/distancetransform.hxx>
#include <imageprocessing/io/ImageStackDirectoryWriter.h>
#include <util/ProgramOptions.h>
#include <util/Logger.h>
#include "ErrorReport.h"

util::ProgramOption optionReportVoi(
		util::_module           = "evaluation",
		util::_long_name        = "reportVoi",
		util::_description_text = "Compute variation of information for the error report.");

util::ProgramOption optionReportRand(
		util::_module           = "evaluation",
		util::_long_name        = "reportRand",
		util::_description_text = "Compute the RAND index for the error report.");

util::ProgramOption optionReportDetectionOverlap(
		util::_module           = "evaluation",
		util::_long_name        = "reportDetectionOverlap",
		util::_description_text = "Compute the detection overlap for the error report.",
		util::_default_value    = true);

util::ProgramOption optionReportTed(
		util::_module           = "evaluation",
		util::_long_name        = "reportTed",
		util::_description_text = "Compute the tolerant edit distance for the error report.",
		util::_default_value    = true);

util::ProgramOption optionGrowSlices(
		util::_module           = "evaluation",
		util::_long_name        = "growSlices",
		util::_description_text = "For the computation of VOI and RAND, grow the reconstruction slices until no background label is present anymore.");

logger::LogChannel errorreportlog("errorreportlog", "[ErrorReport] ");

ErrorReport::ErrorReport(bool headerOnly) :
		_voi(headerOnly),
		_rand(headerOnly),
		_detectionOverlap(headerOnly),
		_ted(headerOnly),
		_reportAssembler(headerOnly),
		_pipelineSetup(false) {

	registerInput(_groundTruthIdMap, "ground truth");
	registerInput(_reconstruction, "reconstruction");

	if (optionReportVoi) {

		_reportAssembler->addInput("errors", _voi->getOutput("errors"));
		registerOutput(_voi->getOutput("errors"), "voi errors");
	}

	if (optionReportRand) {

		_reportAssembler->addInput("errors", _rand->getOutput("errors"));
		registerOutput(_rand->getOutput("errors"), "rand errors");
	}

	if (optionReportDetectionOverlap) {

		_reportAssembler->addInput("errors", _detectionOverlap->getOutput("errors"));
		registerOutput(_detectionOverlap->getOutput("errors"), "detection overlap errors");
	}

	if (optionReportTed) {

		_reportAssembler->addInput("errors", _ted->getOutput("errors"));
		registerOutput(_ted->getOutput("errors"), "ted errors");
	}

	registerOutput(_reportAssembler->getOutput("error report header"), "error report header");

	if (!headerOnly) {

		registerOutput(_reportAssembler->getOutput("error report"), "error report");
		registerOutput(_reportAssembler->getOutput("human readable error report"), "human readable error report");
		registerOutput(_ted->getOutput("corrected reconstruction"), "ted corrected reconstruction");
	}
}

void
ErrorReport::updateOutputs() {

	if (_pipelineSetup)
		return;

	LOG_DEBUG(errorreportlog) << "setting up internal pipeline" << std::endl;

	if (optionGrowSlices) {

		pipeline::Process<> voiRandIdMapProvider;

		voiRandIdMapProvider = pipeline::Process<GrowSlices>();
		voiRandIdMapProvider->setInput(_reconstruction);

		_voi->setInput("stack 2", voiRandIdMapProvider->getOutput());
		_rand->setInput("stack 2", voiRandIdMapProvider->getOutput());

	} else {

		_voi->setInput("stack 2", _reconstruction);
		_rand->setInput("stack 2", _reconstruction);
	}

	_voi->setInput("stack 1", _groundTruthIdMap);
	_rand->setInput("stack 1", _groundTruthIdMap);
	_detectionOverlap->setInput("stack 1", _groundTruthIdMap);
	_detectionOverlap->setInput("stack 2", _reconstruction);
	_ted->setInput("ground truth", _groundTruthIdMap);
	_ted->setInput("reconstruction", _reconstruction);

	_pipelineSetup = true;

	LOG_DEBUG(errorreportlog) << "internal pipeline set up" << std::endl;
}

void
ErrorReport::GrowSlices::updateOutputs() {

	if (!_grown)
		_grown = new ImageStack();
	else
		_grown->clear();

	foreach (boost::shared_ptr<Image> image, *_stack) {

		boost::shared_ptr<Image> grown = boost::make_shared<Image>(image->width(), image->height());
		*grown = *image;

		// perform Euclidean distance transform
		vigra::MultiArray<2, float> dist(image->width(), image->height());
		vigra::distanceTransform(*image, dist, 0, 2);

		float min, max;
		image->minmax(&min, &max);

		// init statistics functor
		vigra::ArrayOfRegionStatistics<vigra::SeedRgDirectValueFunctor<float> > stats(max);

		// grow regions
		vigra::seededRegionGrowing(dist, *grown, *grown, stats);

		_grown->add(grown);
	}

	pipeline::Process<ImageStackDirectoryWriter> writer("result_grown");
	writer->setInput(_grown.getSharedPointer());
	writer->write();
}

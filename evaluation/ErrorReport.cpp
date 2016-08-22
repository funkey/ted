#include <vigra/seededregiongrowing.hxx>
#include <vigra/distancetransform.hxx>
#include <imageprocessing/io/ImageStackDirectoryWriter.h>
#include <util/ProgramOptions.h>
#include <util/Logger.h>
#include "ErrorReport.h"

logger::LogChannel errorreportlog("errorreportlog", "[ErrorReport] ");

ErrorReport::ErrorReport(const Parameters& parameters) :
	_voi(parameters.headerOnly, parameters.ignoreBackground),
	_rand(parameters.headerOnly, parameters.ignoreBackground),
	_detectionOverlap(parameters.headerOnly),
	_ted(parameters.headerOnly),
	_reportAssembler(parameters.headerOnly),
	_pipelineSetup(false),
	_parameters(parameters) {

	if (!parameters.headerOnly) {

		registerInput(_groundTruthIdMap, "ground truth");
		registerInput(_reconstruction, "reconstruction");
	}

	if (parameters.reportVoi) {

		_reportAssembler->addInput("errors", _voi->getOutput("errors"));
		registerOutput(_voi->getOutput("errors"), "voi errors");
	}

	if (parameters.reportRand) {

		_reportAssembler->addInput("errors", _rand->getOutput("errors"));
		registerOutput(_rand->getOutput("errors"), "rand errors");
	}

	if (parameters.reportDetectionOverlap) {

		_reportAssembler->addInput("errors", _detectionOverlap->getOutput("errors"));
		registerOutput(_detectionOverlap->getOutput("errors"), "detection overlap errors");
	}

	if (parameters.reportTed) {

		_reportAssembler->addInput("errors", _ted->getOutput("errors"));
		registerOutput(_ted->getOutput("errors"), "ted errors");
	}

	registerOutput(_reportAssembler->getOutput("error report header"), "error report header");

	if (!parameters.headerOnly) {

		registerOutput(_reportAssembler->getOutput("error report"), "error report");
		registerOutput(_reportAssembler->getOutput("human readable error report"), "human readable error report");

		if (parameters.reportTed)
			registerOutput(_ted->getOutput("corrected reconstruction"), "ted corrected reconstruction");

	} else {

		_pipelineSetup = true;
	}
}

void
ErrorReport::updateOutputs() {

	if (_pipelineSetup)
		return;

	LOG_DEBUG(errorreportlog) << "setting up internal pipeline" << std::endl;

	if (_parameters.growSlices) {

		pipeline::Process<> voiRandIdMapProvider;

		voiRandIdMapProvider = pipeline::Process<GrowSlices>();
		voiRandIdMapProvider->setInput(_reconstruction);

		_voi->setInput("reconstruction", voiRandIdMapProvider->getOutput());
		_rand->setInput("reconstruction", voiRandIdMapProvider->getOutput());

	} else {

		_voi->setInput("reconstruction", _reconstruction);
		_rand->setInput("reconstruction", _reconstruction);
	}

	_voi->setInput("ground truth", _groundTruthIdMap);
	_rand->setInput("ground truth", _groundTruthIdMap);
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

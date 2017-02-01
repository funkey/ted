#ifndef TED_EVALUATION_ERROR_REPORT_H__
#define TED_EVALUATION_ERROR_REPORT_H__

#include <string>
#include <pipeline/SimpleProcessNode.h>
#include <imageprocessing/ImageStack.h>
#include "VariationOfInformation.h"
#include "RandIndex.h"
#include "DetectionOverlap.h"
#include "TolerantEditDistance.h"

class ErrorReport : public pipeline::SimpleProcessNode<> {

public:

	struct Parameters {

		Parameters() :
			headerOnly(false),
			reportTed(true),
			reportRand(false),
			reportVoi(false),
            fromSkeleton(false),
            distanceThreshold(10),
            gtBackgroundLabel(0.0),
            haveBackground(true),
            recBackgroundLabel(0.0),
			reportDetectionOverlap(false),
			ignoreBackground(false),
			growSlices(false) {}

		/**
		 * If set to true, no error will be computed, only the header 
		 * information for the output "error report header" will be fetched.
		 */
		bool headerOnly;

		/**
		 * Compute the TED (enabled by default).
		 */
		bool reportTed;

		/**
		 * Compute the RAND index.
		 */
		bool reportRand;

		/**
		 * Compute VOI.
		 */
		bool reportVoi;

        /**
         * If set to true the ground truth is assumed
         * to be a skeleton.
         */
        bool fromSkeleton;

        /**
         * Distance tolerance for distance to skeleton.
         */
        unsigned int distanceThreshold;

        /**
         * Ground Truth Background label as opposed to skeleton/region label.
         */
        float gtBackgroundLabel;

        /**
        * Does a background label exist at all in the data?
        */
        bool haveBackground;

        /**
        * Reconstruction Background Label.
        */
        float recBackgroundLabel;

		/**
		 * Compute detection overlap (only for 2D images).
		 */
		bool reportDetectionOverlap;

		/**
		 * For VOI and RAND, ignore background pixels in the ground truth.
		 */
		bool ignoreBackground;

		/**
		 * For VOI and RAND, grow reconstruction labels in each image to 
		 * eliminate background.
		 */
		bool growSlices;
	};

	/**
	 * Create a new error report.
	 *
	 * @param parameters
	 *             See struct Parameters.
	 */
	ErrorReport(const Parameters& parameters);

private:

	class ReportAssembler : public pipeline::SimpleProcessNode<> {

	public:

		ReportAssembler(bool headerOnly) :
				_headerOnly(headerOnly) {

			registerInputs(_errors, "errors");
			registerOutput(_reportHeader, "error report header");

			if (!_headerOnly) {

				registerOutput(_report, "error report");
				registerOutput(_humanReadableReport, "human readable error report");
			}
		}

	private:

		void updateOutputs() {

			_reportHeader        = new std::string();
			_report              = new std::string();
			_humanReadableReport = new std::string();

			foreach (const pipeline::Input<Errors>& errors, _errors) {

				if (!_reportHeader->empty())
					(*_reportHeader) += "\t";

				(*_reportHeader) += errors->errorHeader();

				if (!_headerOnly) {

					if (!_report->empty())
						(*_report) += "\t";

					if (!_humanReadableReport->empty())
						(*_humanReadableReport) += "; ";

					(*_report)              += errors->errorString();
					(*_humanReadableReport) += errors->humanReadableErrorString();
				}

			}
		}

		pipeline::Inputs<Errors> _errors;
		pipeline::Output<std::string> _reportHeader;
		pipeline::Output<std::string> _report;
		pipeline::Output<std::string> _humanReadableReport;

		bool _headerOnly;
	};

	/**
	 * Grows all slices in each image of a given stack until no more background 
	 * pixels are present.
	 */
	class GrowSlices : public pipeline::SimpleProcessNode<> {

	public:

		GrowSlices() {

			registerInput(_stack, "stack");
			registerOutput(_grown, "grown");
		}

	private:

		void updateOutputs();

		pipeline::Input<ImageStack>  _stack;
		pipeline::Output<ImageStack> _grown;
	};

	void updateOutputs();

	pipeline::Input<ImageStack> _groundTruthIdMap;
	pipeline::Input<ImageStack> _reconstruction;

	pipeline::Process<VariationOfInformation> _voi;
	pipeline::Process<RandIndex>              _rand;
	pipeline::Process<DetectionOverlap>       _detectionOverlap;
	pipeline::Process<TolerantEditDistance>   _ted;
	pipeline::Process<ReportAssembler>        _reportAssembler;

	pipeline::Output<VariationOfInformationErrors> _voiErrors;
	pipeline::Output<RandIndexErrors>              _randErrors;
	pipeline::Output<TolerantEditDistanceErrors>   _tedErrors;
	pipeline::Output<std::string>                  _reportLine;
	pipeline::Output<std::string>                  _humanReadableReport;

	bool _pipelineSetup;

	Parameters _parameters;
};

#endif // TED_EVALUATION_ERROR_REPORT_H__


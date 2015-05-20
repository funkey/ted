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

	/**
	 * Create a new error report.
	 *
	 * @param headerOnly
	 *              If set to true, no error will be computed, only the header 
	 *              information for the output "error report header" will be 
	 *              fetched.
	 */
	ErrorReport(bool headerOnly = false);

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
};

#endif // TED_EVALUATION_ERROR_REPORT_H__


/**
 * edit distance main file. Initializes all objects, views, and visualizers.
 */

#include <iostream>
#include <imageprocessing/io/ImageStackDirectoryReader.h>
#include <imageprocessing/io/ImageStackDirectoryWriter.h>
#include <pipeline/Process.h>
#include <pipeline/Value.h>
#include <evaluation/ErrorReport.h>
#include <evaluation/ExtractGroundTruthLabels.h>
#include <evaluation/TolerantEditDistanceErrorsWriter.h>
#include <util/ProgramOptions.h>
#include <util/Logger.h>

using namespace logger;

util::ProgramOption optionGroundTruth(
		util::_long_name        = "groundTruth",
		util::_description_text = "The ground truth image stack.",
		util::_default_value    = "groundtruth");

util::ProgramOption optionExtractGroundTruthLabels(
		util::_long_name        = "extractGroundTruthLabels",
		util::_description_text = "Indicate that the ground truth consists of a foreground/background labeling "
		                          "(dark/bright) and each 4-connected component of foreground represents one region.");

util::ProgramOption optionReconstruction(
		util::_long_name        = "reconstruction",
		util::_description_text = "The reconstruction image stack.",
		util::_default_value    = "reconstruction");

util::ProgramOption optionSaveErrors(
		util::_long_name        = "saveErrors",
		util::_description_text = "Create an image stack for every split and merge error. Be careful, this can result in a lot of data.");

util::ProgramOption optionPlotFileRow(
		util::_long_name        = "plotFileRow",
		util::_description_text = "Create a tab-separated single-line error report, which can be used as a row in a plot file.");

util::ProgramOption optionPlotFileHeader(
		util::_long_name        = "plotFileHeader",
		util::_description_text = "Instead of computing the errors, print a single-line column description for the plot file row, which can be used as a header in a plot file.");

int main(int optionc, char** optionv) {

	try {

		/********
		 * INIT *
		 ********/

		// init command line parser
		util::ProgramOptions::init(optionc, optionv);

		// init logger
		LogManager::init();
		Logger::showChannelPrefix(false);

		/*********
		 * SETUP *
		 *********/

		// setup error report

		pipeline::Process<ErrorReport> report(optionPlotFileHeader.as<bool>());
		//pipeline::Process<TolerantEditDistanceErrorsWriter> errorsWriter;

		if (!optionPlotFileHeader) {

			// setup file readers and writers

			pipeline::Process<ImageStackDirectoryReader> groundTruthReader(optionGroundTruth.as<std::string>());
			pipeline::Process<ImageStackDirectoryReader> reconstructionReader(optionReconstruction.as<std::string>());

			report->setInput("reconstruction", reconstructionReader->getOutput());

			if (optionExtractGroundTruthLabels) {

				LOG_DEBUG(out) << "[main] extracting ground truth labels from connected components" << std::endl;

				pipeline::Process<ExtractGroundTruthLabels> extractLabels;
				extractLabels->setInput(groundTruthReader->getOutput());
				report->setInput("ground truth", extractLabels->getOutput());

			} else {

				report->setInput("ground truth", groundTruthReader->getOutput());
			}

			//errorsWriter->setInput("ground truth", groundTruthReader->getOutput());
			//errorsWriter->setInput("reconstruction", reconstructionReader->getOutput());
			//errorsWriter->setInput("ted errors", report->getOutput("ted errors"));
		}

		// save results

		//pipeline::Process<ImageStackDirectoryWriter> correctedWriter("corrected");
		//pipeline::Process<ImageStackDirectoryWriter> splitsWriter("splits");
		//pipeline::Process<ImageStackDirectoryWriter> mergesWriter("merges");
		//pipeline::Process<ImageStackDirectoryWriter> fpWriter("false_positives");
		//pipeline::Process<ImageStackDirectoryWriter> fnWriter("false_negatives");

		//correctedWriter->setInput(report->getOutput("ted corrected reconstruction"));
		//splitsWriter->setInput(editDistance->getOutput("splits"));
		//mergesWriter->setInput(editDistance->getOutput("merges"));
		//fpWriter->setInput(editDistance->getOutput("false positives"));
		//fnWriter->setInput(editDistance->getOutput("false negatives"));

		//correctedWriter->write();
		//splitsWriter->write();
		//mergesWriter->write();
		//fpWriter->write();
		//fnWriter->write();

		//if (optionSaveErrors && !optionPlotFileHeader)
			//errorsWriter->write("errors");

		if (optionPlotFileHeader) {

			pipeline::Value<std::string> reportText = report->getOutput("error report header");

			LOG_USER(out) << *reportText << std::endl;

		} else if (optionPlotFileRow) {

			pipeline::Value<std::string> reportText = report->getOutput("error report");

			LOG_USER(out) << *reportText << std::endl;

		} else {

			pipeline::Value<std::string> reportText = report->getOutput("human readable error report");

			LOG_USER(out) << *reportText << std::endl;
		}

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}



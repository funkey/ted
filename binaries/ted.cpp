/**
 * edit distance main file. Initializes all objects, views, and visualizers.
 */

#include <iostream>
#include <imageprocessing/io/ImageStackDirectoryReader.h>
#include <imageprocessing/io/ImageStackDirectoryWriter.h>
#include <pipeline/Process.h>
#include <pipeline/Value.h>
#include <evaluation/ErrorReport.h>
#include <evaluation/TolerantEditDistanceErrorsWriter.h>
#include <util/ProgramOptions.h>
#include <util/Logger.h>

using namespace logger;

util::ProgramOption optionGroundTruth(
		util::_long_name        = "groundTruth",
		util::_description_text = "The ground truth image stack.",
		util::_default_value    = "groundtruth");

util::ProgramOption optionReconstruction(
		util::_long_name        = "reconstruction",
		util::_description_text = "The reconstruction image stack.",
		util::_default_value    = "reconstruction");

util::ProgramOption optionSaveErrors(
		util::_long_name        = "saveErrors",
		util::_description_text = "Create an image stack for every split and merge error. Be careful, this can result in a lot of data.");

int main(int optionc, char** optionv) {

	try {

		/********
		 * INIT *
		 ********/

		// init command line parser
		util::ProgramOptions::init(optionc, optionv);

		// init logger
		LogManager::init();

		LOG_USER(out) << "[main] starting..." << std::endl;

		/*********
		 * SETUP *
		 *********/

		// setup file readers and writers

		pipeline::Process<ImageStackDirectoryReader> groundTruthReader(optionGroundTruth.as<std::string>());
		pipeline::Process<ImageStackDirectoryReader> reconstructionReader(optionReconstruction.as<std::string>());

		// setup error report

		pipeline::Process<ErrorReport> report;
		report->setInput("ground truth", groundTruthReader->getOutput());
		report->setInput("reconstruction", reconstructionReader->getOutput());

		// save results

		//pipeline::Process<ImageStackDirectoryWriter> correctedWriter("corrected");
		//pipeline::Process<ImageStackDirectoryWriter> splitsWriter("splits");
		//pipeline::Process<ImageStackDirectoryWriter> mergesWriter("merges");
		//pipeline::Process<ImageStackDirectoryWriter> fpWriter("false_positives");
		//pipeline::Process<ImageStackDirectoryWriter> fnWriter("false_negatives");

		//correctedWriter->setInput(editDistance->getOutput("corrected reconstruction"));
		//splitsWriter->setInput(editDistance->getOutput("splits"));
		//mergesWriter->setInput(editDistance->getOutput("merges"));
		//fpWriter->setInput(editDistance->getOutput("false positives"));
		//fnWriter->setInput(editDistance->getOutput("false negatives"));

		//correctedWriter->write();
		//splitsWriter->write();
		//mergesWriter->write();
		//fpWriter->write();
		//fnWriter->write();

		if (optionSaveErrors) {

			pipeline::Process<TolerantEditDistanceErrorsWriter> errorsWriter;
			errorsWriter->setInput("ground truth", groundTruthReader->getOutput());
			errorsWriter->setInput("reconstruction", reconstructionReader->getOutput());
			errorsWriter->setInput("ted errors", report->getOutput("ted errors"));

			errorsWriter->write("errors");
		}

		pipeline::Value<std::string> reportText = report->getOutput("human readable error report");

		LOG_USER(out) << *reportText << std::endl;

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}



/**
 * edit distance main file. Initializes all objects, views, and visualizers.
 */

#include <iostream>
#include <fstream>
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

util::ProgramOption optionPlotFile(
		util::_long_name        = "plotFile",
		util::_description_text = "Append a tab-separated single-line error report to the given file.");

util::ProgramOption optionPlotFileHeader(
		util::_long_name        = "plotFileHeader",
		util::_description_text = "Instead of computing the errors, print a single-line header in the plot file.");

util::ProgramOption optionTedErrorFiles(
		util::_long_name        = "tedErrorFiles",
		util::_description_text = "Create files splits.dat and merges.dat (with background label als fps.dat and fns.dat) which report wich "
		                          "label got split/merged into which.");


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

		if (optionPlotFileHeader) {

			std::ofstream f(optionPlotFile.as<std::string>(), std::ofstream::app);
			pipeline::Value<std::string> reportText = report->getOutput("error report header");

			f << *reportText << std::endl;
			return 0;
		}

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

		// save corrected reconstruction
		pipeline::Process<ImageStackDirectoryWriter> correctedWriter("corrected");
		correctedWriter->setInput(report->getOutput("ted corrected reconstruction"));
		correctedWriter->write();

		// write error report
		pipeline::Value<std::string> reportText = report->getOutput("human readable error report");
		LOG_USER(out) << *reportText << std::endl;

		if (optionTedErrorFiles) {

			// list of split, merge, fp, and fn errors
			pipeline::Value<TolerantEditDistanceErrors> errors = report->getOutput("ted errors");

			std::ofstream splitFile("splits.dat");
			foreach (float gtLabel, errors->getSplitLabels()) {
				splitFile << gtLabel << "\t";
				foreach (float recLabel, errors->getSplits(gtLabel))
					splitFile << recLabel << "\t";
				splitFile << std::endl;
			}
			std::ofstream mergeFile("merges.dat");
			foreach (float recLabel, errors->getMergeLabels()) {
				mergeFile << recLabel << "\t";
				foreach (float gtLabel, errors->getMerges(recLabel))
					mergeFile << gtLabel << "\t";
				mergeFile << std::endl;
			}

			if (errors->hasBackgroundLabel()) {

				std::ofstream fpFile("fps.dat");
				foreach (float recLabel, errors->getFalsePositives())
					fpFile << recLabel << std::endl;
				std::ofstream fnFile("fns.dat");
				foreach (float gtLabel, errors->getFalseNegatives())
					fnFile << gtLabel << std::endl;
			}
		}

		if (optionPlotFile) {

			std::ofstream f(optionPlotFile.as<std::string>(), std::ofstream::app);
			pipeline::Value<std::string> reportText = report->getOutput("error report");

			f << *reportText << std::endl;

		}

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}



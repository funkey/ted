#include <boost/python/numeric.hpp> // TODO: needed?
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>
#include <util/ProgramOptions.h>
#include <util/exceptions.h>
#include <evaluation/VariationOfInformation.h>
#include <evaluation/RandIndex.h>
#include <evaluation/TolerantEditDistance.h>
#include <git_sha1.h>
#include "logging.h"
#include "PyTed.h"

logger::LogChannel pytedlog("pytedlog", "[Ted] ");

PyTed::PyTed(const PyTed::Parameters& parameters) :
		_parameters(parameters),
		_numThreads(0) {

	switch (parameters.verbosity) {

	case 0:
		pyted::setLogLevel(logger::Quiet);
		break;
	case 1:
		pyted::setLogLevel(logger::Error);
		break;
	case 2:
		pyted::setLogLevel(logger::User);
		break;
	case 3:
		pyted::setLogLevel(logger::Debug);
		break;
	default:
		pyted::setLogLevel(logger::All);
		break;
	}

	LOG_DEBUG(pytedlog) << "constructed" << std::endl;
	initialize();
}

void
PyTed::setNumThreads(int numThreads) {

	_numThreads = numThreads;
}

boost::python::dict
PyTed::createReport(PyObject* gt, PyObject* rec, PyObject* voxel_size, PyObject* corrected) {

	util::ProgramOptions::setOptionValue("numThreads", util::to_string(_numThreads));

	boost::python::dict summary;

	ImageStack groundTruth = imageStackFromArray(gt, voxel_size);
	ImageStack reconstruction = imageStackFromArray(rec, voxel_size);

	if (_parameters.reportVoi) {

		VariationOfInformation voi(_parameters.ignoreBackground);
		VariationOfInformationErrors errors = voi.compute(groundTruth, reconstruction);

		summary["voi_split"] = errors.getSplitEntropy();
		summary["voi_merge"] = errors.getMergeEntropy();
	}

	if (_parameters.reportRand) {

		RandIndex rand(_parameters.ignoreBackground);
		RandIndexErrors errors = rand.compute(groundTruth, reconstruction);

		summary["rand_index"] = errors.getRandIndex();
		summary["rand_precision"] = errors.getPrecision();
		summary["rand_recall"] = errors.getRecall();
		summary["adapted_rand_error"] = errors.getAdaptedRandError();
	}

	if (_parameters.reportTed) {

		TolerantEditDistance::Parameters tedParameters;
		tedParameters.fromSkeleton = _parameters.fromSkeleton;
		tedParameters.distanceThreshold = _parameters.distanceThreshold;
		tedParameters.reportFPsFNs = _parameters.haveBackground;
		tedParameters.allowBackgroundAppearance = true; // to be backwards compatible, might change at some point
		tedParameters.gtBackgroundLabel = _parameters.gtBackgroundLabel;
		tedParameters.recBackgroundLabel = _parameters.recBackgroundLabel;
		tedParameters.timeout = _parameters.tedTimeout;

		TolerantEditDistance ted(tedParameters);
		TolerantEditDistanceErrors errors = ted.compute(groundTruth, reconstruction);

		boost::python::dict splits;
		for (size_t split_label : errors.getSplitLabels()) {

			boost::python::list split_into;
			for (size_t into : errors.getSplits(split_label))
				split_into.append(into);
			splits[split_label] = split_into;
		}

		boost::python::dict merges;
		for (size_t merge_label : errors.getMergeLabels()) {

			boost::python::list merge_into;
			for (size_t into : errors.getMerges(merge_label))
				merge_into.append(into);
			merges[merge_label] = merge_into;
		}

		boost::python::list fps;
		if (tedParameters.reportFPsFNs)
			for (size_t l : errors.getFalsePositives())
				fps.append(l);
		boost::python::list fns;
		if (tedParameters.reportFPsFNs)
			for (size_t l : errors.getFalseNegatives())
				fns.append(l);

		boost::python::list matches;
		for (const TolerantEditDistanceErrors::Match& match : errors.getMatches())
			matches.append(
					boost::python::make_tuple(
							match.gtLabel,
							match.recLabel,
							match.overlap));

		if (_parameters.reportTedErrorLocations) {

			boost::python::list splitErrors;
			for (const TolerantEditDistanceErrors::SplitError& splitError : errors.getSplitErrors()) {

				boost::python::dict split_error;
				split_error["gt_label"] = splitError.gtLabel;
				split_error["rec_label_1"] = splitError.recLabel1;
				split_error["rec_label_2"] = splitError.recLabel2;
				split_error["distance"] = splitError.distance;
				split_error["location"] = boost::python::make_tuple(
						splitError.location.z*groundTruth.getResolutionZ(),
						splitError.location.y*groundTruth.getResolutionY(),
						splitError.location.x*groundTruth.getResolutionX());
				split_error["size"] = splitError.size;

				splitErrors.append(split_error);
			}

			boost::python::list mergeErrors;
			for (const TolerantEditDistanceErrors::MergeError& mergeError : errors.getMergeErrors()) {

				boost::python::dict merge_error;
				merge_error["rec_label"] = mergeError.recLabel;
				merge_error["gt_label_1"] = mergeError.gtLabel1;
				merge_error["gt_label_2"] = mergeError.gtLabel2;
				merge_error["distance"] = mergeError.distance;
				merge_error["location"] = boost::python::make_tuple(
						mergeError.location.z*groundTruth.getResolutionZ(),
						mergeError.location.y*groundTruth.getResolutionY(),
						mergeError.location.x*groundTruth.getResolutionX());
				merge_error["size"] = mergeError.size;

				mergeErrors.append(merge_error);
			}

			summary["split_errors"] = splitErrors;
			summary["merge_errors"] = mergeErrors;
		}

		summary["ted_split"] = errors.getNumSplits();
		summary["ted_merge"] = errors.getNumMerges();
		summary["splits"] = splits;
		summary["merges"] = merges;
		summary["matches"] = matches;
		if (tedParameters.reportFPsFNs) {
			summary["ted_fp"] = errors.getNumFalsePositives();
			summary["ted_fn"] = errors.getNumFalseNegatives();
			summary["fps"] = fps;
			summary["fns"] = fns;
		}
		summary["ted_inference_time"] = errors.getInferenceTime();
		summary["ted_num_variables"] = errors.getNumVariables();

		if (corrected != 0)
			imageStackToArray(ted.getCorrectedReconstruction(), corrected);
	}

	summary["ted_version"] = std::string(__git_sha1);

	return summary;
}

ImageStack
PyTed::imageStackFromArray(PyObject* a, PyObject* voxel_size) {

	// create (expect?) a C contiguous uint32 array
	PyArray_Descr* descr = PyArray_DescrFromType(NPY_UINT32);
	PyArrayObject* array = (PyArrayObject*)(PyArray_FromAny(a, descr, 2, 3, 0, NULL));

	PyArray_Descr* vs_descr = PyArray_DescrFromType(NPY_FLOAT64);
	PyArrayObject* vs = (PyArrayObject*)(PyArray_FromAny(voxel_size, vs_descr, 1, 1, 0, NULL));

	if (vs == NULL)
		UTIL_THROW_EXCEPTION(
				UsageError,
				"only voxel size arrays of dimension 1, with datatype np.float64 are supported");

	if (array == NULL)
		UTIL_THROW_EXCEPTION(
				UsageError,
				"only arrays of dimension 2 or 3, with datatype np.uint32 are supported");

	LOG_DEBUG(pytedlog) << "converted to array" << std::endl;

	int dims = PyArray_NDIM(array);
	size_t width, height, depth;

	if (dims == 2) {

		depth = 1;
		height = PyArray_DIM(array, 0);
		width = PyArray_DIM(array, 1);

	} else {

		depth =  PyArray_DIM(array, 0);
		height = PyArray_DIM(array, 1);
		width = PyArray_DIM(array, 2);
	}

	LOG_DEBUG(pytedlog) << "copying data..." << std::endl;

	ImageStack stack;

	double res_x = *static_cast<double*>(PyArray_GETPTR1(vs, 2));
	double res_y = *static_cast<double*>(PyArray_GETPTR1(vs, 1));
	double res_z = *static_cast<double*>(PyArray_GETPTR1(vs, 0));

	stack.setResolution(res_x, res_y, res_z);

	for (size_t z = 0; z < depth; z++) {

		std::shared_ptr<Image> image = std::make_shared<Image>(width, height);
		for (size_t y = 0; y < height; y++)
			for (size_t x = 0; x < width; x++) {

				uint32_t value;
				if (dims == 2)
					value = *static_cast<uint32_t*>(PyArray_GETPTR2(array, y, x));
				else
					value = *static_cast<uint32_t*>(PyArray_GETPTR3(array, z, y, x));

				(*image)(x,y) = value;
			}

		stack.add(image);
	}

	LOG_DEBUG(pytedlog) << "done" << std::endl;

	return stack;
}

void
PyTed::imageStackToArray(const ImageStack& stack, PyObject* a) {

	// create (expect?) a C contiguous uint32 array
	PyArray_Descr* descr = PyArray_DescrFromType(NPY_UINT32);
	PyArrayObject* array = (PyArrayObject*)(PyArray_FromAny(a, descr, 2, 3, 0, NULL));

	if (array == NULL)
		UTIL_THROW_EXCEPTION(
				UsageError,
				"only arrays of dimension 2 or 3, with datatype np.uint32 are supported");

	LOG_DEBUG(pytedlog) << "converted to array" << std::endl;

	int dims = PyArray_NDIM(array);
	size_t width, height, depth;

	if (dims == 2) {

		depth = 1;
		height = PyArray_DIM(array, 0);
		width = PyArray_DIM(array, 1);

	} else {

		depth =  PyArray_DIM(array, 0);
		height = PyArray_DIM(array, 1);
		width = PyArray_DIM(array, 2);
	}

	LOG_DEBUG(pytedlog) << "copying data..." << std::endl;

	for (size_t z = 0; z < depth; z++) {
		for (size_t y = 0; y < height; y++)
			for (size_t x = 0; x < width; x++) {

				uint32_t value = (*stack[z])(x,y);
				if (dims == 2)
					*static_cast<uint32_t*>(PyArray_GETPTR2(array, y, x)) = value;
				else
					*static_cast<uint32_t*>(PyArray_GETPTR3(array, z, y, x)) = value;
			}
	}

	LOG_DEBUG(pytedlog) << "done" << std::endl;
}

void
PyTed::initialize() {

	// import_array is a macro expanding to returning different types across 
	// different versions of the NumPy API. This lambda hack works around that.
	auto a = []{ import_array(); };
	a();
}

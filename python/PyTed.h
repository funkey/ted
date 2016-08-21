#include <boost/python/numeric.hpp>
#include <boost/python/dict.hpp>
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

#include <util/Logger.h>
#include <evaluation/ErrorReport.h>

logger::LogChannel pytedlog("pytedlog", "[Ted] ");

class PyTed {

public:

	PyTed() :
		_reportTed(true),
		_reportRand(true),
		_reportVoi(true) {

		LOG_DEBUG(pytedlog) << "[Ted] constructed" << std::endl;
		initialize();
	}

	void reportTed(bool reportTed)   { _reportTed  = reportTed; }
	void reportRand(bool reportRand) { _reportRand = reportRand; }
	void reportVoi(bool reportVoi)   { _reportVoi  = reportVoi; }

	boost::python::dict createReport(PyObject* gt, PyObject* rec) {

		boost::python::dict summary;

		pipeline::Value<ImageStack> groundTruth = imageStackFromArray(gt);
		pipeline::Value<ImageStack> reconstruction = imageStackFromArray(rec);

		pipeline::Process<ErrorReport> report(false, _reportTed, _reportRand, _reportVoi, false);
		report->setInput("reconstruction", reconstruction);
		report->setInput("ground truth", groundTruth);

		pipeline::Value<RandIndexErrors> randErrors = report->getOutput("rand errors");
		pipeline::Value<VariationOfInformationErrors> voiErrors = report->getOutput("voi errors");

		summary["voi_split"] = voiErrors->getSplitEntropy();
		summary["voi_merge"] = voiErrors->getMergeEntropy();
		summary["rand_index"] = randErrors->getRandIndex();
		return summary;
	}

private:

	pipeline::Value<ImageStack> imageStackFromArray(PyObject* a) {

		// create (expect?) a C contiguous uint32 array
		PyArray_Descr* descr = PyArray_DescrFromType(NPY_UINT32);
		PyArrayObject* array = (PyArrayObject*)(PyArray_FromAny(a, descr, 2, 3, 0, NULL));

		if (array == NULL)
			UTIL_THROW_EXCEPTION(
					UsageError,
					"conversion to array did not work");

		LOG_DEBUG(pytedlog) << "converted to array" << std::endl;

		int dims = PyArray_NDIM(array);
		if (dims != 2 and dims != 3)
			UTIL_THROW_EXCEPTION(
					UsageError,
					"only arrays of dimensions 2 or 3 are supported.");

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

		pipeline::Value<ImageStack> stack;
		for (size_t z = 0; z < depth; z++) {

			boost::shared_ptr<Image> image = boost::make_shared<Image>(width, height);
			for (size_t y = 0; y < height; y++)
				for (size_t x = 0; x < width; x++) {

					uint32_t value;
					if (dims == 2)
						value = *static_cast<uint32_t*>(PyArray_GETPTR2(array, y, x));
					else
						value = *static_cast<uint32_t*>(PyArray_GETPTR3(array, z, y, x));

					if (value > 16777216)
						UTIL_THROW_EXCEPTION(
								Exception,
								"array contains value " << value << " which can not be represented exactly in float (which we unfortunately still use...)");
					(*image)(x,y) = value;
				}

			stack->add(image);
		}

		LOG_DEBUG(pytedlog) << "done" << std::endl;

		return stack;
	}

	void initialize() {

		import_array();
	}

	bool _reportTed;
	bool _reportRand;
	bool _reportVoi;
};

#include <boost/python/dict.hpp>

#include <util/helpers.hpp>
#include <imageprocessing/ImageStack.h>

class PyTed {

public:

	struct Parameters {

		Parameters() :
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
			tedTimeout(0),
			verbosity(2) {}

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
		 * Distance tolerance for TED
		 */
		float distanceThreshold;

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
		 * Timeout for TED computation.
		 */
		double tedTimeout;

		/**
		 * Level of verbosity.
		 *
		 * 0 none
		 * 1 error
		 * 2 user (default)
		 * 3 debug
		 * 4 all
		 */
		int verbosity;
	};

	PyTed(const Parameters& parameters = Parameters());

	void setNumThreads(int numThreads);

	boost::python::dict createReport(PyObject* gt, PyObject* rec, PyObject* voxel_size) { return createReport(gt, rec, voxel_size, 0); }
	boost::python::dict createReport(PyObject* gt, PyObject* rec, PyObject* voxel_size, PyObject* corrected);

private:

	ImageStack imageStackFromArray(PyObject* a, PyObject* voxel_size);

	void imageStackToArray(const ImageStack& stack, PyObject* a);

	void initialize();

	Parameters _parameters;

	int _numThreads;
};

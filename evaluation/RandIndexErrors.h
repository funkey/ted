#ifndef TED_EVALUATION_RAND_INDEX_ERRORS_H__
#define TED_EVALUATION_RAND_INDEX_ERRORS_H__

#include "Errors.h"

class RandIndexErrors : public Errors {

public:

	RandIndexErrors() :
		_numPairs(0),
		_numAgreeing(0) {}

	void setNumPairs(double numPairs) { _numPairs = numPairs; }

	void setNumAggreeingPairs(double numAggreeingPairs) { _numAgreeing = numAggreeingPairs; }

	double getRandIndex() { return _numAgreeing/_numPairs; }

	std::string errorHeader() { return "RAND"; }

	std::string errorString() {

		std::stringstream ss;
		ss << std::scientific << std::setprecision(5);
		ss << getRandIndex();

		return ss.str();
	}

	std::string humanReadableErrorString() {

		std::stringstream ss;
		ss << "RAND: " << getRandIndex();

		return ss.str();
	}

private:

	double _numPairs;
	double _numAgreeing;
};

#endif // TED_EVALUATION_RAND_INDEX_ERRORS_H__


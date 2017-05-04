#ifndef TED_EVALUATION_RAND_INDEX_ERRORS_H__
#define TED_EVALUATION_RAND_INDEX_ERRORS_H__

class RandIndexErrors {

public:

	RandIndexErrors() :
		_numPairs(0),
		_numAgreeing(0) {}

	void setNumPairs(double numPairs) { _numPairs = numPairs; }

	void setNumAggreeingPairs(double numAggreeingPairs) { _numAgreeing = numAggreeingPairs; }

	void setPrecision(double precision) { _precision = precision; }

	void setRecall(double recall) { _recall = recall; }

	void setAdaptedRandError(double arand) { _arand = arand; }

	double getRandIndex() { return _numAgreeing/_numPairs; }

	double getPrecision() { return _precision; }

	double getRecall() { return _recall; }

	double getAdaptedRandError() { return _arand; }

private:

	double _numPairs;
	double _numAgreeing;
	double _precision;
	double _recall;
	double _arand;
};

#endif // TED_EVALUATION_RAND_INDEX_ERRORS_H__


#ifndef TED_DETECTION_OVERLAP_ERRORS_H__
#define TED_DETECTION_OVERLAP_ERRORS_H__

#include "Errors.h"

class DetectionOverlapErrors : public Errors {

public:

	typedef std::pair<float, float> pair_t;

	void addFalsePositive(float label) {

		_fps.insert(label);
	}

	void addFalseNegative(float label) {

		_fns.insert(label);
	}

	/**
	 * Add a ground truth - reconstruction mapping with the given m1 score.
	 */
	void addMatch(pair_t p, float m1, float m2) {

		_matches.insert(p);
		_m1[p] = m1;
		_m2[p] = m2;
	}

	const std::set<float>& getFalsePositives() const {

		return _fps;
	}

	const std::set<float>& getFalseNegatives() const {

		return _fns;
	}

	const std::set<pair_t>& getMatches() const {

		return _matches;
	}

	float getRecall() const {

		float tp = _matches.size();
		float fn = _fns.size();

		return tp/(tp+fn);
	}

	float getPrecision() const {

		float tp = _matches.size();
		float fp = _fps.size();

		return tp/(tp+fp);
	}

	float getFScore() const {

		float p = getPrecision();
		float r = getRecall();

		return 2.0*p*r/(p+r);
	}

	float getM1(pair_t p) const {

		return _m1.at(p);
	}

	float getM2(pair_t p) const {

		return _m2.at(p);
	}

	float getMeanM1() const {

		float sum = 0;
		foreach (const pair_t& p, _matches)
			sum += getM1(p);

		return sum/_matches.size();
	}

	float getStdDevM1() const {

		float mean = getMeanM1();

		float sum = 0;
		foreach (const pair_t& p, _matches)
			sum += pow(mean - getM1(p), 2.0);

		return sqrt(sum/_matches.size());
	}

	float getMeanM2() const {

		float sum = 0;
		foreach (const pair_t& p, _matches)
			sum += getM2(p);

		return sum/_matches.size();
	}

	float getStdDevM2() const {

		float mean = getMeanM2();

		float sum = 0;
		foreach (const pair_t& p, _matches)
			sum += pow(mean - getM2(p), 2.0);

		return sqrt(sum/_matches.size());
	}

	void clear() {

		_fps.clear();
		_fns.clear();
	}

	std::string errorHeader() { return "DO_FP\tDO_FN\tDO_PRE\tDO_REC\tDO_FS\tDO_MEAN_M1\tDO_STD_M1\tDO_MEAN_M2\tDO_STD_M2"; }

	std::string errorString() {

		std::stringstream ss;
		ss << _fps.size() << "\t";
		ss << _fns.size() << "\t";
		ss << getPrecision() << "\t";
		ss << getRecall() << "\t";
		ss << getFScore() << "\t";
		ss << getMeanM1() << "\t";
		ss << getStdDevM1() << "\t";
		ss << getMeanM2() << "\t";
		ss << getStdDevM2() << std::endl;

		return ss.str();
	}

	std::string humanReadableErrorString() {

		std::stringstream ss;
		ss << "DO_FP: "      << _fps.size() << ", ";
		ss << "DO_FN: "      << _fns.size() << ", ";
		ss << "DO_PRE: "     << getPrecision() << ", ";
		ss << "DO_REC: "     << getRecall() << ", ";
		ss << "DO_FS: "      << getFScore() << ", ";
		ss << "DO_MEAN_M1: " << getMeanM1() << ", ";
		ss << "DO_STD_M1: "  << getStdDevM1() << ", ";
		ss << "DO_MEAN_M2: " << getMeanM2() << ", ";
		ss << "DO_STD_M2: "  << getStdDevM2() << std::endl;

		return ss.str();
	}

private:

	std::set<float>  _fps;
	std::set<float>  _fns;
	std::set<pair_t> _matches;

	std::map<pair_t, float> _m1;
	std::map<pair_t, float> _m2;
};

#endif // TED_DETECTION_OVERLAP_ERRORS_H__


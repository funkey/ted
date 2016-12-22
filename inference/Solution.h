#ifndef INFERENCE_SOLUTION_H__
#define INFERENCE_SOLUTION_H__

#include <pipeline/all.h>

class Solution : public pipeline::Data {

public:

	Solution(unsigned int size = 0);

	void resize(unsigned int size);

	unsigned int size() const { return _solution.size(); }

	const double& operator[](unsigned int i) const { return _solution[i]; }

	double& operator[](unsigned int i) { return _solution[i]; }

	std::vector<double>& getVector() { return _solution; }

	void setTime(double time) { _time = time; }

	double getTime() const { return _time; }

private:

	std::vector<double> _solution;

	double _time;
};

#endif // INFERENCE_SOLUTION_H__


#ifndef ANALYZER_H_
#define ANALYZER_H_

#include <math.h>

class Analyzer
{
public:
	void Init() {
		initwindow();
	    fftsize = 4096;
	    fftsizelog2 = 12;
		signalmean = 0.0;
		filteredmean = 0.0;

		enoughdata = false;
		resultready = false;
	}

	int frequencyfftbin(float frequency, int fftsize)
	{
		return roundf(frequency) * (fftsize / 48000.0);
	}

	float fftbinfrequency(int index, int fftsize)
	{
		return float(index) * (48000.0 / fftsize);
	}

	float fftabsvaluedb(float value)
	{
		if (value <= 0) return -144.4;
		return 20*log10f(value);
	}

	void Refresh();
	bool Update(float frequency);
	void Process(float frequency, bool mode);
	void Finish();

private:
	void SplitInput(float *resignal, float *refiltered, float& signalmean, float& filteredmean, int fftsize);
	void fftabs(float *re, float *im, int start, int end, float& maxvalue, int& maxindex, int fftsize);
	void initwindow();


    int fftsize;
    int fftsizelog2;
	float signalmean;
	float filteredmean;

	bool enoughdata;
	bool resultready;
};


#endif /* ANALYZER_H_ */

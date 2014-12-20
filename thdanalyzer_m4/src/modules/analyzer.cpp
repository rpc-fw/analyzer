#ifdef __USE_CMSIS
#include "LPC43xx.h"
#endif

#include <string.h>

#include "analyzer.h"
#include "audio.h"
#include "audioring.h"
#include "../common/sharedtypes.h"

#include "../lib/fft.h"

template <typename T>
T min(T a, T b)
{
	return a < b
		   ? a
		   : b;
}

template <typename T>
T max(T a, T b)
{
	return a > b
		   ? a
		   : b;
}

int msb(unsigned int a)
{
	int bits = -1;
	while (a) {
		a >>= 1;
		bits++;
	}

	return bits;
}


void Analyzer::SplitInput(float *resignal, float *refiltered, float& signalmean, float& filteredmean, int fftsize)
{
	__disable_irq();
	InputRing::RingRange range = inputRing.delayrange(2*fftsize);
	__enable_irq();

	int64_t signalsum = 0;
	int64_t filteredsum = 0;
	for (int i = 0; i < fftsize; i++) {
		int32_t value;

		// get input signal, convert, store
		value = range.value();
		signalsum += value;
		*resignal = value;
		resignal++;
		range.advance();

		// get filtered signal, convert, store
		value = range.value();
		filteredsum += value;
		*refiltered = value;
		refiltered++;
		range.advance();
	}

	signalmean = float(signalsum / fftsize);
	filteredmean = float(filteredsum / fftsize);
}

void Analyzer::fftabs(float *re, float *im, int start, int end, float& maxvalue, int& maxindex, int fftsize)
{
	//float scaling_0dBu = sqrt((6.303352392838346e-25 * 65536.0 * 65536.0) / (2.11592368524*2.11592368524)) / float(fftsize);
	float scaling_0dBu = 2.43089234e-8 / float(fftsize);
	//float scaling_0dBu = 1.20438607134e-8 / float(fftsize);
	float maxv = 0;
	int maxi = 0;

	start = max(1, start);
	start = min(start, fftsize/2);
	end = max(1, end);
	end = min(end, fftsize/2);

	for (int i = start; i < end; ) {
		int n = min(end - i, 256);

		for (; n--; i++) {
			float a = hypotf(re[i], im[i]) * scaling_0dBu;
			re[i] = a;
			if (a > maxv) {
				maxv = a;
				maxi = i;
			}
		}
	}

	maxvalue = maxv;
	maxindex = maxi;
}

#define MAXFFTSIZELOG2 16
#define MAXFFTSIZE (1 << MAXFFTSIZELOG2)

void Analyzer::initwindow()
{
	for (int level = 16; level <= MAXFFTSIZE; level *= 2) {

		float *fftwindow = (float*)(SDRAM_BASE_ADDR + 14*1048576 + level*4);
		float phasescale = (2*M_PI/(float(level) - 1.0));

		for (int i = 0; i < level; i++) {
			//float w = 0.5 * (1.0 - cosf(float(i) * (2*M_PI/(FFTSIZE - 1.0))));

			float phase = float(i) * phasescale;
			float w = 1
					- 1.93 * cosf(phase)
					+ 1.29 * cosf(2*phase)
					- 0.388 * cosf(3*phase)
					+ 0.028 * cosf(4*phase);
			fftwindow[i] = w;
		}
	}
}

void Analyzer::Refresh()
{
	enoughdata = false;
}

bool Analyzer::Update(float frequency)
{
	float *fftmem = (float*)(SDRAM_BASE_ADDR + 15*1048576);
	float *resignal = &fftmem[0*MAXFFTSIZE];
	float *imsignal = &fftmem[1*MAXFFTSIZE];
	float *refiltered = &fftmem[2*MAXFFTSIZE];
	float *imfiltered = &fftmem[3*MAXFFTSIZE];

	int extralen = max(int(4 * audio.SampleRateFloat() / frequency), 200);
	int datalen = (inputRing.used() >> 1);
	int mindatalen = min(max(int(11 * audio.SampleRateFloat() / frequency), 1024), MAXFFTSIZE);

	if (!resultready) {
		if (datalen < mindatalen + extralen) {
			enoughdata = false;
		}
		else {
			enoughdata = true;

			fftsizelog2 = msb(datalen);
			if (fftsizelog2 > MAXFFTSIZELOG2) {
				fftsizelog2 = MAXFFTSIZELOG2;
			}

			fftsize = 1 << fftsizelog2;

			SplitInput(resignal, refiltered, signalmean, filteredmean, fftsize);
		}
	}

	return !resultready;
}

bool Analyzer::CanProcess() const
{
	return enoughdata;
}

void Analyzer::Process(float frequency, bool mode)
{
	float *fftmem = (float*)(SDRAM_BASE_ADDR + 15*1048576);
	float *resignal = &fftmem[0*MAXFFTSIZE];
	float *imsignal = &fftmem[1*MAXFFTSIZE];
	float *refiltered = &fftmem[2*MAXFFTSIZE];
	float *imfiltered = &fftmem[3*MAXFFTSIZE];

	bool include_first_harmonic = false;
	float *re;
	float *im;

	if (mode) {
		re = resignal;
		im = imsignal;
		include_first_harmonic = true;
	}
	else {
		re = refiltered;
		im = imfiltered;
		include_first_harmonic = false;
	}

	float *fftwindow = (float*)(SDRAM_BASE_ADDR + 14*1048576 + fftsize*4);
	for (int i = 0; i < fftsize; i++) {
		float w = fftwindow[i];
		resignal[i] = (resignal[i] - signalmean) * w;
		refiltered[i] = (refiltered[i] - filteredmean) * w;
	}

	memset(imsignal, 0, fftsize*sizeof(float));
	memset(imfiltered, 0, fftsize*sizeof(float));
	fft(re, im, fftsizelog2-1);

	int startbin = frequencyfftbin(frequency, fftsize);
	int endbin = min(frequencyfftbin(frequency * 34, fftsize), frequencyfftbin(audio.SampleRateFloat() - 3000.0, fftsize));

	if (include_first_harmonic) {
		startbin -= 10;
	} else {
		startbin += 10;
	}

	float filteredmaxvalue;
	int filteredmaxbin;
	fftabs(re, im, startbin, endbin, filteredmaxvalue, filteredmaxbin, fftsize);

	*distortionFrequency = fftbinfrequency(filteredmaxbin, fftsize);
	*distortionLevel = fftabsvaluedb(filteredmaxvalue);

	resultready = true;
}

void Analyzer::Finish()
{
	resultready = false;
}


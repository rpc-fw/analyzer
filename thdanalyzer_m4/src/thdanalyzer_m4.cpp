/*
===============================================================================
 Name        : main.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#include <math.h>
#include <string.h>

#ifdef __USE_CMSIS
#include "LPC43xx.h"
#endif

// System clock configuration
extern "C" {
#include "lpc43xx_cgu.h"

void check_failed(uint8_t *file, uint32_t line)
{
	while(1);
}

}

#include <cr_section_macros.h>

#if defined (LPC43_MULTICORE_M0APP) | defined (LPC43_MULTICORE_M0SUB)
#include "cr_start_m0.h"
#endif

// TODO: insert other include files here
#include "emc_setup.h"
#include "modules/audio.h"
#include "lib/RingBuffer.h"
#include "lib/IpcMailbox.h"
#include "lib/LocalMailbox.h"
#include "common/sharedtypes.h"

#include "lib/fft.h"

extern "C"
void SysTick_Handler(void)
{
	// M0 core does not have a systick handler: pass systick as a core interrupt
    __DSB();
    __SEV();
}

// TODO: insert other definitions and declarations here
Audio audio;

struct OscillatorParameters
{
	float e;
	float level;
};

struct FilterParameters
{
	int32_t a1, a2, b0, b1, b2;
};

OscillatorParameters PrecalculateOsc(float frequencyhz, float leveldbu)
{
	OscillatorParameters result;

	float sincoeff = 2.f*3.141592654f*frequencyhz/audio.SampleRateFloat()/2.0f;
	result.e = 2.0f * sinf(sincoeff);

	float levelscale = powf(10.0f, leveldbu * 0.05f);
	result.level = levelscale * 0.5f * 2.19089023f / 25.6f;

	return result;
}

FilterParameters PrecalculateFilter(float f)
{
	float w0 = 2.0*3.141592654*f;

	float Q = 2.8;
	float alpha = sinf(w0)/(2.0*Q);

	float b0 = 1;
	float b1 = -2.0*cosf(w0);
	float b2 = 1;
	float a0 = 1.0 + alpha;
	float a1 = -2.0*cosf(w0);
	float a2 = 1.0 - alpha;

	float scaling = float(1 << 25) / a0;

	FilterParameters result = {
			.a1 = int32_t(floor((a1 * scaling) + 0.5)),
			.a2 = int32_t(floor((a2 * scaling) + 0.5)),
			.b0 = int32_t(floor((b0 * scaling) + 0.5)),
			.b1 = int32_t(floor((b1 * scaling) + 0.5)),
			.b2 = int32_t(floor((b2 * scaling) + 0.5))
	};

	return result;
}

struct OscillatorState
{
	float y;
	float yq;
	float maxlevel;
	float maxlevelinv;
};

struct FilterState
{
	int64_t x1, x2;
	int64_t y1, y2;

	FilterState() {
		x1 = 0;
		x2 = 0;
		y1 = 0;
		y2 = 0;
	}
};

OscillatorState oscstate;
FilterState filterstate;
FilterState filterstate2;
FilterState filterstate3;
FilterState filterstate4;

int32_t Generator(OscillatorState& state, const OscillatorParameters& params, bool reset)
{
	if (reset) {
		// reset oscillator
		state.yq = 1.0;
		state.y = 0.0;
		state.maxlevel = 0.5;
		state.maxlevelinv = 2.0;
	}

	// iterate oscillator
	float e_local = params.e;
	float gain_local = params.level;
	state.yq = state.yq - e_local*state.y;
	state.y = e_local*state.yq + state.y;

	if (state.y > state.maxlevel) {
		state.maxlevel = state.y;
		state.maxlevelinv = 1.0 / state.y;
	}

	gain_local *= state.maxlevelinv;

	// store results
	return int32_t(state.y * gain_local * 2147483648.0);
}

int32_t Filter(int32_t in, FilterState& state, const FilterParameters& params)
{
	int64_t scaledin = int64_t(in) << 5;
	int64_t out = 0;
	out += (int64_t(params.b0) * scaledin);
	out += (int64_t(params.b1) * int64_t(state.x1));
	out += (int64_t(params.b2) * int64_t(state.x2));
	out -= (int64_t(params.a1) * int64_t(state.y1));
	out -= (int64_t(params.a2) * int64_t(state.y2));

	//out *= 2;
	out >>= 25;

	state.x2 = state.x1;
	state.x1 = scaledin;
	state.y2 = state.y1;
	state.y1 = out;

	return out >> 5;
}

struct AudioIrqParameters
{
	OscillatorParameters osc;
	FilterParameters filter;
};

AudioIrqParameters current_params;
LocalMailbox<AudioIrqParameters> oscMailbox;

#define INPUTRINGLEN ((15*1048576)/4)
typedef dsp::RingBufferMemory<int32_t, INPUTRINGLEN, SDRAM_BASE_ADDR> InputRing;
InputRing inputRing;

int32_t nextsample0 = 0;
int32_t nextsample1 = 0;

extern "C"
void I2S0_IRQHandler(void)
{
	LPC_I2S0->TXFIFO = nextsample0;
	LPC_I2S0->TXFIFO = nextsample1;

	// I2S0 and I2S1 run in sync, so we can check both here

	int32_t in0_l = LPC_I2S0->RXFIFO;
	int32_t in0_r = LPC_I2S0->RXFIFO;
	int32_t in1_l = LPC_I2S1->RXFIFO;
	int32_t in1_r = LPC_I2S1->RXFIFO;

	int32_t average = (in0_l >> 2) + (in0_r >> 2) + (in1_l >> 2) + (in1_r >> 2);
	// filter x2
	int32_t filtered1 = Filter(average, filterstate, current_params.filter);
	int32_t filtered2 = Filter(filtered1, filterstate2, current_params.filter);
	int32_t filtered3 = Filter(filtered2, filterstate3, current_params.filter);
	int32_t filtered4 = Filter(filtered3, filterstate4, current_params.filter);
	inputRing.insert(average);
	inputRing.insert(filtered4);

	if (inputRing.used() >= INPUTRINGLEN/2+16UL) {
		inputRing.advance(16);
	}

	*oldestPtr = inputRing.oldestPtr();
	*latestPtr = inputRing.latestPtr()+1;

	bool reset = oscMailbox.Read(current_params);

	// then generate next sample
	nextsample0 = Generator(oscstate, current_params.osc, reset);
	nextsample1 = -nextsample0;
}

AudioIrqParameters CalculateParameters(float frequencyhz, float leveldbu)
{
	AudioIrqParameters irqparams;

	irqparams.osc = PrecalculateOsc(frequencyhz, leveldbu);
	irqparams.filter = PrecalculateFilter(frequencyhz/audio.SampleRateFloat());

	return irqparams;
}

void SplitInput(float *resignal, float *refiltered, float& signalmean, float& filteredmean)
{
#define FFTSIZE 65536
	__disable_irq();
	InputRing::RingRange range = inputRing.delayrange(2*FFTSIZE);
	__enable_irq();

	int64_t signalsum = 0;
	int64_t filteredsum = 0;
	for (int i = 0; i < FFTSIZE; i++) {
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

	signalmean = float(signalsum / FFTSIZE);
	filteredmean = float(filteredsum / FFTSIZE);
}

void fftabs(float *re, float *im)
{
	// 1/max(abs(f).^2)
	float scaling_0dBu = 6.303352392838346e-25;
	for (int i = 1; i < FFTSIZE/2+1; i++) {
		float a = (re[i] * re[i] + im[i] * im[i]) * scaling_0dBu;
		re[i] = a;
	}
	re[0] = 0;
}

int main(void)
{
	const int clock_multiplier = 17;
    CGU_Init(clock_multiplier);
    SystemCoreClock = clock_multiplier*12000000;

    // Start M0APP slave processor
#if defined (LPC43_MULTICORE_M0APP)
    cr_start_m0(SLAVE_M0APP,&__core_m0app_START__);
#endif

    // Start M0SUB slave processor
#if defined (LPC43_MULTICORE_M0SUB)
    cr_start_m0(SLAVE_M0SUB,&__core_m0sub_START__);
#endif

    SysTick_Config(SystemCoreClock/1000);
    emc_init();
	memset((unsigned int*)SDRAM_BASE_ADDR, 0xFF, SDRAM_SIZE_BYTES);

	// Prepare for first I2S interrupt
	oscMailbox.Write(CalculateParameters(1000.0, 4.0));

	__disable_irq();
    audio.Init();
    __enable_irq();

    bool disableAnalysis = false;

    while(1) {

		float *fftmem = (float*)(SDRAM_BASE_ADDR + 15*1048576);
		float *resignal = &fftmem[0*FFTSIZE];
		float *imsignal = &fftmem[1*FFTSIZE];
		float *refiltered = &fftmem[2*FFTSIZE];
		float *imfiltered = &fftmem[3*FFTSIZE];

    	if (!disableAnalysis) {

    		float signalmean;
    		float filteredmean;
    		SplitInput(resignal, refiltered, signalmean, filteredmean);

			for (int i = 0; i < FFTSIZE; i++) {
				float window = 0.5 * (1.0 - cosf(float(i) * (2*M_PI/(FFTSIZE - 1.0))));
				resignal[i] = (resignal[i] - signalmean) * window;
				refiltered[i] = (refiltered[i] - filteredmean) * window;
			}
			memset(imsignal, 0, FFTSIZE*sizeof(float));
			memset(imfiltered, 0, FFTSIZE*sizeof(float));
    	}

    	GeneratorParameters newparams;
    	if (commandMailbox.Read(newparams)) {
    		oscMailbox.Write(CalculateParameters(newparams.frequency, newparams.level));
    		ackMailbox.Write(true);
    	}

    	AnalysisCommand analysiscmd;
    	if (analysisCommandMailbox.Read(analysiscmd)) {
    		if (analysiscmd.commandType == AnalysisCommand::BLOCK) {

    			fft(resignal, imsignal, 15);
    			fft(refiltered, imfiltered, 15);

    			fftabs(resignal, imsignal);
    			fftabs(refiltered, imfiltered);

    			disableAnalysis = true;
    		}
    		else {
    			disableAnalysis = false;
    		}
        	analysisAckMailbox.Write(true);
    	}

    }
	return 0;
}

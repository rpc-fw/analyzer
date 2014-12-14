#ifdef __USE_CMSIS
#include "LPC43xx.h"
#endif

#include "process.h"
#include "audio.h"
#include "common/sharedtypes.h"
#include "lib/LocalMailbox.h"

Process process;

struct AudioIrqParameters
{
	OscillatorParameters osc;
	FilterParameters filter;

	bool balancedio;
};

AudioIrqParameters current_params;
LocalMailbox<AudioIrqParameters> oscMailbox;

static AudioIrqParameters CalculateParameters(float frequencyhz, float leveldbu, bool balancedio)
{
	AudioIrqParameters irqparams;

	irqparams.osc = PrecalculateOsc(frequencyhz, leveldbu);
	irqparams.filter = PrecalculateFilter(frequencyhz/audio.SampleRateFloat());
	irqparams.balancedio = balancedio;

	return irqparams;
}

void Process::Init()
{
	// Prepare for first I2S interrupt
	SetParameters(1000.0, 4.0, true);
}

void Process::SetParameters(float frequencyhz, float leveldbu, bool balancedio)
{
	oscMailbox.Write(CalculateParameters(frequencyhz, leveldbu, balancedio));
}

#include "modules/audioring.h"
InputRing inputRing;

OscillatorState oscstate;
FilterState filterstate;
FilterState filterstate2;
FilterState filterstate3;
FilterState filterstate4;

int32_t nextsample_pos = 0;
int32_t nextsample_neg = 0;

extern "C"
void I2S0_IRQHandler(void)
{
	LPC_I2S0->TXFIFO = nextsample_neg;
	LPC_I2S0->TXFIFO = nextsample_pos;

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
	nextsample_pos = Generator(oscstate, current_params.osc, reset);

	if (current_params.balancedio) {
		nextsample_neg = -nextsample_pos;
	}
	else {
		nextsample_neg = 0;
	}

	if (reset) {
		inputRing.clear();
	}
}

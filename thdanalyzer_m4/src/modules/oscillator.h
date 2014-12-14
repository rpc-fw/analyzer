#ifndef OSCILLATOR_H_
#define OSCILLATOR_H_

#include <stdint.h>

struct OscillatorParameters
{
	float e;
	float level;
};

struct OscillatorState
{
	float y;
	float yq;
	float maxlevel;
	float maxlevelinv;
};

OscillatorParameters PrecalculateOsc(float frequencyhz, float leveldbu);
int32_t Generator(OscillatorState& state, const OscillatorParameters& params, bool reset);

#endif /* OSCILLATOR_H_ */

#ifndef PROCESS_H_
#define PROCESS_H_

#include "oscillator.h"
#include "filter.h"

#include "audioring.h"
extern InputRing inputRing;

class Process
{
public:
	enum GeneratorMode
	{
		GeneratorModeOscillator,
		GeneratorModeDC
	};
	void Init();
	void SetParameters(GeneratorMode mode, float frequencyhz, float leveldbu, bool balancedio, float cv0, float cv1);
};

extern Process process;

#endif /* PROCESS_H_ */

#ifndef PROCESS_H_
#define PROCESS_H_

#include "oscillator.h"
#include "filter.h"

#include "audioring.h"
extern InputRing inputRing;

class Process
{
public:
	void Init();
	void SetParameters(float frequencyhz, float leveldbu, bool balancedio);
};

extern Process process;

#endif /* PROCESS_H_ */

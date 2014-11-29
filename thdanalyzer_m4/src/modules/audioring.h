#ifndef AUDIORING_H_
#define AUDIORING_H_

#include "../emc_setup.h"
#include "../lib/RingBuffer.h"

#define INPUTRINGLEN ((14*1048576)/4)
typedef dsp::RingBufferMemory<int32_t, INPUTRINGLEN, SDRAM_BASE_ADDR> InputRing;
extern InputRing inputRing;

#endif /* AUDIORING_H_ */

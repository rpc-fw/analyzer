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

OscillatorParameters oscparam[3];

OscillatorParameters* volatile current_params = &oscparam[0];
OscillatorParameters* volatile next_params = current_params;

OscillatorParameters* FindFreeParams()
{
	bool used[3] = { false, false, false };

	OscillatorParameters* next_ptr = next_params;

	used[current_params - &oscparam[0]] = true;
	if (next_ptr) {
		used[next_ptr - &oscparam[0]] = true;
	}

	for (int i = 0; i < 3; i++) {
		if (!used[i]) {
			return &oscparam[i];
		}
	}

	// shouldn't reach here
	//while(1);
	return &oscparam[0];
}

float y = 0.0;
float yq = 1.0;
float maxlevel = 1.0;
float maxlevelinv = 1.0;

volatile int last_update = 0;

#define INPUTRINGLEN (SDRAM_SIZE_BYTES/4)
//dsp::RingBufferStatic<int32_t, 4> outputRing;
dsp::RingBufferMemory<int32_t, INPUTRINGLEN, SDRAM_BASE_ADDR> inputRing;

const int32_t** inputRingReadPtr = (const int32_t**) 0x2000C000;

extern "C"
void I2S0_IRQHandler(void)
{
	if (next_params != 0) {
		current_params = next_params;
		next_params = 0;

		// reset oscillator
		yq = 1.0;
		y = 0.0;
		maxlevel = 0.5;
		maxlevelinv = 2.0;
	}
	int outputFifoLevel = (LPC_I2S0->STATE >> 16) & 0xF;
	if (outputFifoLevel <= 6) {
		// iterate oscillator
		float e_local = current_params->e;
		float gain_local = current_params->level;
	 	yq = yq - e_local*y;
		y = e_local*yq + y;

		if (y > maxlevel) {
			maxlevel = y;
			maxlevelinv = 1.0 / y;
		}

		gain_local *= maxlevelinv;

		// write
		LPC_I2S0->TXFIFO = int32_t(y * gain_local * 2147483648.0);
		LPC_I2S0->TXFIFO = int32_t(-y * gain_local * 2147483648.0);
	}

	// I2S0 and I2S1 run in sync, so we can check both here

	int inputFifoLevel0 = (LPC_I2S0->STATE >> 8) & 0xF;
	if (inputFifoLevel0 >= 2) {
		int32_t in_l = LPC_I2S0->RXFIFO;
		int32_t in_r = LPC_I2S0->RXFIFO;
		inputRing.insert(in_l);
		inputRing.insert(in_r);
	}

	int inputFifoLevel1 = (LPC_I2S1->STATE >> 8) & 0xF;
	if (inputFifoLevel1 >= 2) {
		int32_t in_l = LPC_I2S1->RXFIFO;
		int32_t in_r = LPC_I2S1->RXFIFO;
		inputRing.insert(in_l);
		inputRing.insert(in_r);
	}

	if (inputRing.used() >= INPUTRINGLEN/2+16UL) {
		inputRing.advance(16);
	}

	*inputRingReadPtr = inputRing.oldestPtr();
}

struct GeneratorParameters
{
	int32_t update;
	float frequency;
	float level;
};

int main(void) {

	const int clock_multiplier = 17;
    CGU_Init(clock_multiplier);
    SystemCoreClock = clock_multiplier*12000000;

    // Setup parameters
	volatile GeneratorParameters* params = (volatile GeneratorParameters*) 0x2000C010;
	params->update = 1;
	params->frequency = 440.0;
	params->level = 4.0;

	oscparam[0].e = 0.0;
	oscparam[0].level = 0.0;

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

    audio.Init();

    __enable_irq();

    while(1) {
    	GeneratorParameters curparams;
    	memcpy(&curparams, (void*)params, sizeof(curparams));
    	if (last_update != curparams.update) {

    		OscillatorParameters* other_params = FindFreeParams();

    		float sincoeff = 2.f*3.141592654f*curparams.frequency/48000.0f/2.0f;
    		other_params->e = 2.0f * sinf(sincoeff);

    		float levelscale = powf(10.0f, curparams.level * 0.05f);
    		other_params->level = levelscale * 0.5f * 2.19089023f / 25.6f;
    		last_update = curparams.update;

    		// put new parameter set in mailbox
    		next_params = other_params;
    	}
    }
	return 0;
}

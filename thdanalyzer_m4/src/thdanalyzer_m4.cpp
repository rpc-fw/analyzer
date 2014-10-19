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

LocalMailbox<OscillatorParameters> oscMailbox;

float y = 0.0;
float yq = 1.0;
float maxlevel = 1.0;
float maxlevelinv = 1.0;

volatile int last_update = 0;

#define INPUTRINGLEN (SDRAM_SIZE_BYTES/4)
//dsp::RingBufferStatic<int32_t, 4> outputRing;
dsp::RingBufferMemory<int32_t, INPUTRINGLEN, SDRAM_BASE_ADDR> inputRing;

OscillatorParameters current_params = { .e = 0, .level = 0 };

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
	inputRing.insert(in0_l);
	inputRing.insert(in0_r);

	int32_t in1_l = LPC_I2S1->RXFIFO;
	int32_t in1_r = LPC_I2S1->RXFIFO;
	inputRing.insert(in1_l);
	inputRing.insert(in1_r);

	if (inputRing.used() >= INPUTRINGLEN/2+16UL) {
		inputRing.advance(16);
	}

	*oldestPtr = inputRing.oldestPtr();
	*latestPtr = inputRing.latestPtr()+1;

	// then generate next sample

	if (oscMailbox.Read(current_params)) {

		// reset oscillator
		yq = 1.0;
		y = 0.0;
		maxlevel = 0.5;
		maxlevelinv = 2.0;
	}

	// iterate oscillator
	float e_local = current_params.e;
	float gain_local = current_params.level;
	yq = yq - e_local*y;
	y = e_local*yq + y;

	if (y > maxlevel) {
		maxlevel = y;
		maxlevelinv = 1.0 / y;
	}

	gain_local *= maxlevelinv;

	// store results
	nextsample0 = int32_t(y * gain_local * 2147483648.0);
	nextsample1 = int32_t(-y * gain_local * 2147483648.0);
}

int main(void) {

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

	__disable_irq();
    audio.Init();
    __enable_irq();

    while(1) {
    	GeneratorParameters newparams;
    	if (commandMailbox.Read(newparams)) {
    		OscillatorParameters irqparams;

    		float sincoeff = 2.f*3.141592654f*newparams.frequency/48000.0f/2.0f;
    		irqparams.e = 2.0f * sinf(sincoeff);

    		float levelscale = powf(10.0f, newparams.level * 0.05f);
    		irqparams.level = levelscale * 0.5f * 2.19089023f / 25.6f;

    		oscMailbox.Write(irqparams);
    		ackMailbox.Write(true);
    	}
    }
	return 0;
}

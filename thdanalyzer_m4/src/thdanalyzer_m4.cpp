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

float e = 2.0f * sin(2*3.141592654*440.0/48000.0/2.0);
float y = 0.0;
float yq = 1.0;

#define INPUTRINGLEN (SDRAM_SIZE_BYTES/4)
//dsp::RingBufferStatic<int32_t, 4> outputRing;
dsp::RingBufferMemory<int32_t, INPUTRINGLEN, SDRAM_BASE_ADDR> inputRing;

extern "C"
void I2S0_IRQHandler(void)
{
	int outputFifoLevel = (LPC_I2S0->STATE >> 16) & 0xF;
	if (outputFifoLevel <= 6) {
		// iterate oscillator
	 	yq = yq - e*y;
		y = e*yq + y;

		// write
		LPC_I2S0->TXFIFO = int32_t(y * 0.5 * 2147483648.0);
		LPC_I2S0->TXFIFO = int32_t(-y * 0.5 * 2147483648.0);
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

	if (inputRing.used() >= INPUTRINGLEN-16) {
		inputRing.advance(16);
	}
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

    audio.Init();

    __enable_irq();

    while(1) {}
	return 0;
}

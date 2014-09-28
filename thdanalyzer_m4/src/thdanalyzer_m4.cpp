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

extern "C"
void SysTick_Handler(void)
{
	// M0 core does not have a systick handler: pass systick as a core interrupt
    __DSB();
    __SEV();
}

// TODO: insert other definitions and declarations here
Audio audio;

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

    unsigned int* const membase = (unsigned int*)SDRAM_BASE_ADDR;
    const int memsize = SDRAM_SIZE_BYTES/4;
    int counter0 = 0;
    int counter1 = 2;
    float e = 2.0f * sin(2*3.141592654*440.0/48000.0/2.0);
    float y = 0.0;
    float yq = 1.0;
    while (1)
    {
    	// wait for audio tx ready
    	while (((LPC_I2S0->STATE >> 16) & 0xF) > 6);

    	// iterate oscillator
     	yq = yq - e*y;
    	y = e*yq + y;

    	// write
    	LPC_I2S0->TXFIFO = int32_t(y * 2147483648.0);
    	LPC_I2S0->TXFIFO = int32_t(-y * 2147483648.0);

    	// read
		while (((LPC_I2S0->STATE >> 8) & 0xF) > 1) {
			int32_t in_l = LPC_I2S0->RXFIFO;
			int32_t in_r = LPC_I2S0->RXFIFO;
			membase[counter0++] = in_l;
			membase[counter0++] = in_r;
			counter0 += 2;
			if (counter0 >= memsize) {
				counter0 = 0;
			}
		}
		while (((LPC_I2S1->STATE >> 8) & 0xF) > 1) {
			int32_t in_l = LPC_I2S1->RXFIFO;
			int32_t in_r = LPC_I2S1->RXFIFO;
			membase[counter1++] = in_l;
			membase[counter1++] = in_r;
			counter1 += 2;
			if (counter1 >= memsize) {
				counter1 = 2;
			}
		}
    }

#if 0
    /* ok, then what? */

    unsigned int counter = 1;//0x55AA5500;
    unsigned int* membase = (unsigned int*)SDRAM_BASE_ADDR;
    const int memsize = (128/8)*1048576;
    volatile unsigned int lastfail = 0xFFFFFFFF;
    volatile unsigned int failures = 0;
    volatile unsigned int cycles = 0;

	for (int i = 0; i < memsize/4; i++) {
		membase[i] = 0xFFFFFFFF;
	}

    while(1) {
    	for (int i = 0; i < memsize/4; i++) {
    		membase[i] = counter * (i + 1);
    	}

    	for (int i = 0; i < memsize/4-16; i++) {
    		if (membase[i] != counter * (i + 1))
    		{
    			lastfail = i;
    			failures++;
    		}
    	}

    	counter += 0x55AA55FF;
    	cycles++;
    }
#endif
	// should not reach this
    while(1) {}
	return 0;
}

/*
===============================================================================
 Name        : main.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#ifdef __USE_CMSIS
#include "LPC43xx.h"
#endif

// System clock configuration
extern "C" {
#include "lpc43xx_cgu.h"
}

#include <cr_section_macros.h>

#if defined (LPC43_MULTICORE_M0APP) | defined (LPC43_MULTICORE_M0SUB)
#include "cr_start_m0.h"
#endif

// TODO: insert other include files here
#include "emc_setup.h"

extern "C"
void SysTick_Handler(void)
{
	// M0 core does not have a systick handler: pass systick as a core interrupt
    __DSB();
    __SEV();
}

// TODO: insert other definitions and declarations here

int main(void) {

	const int clock_multiplier = 17;
    CGU_Init(clock_multiplier);
    SystemCoreClock = clock_multiplier*12000000;

    SysTick_Config(SystemCoreClock/1000);
    emc_init();

    // Start M0APP slave processor
#if defined (LPC43_MULTICORE_M0APP)
    cr_start_m0(SLAVE_M0APP,&__core_m0app_START__);
#endif

    // Start M0SUB slave processor
#if defined (LPC43_MULTICORE_M0SUB)
    cr_start_m0(SLAVE_M0SUB,&__core_m0sub_START__);
#endif

    __enable_irq();

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

	// should not reach this
    while(1) {}
	return 0;
}

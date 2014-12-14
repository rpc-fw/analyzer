/*
===============================================================================
 Name        : main.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#include <string.h>
#include <algorithm>

#include "FreeRTOS.h"
#include "task.h"

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
#include "modules/analyzer.h"
#include "modules/process.h"
#include "lib/fft.h"

extern "C"
void M0CORE_IRQHandler(void)
{
	fft_interrupt();
	// clear event interrupt
	LPC_CREG->M0TXEVENT = 0x0;
}

uint8_t freertos_heap[0x8000];

void init_freertos_heap()
{
	const HeapRegion_t xHeapRegions[] =
	{
	    { freertos_heap, 0x8000 },
	    { NULL, 0 } /* Terminates the array. */
	};

	// Initialize heap */
	vPortDefineHeapRegions(xHeapRegions);
}

extern "C"
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
        configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
        function is called if a stack overflow is detected. */
	taskDISABLE_INTERRUPTS();
	for( ;; );
}


Analyzer analyzer;

// Main task
void vInitTask(void* pvParameters)
{
    GeneratorParameters params = GeneratorParameters(1000.0, 4.0, true, false);

	while(1) {
    	if (analyzer.Update(params._frequency)) {
			if (commandMailbox.Read(params)) {
				process.SetParameters(params._frequency, params._level, params._balancedio);
				ackMailbox.Write(true);
				analyzer.Refresh();
			}
    	}

		AnalysisCommand analysiscmd;
		if (analysisCommandMailbox.Read(analysiscmd)) {
			if (analysiscmd.commandType == AnalysisCommand::BLOCK) {
				analyzer.Process(params._frequency, params._analysismode);
			}
			else {
				analyzer.Finish();
			}
			analysisAckMailbox.Write(true);
    	}
		taskYIELD();
	}
}

void set_clock_frequency()
{
	const int clock_multiplier = 17;
    CGU_Init(clock_multiplier);
    SystemCoreClock = clock_multiplier*12000000;
}

void set_fpu_configuration()
{
    FPU->FPCCR |= FPU_FPCCR_ASPEN_Msk;
    FPU->FPCCR &= ~FPU_FPCCR_LSPEN_Msk;
}

void start_coprocessors()
{
    // Start M0APP slave processor
#if defined (LPC43_MULTICORE_M0APP)
    cr_start_m0(SLAVE_M0APP,&__core_m0app_START__);
#endif

    // Start M0SUB slave processor
#if defined (LPC43_MULTICORE_M0SUB)
    cr_start_m0(SLAVE_M0SUB,&__core_m0sub_START__);
#endif
}

void init_sdram()
{
    emc_init();
	memset((unsigned int*)SDRAM_BASE_ADDR, 0xFF, SDRAM_SIZE_BYTES);
}

int main(void)
{
	set_clock_frequency();

    set_fpu_configuration();

    init_freertos_heap();

    start_coprocessors();

    // Spawn init task
    xTaskCreate(vInitTask, "init", 2048, NULL, tskIDLE_PRIORITY, NULL);

    init_sdram();

	analyzer.Init();
	process.Init();

	__disable_irq();

	// Enable M0 interrupt
	NVIC_SetPriority(M0CORE_IRQn, 3);
	NVIC_EnableIRQ(M0CORE_IRQn);

    audio.Init();

    __enable_irq();

    // Run main loop
    vTaskStartScheduler();

	return 0;
}

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
#include "queue.h"

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

enum MainTaskEvent {
	AnalyzerDone,
	InterruptAnalyzer
};


Analyzer analyzer;

GeneratorParameters params = GeneratorParameters(1000.0, 4.0, true, false);

QueueHandle_t processingDoneQueue;

extern "C"
void M0CORE_IRQHandler(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	MainTaskEvent msg = InterruptAnalyzer;
	xQueueSendToBackFromISR(processingDoneQueue, &msg, &xHigherPriorityTaskWoken);

	// clear event interrupt
	LPC_CREG->M0TXEVENT = 0x0;

	if (xHigherPriorityTaskWoken != pdFALSE) {
		taskYIELD();
	}
}

void vProcessTask(void* pvParameters)
{
	analyzer.Process(params._frequency, params._analysismode);

	// ack process to main task
	MainTaskEvent result = AnalyzerDone;
	xQueueSend(processingDoneQueue, &result, 0);

	vTaskDelete(NULL);
}

// Main task
void vMainTask(void* pvParameters)
{
	TaskHandle_t analyzerTaskHandle = NULL;
	bool needToStart = false;

	while(1) {
		if (commandMailbox.Read(params)) {
			process.SetParameters(params._frequency, params._level, params._balancedio);
			ackMailbox.Write(true);
			analyzer.Refresh();
		}

		if (analyzerTaskHandle == NULL) {
				 analyzer.Update(params._frequency);
    	}

		AnalysisCommand analysiscmd;
		if (analysisCommandMailbox.Read(analysiscmd)) {
			if (analysiscmd.commandType == AnalysisCommand::BLOCK) {
				needToStart = true;
			}
			else {
				analyzer.Finish();
				analysisAckMailbox.Write(true);
			}
    	}

		if (needToStart && analyzer.CanProcess()) {
			xQueueReset(processingDoneQueue);
			// start process task
			BaseType_t r = xTaskCreate(vProcessTask, "process", 2048, NULL, 1, &analyzerTaskHandle);
			if (analyzerTaskHandle != NULL) {
				needToStart = false;
			}
		}

		if (analyzerTaskHandle != NULL) {
			// wait
			MainTaskEvent msg;
			if (xQueueReceive(processingDoneQueue, &msg, 0) == pdTRUE)
			{
				if (msg == InterruptAnalyzer) {
					// need to stop the analyzer
					vTaskSuspend(analyzerTaskHandle);
					vTaskDelete(analyzerTaskHandle);
					xQueueReset(processingDoneQueue);
				}

				analysisAckMailbox.Write(true);
				analyzerTaskHandle = NULL;
			}
		}

		// Delay instead of yielding, so that idle process can sweep out deleted tasks
		vTaskDelay(1);
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

    // Spawn main task
    xTaskCreate(vMainTask, "main", 256, NULL, 2, NULL);
	processingDoneQueue = xQueueCreate(32, sizeof(MainTaskEvent));

    init_sdram();

	analyzer.Init();
	process.Init();

	__disable_irq();

	// Enable M0 interrupt
	NVIC_SetPriority(M0CORE_IRQn, 1);
	NVIC_EnableIRQ(M0CORE_IRQn);

    audio.Init();

    __enable_irq();

    // Run main loop
    vTaskStartScheduler();

	return 0;
}

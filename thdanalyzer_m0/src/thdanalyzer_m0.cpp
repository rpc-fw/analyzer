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

#include <cr_section_macros.h>

// TODO: insert other include files here
#include "modules/ethernet/EthernetHost.h"
#include "FreeRTOS/include/freertos.h"
#include "FreeRTOS/include/task.h"

#include <stdlib.h>

uint32_t SystemCoreClock;

// TODO: insert other definitions and declarations here
EthernetHost ethhost;

#if defined (M0_SLAVE_PAUSE_AT_MAIN)
volatile unsigned int pause_at_main;
#endif

void init_freertos_heap()
{
	// Allocate the 16kB AHB SRAM bank at 0x20008000 for the Ethernet/IP stack.
	const HeapRegion_t xHeapRegions[] =
	{
	    { ( uint8_t * ) 0x20000000UL, 0x10000 },
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

// Task to be created.
void vInitTask(void* pvParameters)
{
	ethhost.Init();

	while(1) {
		taskYIELD();
	}
}

int main(void) {

#if defined (M0_SLAVE_PAUSE_AT_MAIN)
    // Pause execution until debugger attaches and modifies variable
    while (pause_at_main == 0) {}
#endif

    SystemCoreClock = 204000000;

    // TODO: insert code here
    init_freertos_heap();

    // Force the counter to be placed into memory
    volatile static int i = 0 ;
    volatile static int keeplooping = 0 ;
    keeplooping = 0;
    // Enter an infinite loop, just incrementing a counter
    while(keeplooping) {
        i++ ;
    }

    // Spawn init task
    xTaskCreate(vInitTask, "init", 2048, NULL, tskIDLE_PRIORITY, NULL);

    // Run main loop
    vTaskStartScheduler();

    return 0 ;
}

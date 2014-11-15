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
#include "modules/lcd1602.h"
#include "modules/frontpanelcontrols.h"

#include "FreeRTOS/include/freertos.h"
#include "FreeRTOS/include/task.h"

#include <stdlib.h>

uint32_t SystemCoreClock;

// TODO: insert other definitions and declarations here
EthernetHost ethhost;
LCD1602 lcd;
FrontPanelControls frontpanelcontrols;

#ifdef DEBUG
extern "C" void check_failed(uint8_t *file, uint32_t line)
{
	while (1) ;
}
#endif

#if defined (M0_SLAVE_PAUSE_AT_MAIN)
volatile unsigned int pause_at_main;
#endif

void init_freertos_heap()
{
	// Allocate the 32kB+16kB AHB SRAM bank at 0x20000000-0x2000BFFF for the Ethernet/IP stack.
	const HeapRegion_t xHeapRegions[] =
	{
	    { ( uint8_t * ) 0x20000000UL, 0xC000 },
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

void changeled(FrontPanelControls::Led led)
{
	static bool ledstate[6] = { };

	ledstate[led] = !ledstate[led];
	frontpanelcontrols.SetLed(led, ledstate[led]);
}

void vFrontPanelTask(void* pvParameters)
{
	changeled(FrontPanelControls::LedRefLevel4dBu);
	frontpanelcontrols.SetLed(FrontPanelControls::LedEnable, true);

	while(1) {
		bool readencoder = false;

		while (true) {
			bool pressed;
			FrontPanelControls::Button b = frontpanelcontrols.ReadNextButton(pressed);
			if (b == FrontPanelControls::ButtonNone) {
				// out of events
				break;
			}

			switch (b) {
			case FrontPanelControls::ButtonEncoder:
				readencoder = true;
				break;
			case FrontPanelControls::ButtonEnable:
				frontpanelcontrols.SetLed(FrontPanelControls::LedEnable, pressed);
				break;
			case FrontPanelControls::ButtonAuto:
				if (pressed) {
					changeled(FrontPanelControls::LedAuto);
				}
				break;
			case FrontPanelControls::ButtonBandwidthLimit:
				if (pressed) {
					changeled(FrontPanelControls::LedBandwidthLimit);
				}
				break;
			case FrontPanelControls::ButtonBalancedIO:
				if (pressed) {
					changeled(FrontPanelControls::LedBalancedIO);
				}
				break;
			case FrontPanelControls::ButtonCustomLevel:
				if (pressed) {
					changeled(FrontPanelControls::LedRefLevelCustom);
				}
				break;
			case FrontPanelControls::ButtonRefLevel:
				if (pressed) {
					changeled(FrontPanelControls::LedRefLevel4dBu);
					changeled(FrontPanelControls::LedRefLevel10dBV);
				}
				break;
			default:
				break;
			}
		}

		if (readencoder) {
			int gaindelta = frontpanelcontrols.ReadEncoderDelta(FrontPanelControls::EncoderGain);
			int freqdelta = frontpanelcontrols.ReadEncoderDelta(FrontPanelControls::EncoderFrequency);
		}

		taskYIELD();
	}
}

// Main task
void vInitTask(void* pvParameters)
{
	ethhost.Init();
	frontpanelcontrols.Init();

    xTaskCreate(vFrontPanelTask, "frontpanel", 512, NULL, 1 /* priority */, NULL);

	while(1) {
		taskYIELD();
	}
}

int main(void) {

#if defined (M0_SLAVE_PAUSE_AT_MAIN)
    // Pause execution until debugger attaches and modifies variable
    while (pause_at_main == 0) {}
#endif

    SystemCoreClock = 17*12000000;

    // TODO: insert code here
    init_freertos_heap();

    lcd.Init();
    frontpanelcontrols.Init();

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

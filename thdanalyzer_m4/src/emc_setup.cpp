/**********************************************************************
 * $Id: lpc43xx_emc.c 8765 2011-12-08 00:51:21Z nxp21346 $		lpc43xx_emc.c		2011-12-07
 *//**
 * @file		lpc43xx_emc.c
 * @brief	Contains all functions support for Clock Generation and Control
 * 			firmware library on lpc43xx
 * @version	1.0
 * @date		07. December. 2011
 * @author	NXP MCU SW Application Team
 *
 * Copyright(C) 2011, NXP Semiconductor
 * All rights reserved.
 *
 ***********************************************************************
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * products. This software is supplied "AS IS" without any warranties.
 * NXP Semiconductors assumes no responsibility or liability for the
 * use of the software, conveys no license or title under any patent,
 * copyright, or mask work right to the product. NXP Semiconductors
 * reserves the right to make changes in the software without
 * notification. NXP Semiconductors also make no representation or
 * warranty that such application will be suitable for the specified
 * use without further testing or modification.
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors�
 * relevant copyright in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 **********************************************************************/

extern "C" {
#include "LPC43xx.h"
#include "lpc43xx_scu.h"
#include "lpc43xx_cgu.h"
}

#include "emc_setup.h"

#define M32(x)	*((uint32_t *)x)
#define DELAYCYCLES(ns) (ns / ((1.0 / __EMCHZ) * 1E9))

#define __CRYSTAL        (12000000UL)    /* Crystal Oscillator frequency          */
#define __PLLMULT		 (17)
#define __PLLOUTHZ		 (__CRYSTAL * __PLLMULT)
#define __EMCDIV		 (2)
#define __EMCHZ			 (__PLLOUTHZ / __EMCDIV)

#define EMC_SDRAM_WIDTH_8_BITS		0
#define EMC_SDRAM_WIDTH_16_BITS		1
#define EMC_SDRAM_WIDTH_32_BITS		2

#define EMC_SDRAM_SIZE_16_MBITS		0
#define EMC_SDRAM_SIZE_64_MBITS		1
#define EMC_SDRAM_SIZE_128_MBITS	2
#define EMC_SDRAM_SIZE_256_MBITS	3
#define EMC_SDRAM_SIZE_512_MBITS	4

#define EMC_SDRAM_DATA_BUS_16_BITS	0
#define EMC_SDRAM_DATA_BUS_32_BITS	1

#define EMC_B_ENABLE 					(1 << 19)
#define EMC_ENABLE 						(1 << 0)
#define EMC_CE_ENABLE 					(1 << 0)
#define EMC_CS_ENABLE 					(1 << 1)
#define EMC_CLOCK_DELAYED_STRATEGY 		(0 << 0)
#define EMC_COMMAND_DELAYED_STRATEGY 	(1 << 0)
#define EMC_COMMAND_DELAYED_STRATEGY2 	(2 << 0)
#define EMC_COMMAND_DELAYED_STRATEGY3 	(3 << 0)
#define EMC_INIT(i) 					((i) << 7)
#define EMC_NORMAL 						(0)
#define EMC_MODE 						(1)
#define EMC_PRECHARGE_ALL 				(2)
#define EMC_NOP 						(3)

#define SDRAM_WIDTH				EMC_SDRAM_WIDTH_16_BITS
#define SDRAM_SIZE_MBITS		EMC_SDRAM_SIZE_128_MBITS
#define SDRAM_DATA_BUS_BITS		EMC_SDRAM_DATA_BUS_16_BITS
#define SDRAM_COL_ADDR_BITS		9

#define SDRAM_CLK0_DELAY     4

static void emc_waitus(volatile uint32_t us)
{
	us *= (SystemCoreClock / 1000000) / 3;
	while(us--);
}

static void emc_waitms(uint32_t ms)
{
	emc_waitus(ms * 1000);
}

static void emc_pinsetup(void)
{
	/* select correct functions on the GPIOs */

	/* DATA LINES 0..15 > D0..D15 */
	scu_pinmux(0x1,  7,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3);  /* P1_7: D0 (function 0) errata */
	scu_pinmux(0x1,  8,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3);  /* P1_8: D1 (function 0) errata */
	scu_pinmux(0x1,  9,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3);  /* P1_9: D2 (function 0) errata */
	scu_pinmux(0x1,  10, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3);  /* P1_10: D3 (function 0) errata */
	scu_pinmux(0x1,  11, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3);  /* P1_11: D4 (function 0) errata */
	scu_pinmux(0x1,  12, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3);  /* P1_12: D5 (function 0) errata */
	scu_pinmux(0x1,  13, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3);  /* P1_13: D6 (function 0) errata */
	scu_pinmux(0x1,  14, (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3);  /* P1_14: D7 (function 0) errata */
	scu_pinmux(0x5,  4,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC2);  /* P5_4: D8 (function 0) errata */
	scu_pinmux(0x5,  5,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC2);  /* P5_5: D9 (function 0) errata */
	scu_pinmux(0x5,  6,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC2);  /* P5_6: D10 (function 0) errata */
	scu_pinmux(0x5,  7,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC2);  /* P5_7: D11 (function 0) errata */
	scu_pinmux(0x5,  0,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC2);  /* P5_0: D12 (function 0) errata */
	scu_pinmux(0x5,  1,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC2);  /* P5_1: D13 (function 0) errata */
	scu_pinmux(0x5,  2,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC2);  /* P5_2: D14 (function 0) errata */
	scu_pinmux(0x5,  3,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC2);  /* P5_3: D15 (function 0) errata */

	/* ADDRESS LINES A0..A14 > A0..A14 */
	scu_pinmux(0x2,  9,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3);	/* P2_9 - EXTBUS_A0 � External memory address line 0 */
	scu_pinmux(0x2, 10,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3);	/* P2_10 - EXTBUS_A1 � External memory address line 1 */	
	scu_pinmux(0x2, 11,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3);	/* P2_11 - EXTBUS_A2 � External memory address line 2 */	
	scu_pinmux(0x2, 12,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3);	/* P2_12 - EXTBUS_A3 � External memory address line 3 */
	scu_pinmux(0x2, 13,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3);	/* P2_13 - EXTBUS_A4 � External memory address line 4 */	
	scu_pinmux(0x1,  0,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC2);	/* P1_0 - EXTBUS_A5 � External memory address line 5 */
	scu_pinmux(0x1,  1,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC2);	/* P1_1 - EXTBUS_A6 � External memory address line 6 */	
	scu_pinmux(0x1,  2,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC2);	/* P1_2 - EXTBUS_A7 � External memory address line 7 */	
	scu_pinmux(0x2,  8,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3);	/* P2_8 - EXTBUS_A8 � External memory address line 8 */
	scu_pinmux(0x2,  7,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3);	/* P2_7 - EXTBUS_A9 � External memory address line 9 */	
	scu_pinmux(0x2,  6,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC2);	/* P2_6 - EXTBUS_A10 � External memory address line 10 */
	scu_pinmux(0x2,  2,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC2);	/* P2_2 - EXTBUS_A11 � External memory address line 11 */
	scu_pinmux(0x2,  1,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC2);	/* P2_1 - EXTBUS_A12 � External memory address line 12 */
	scu_pinmux(0x2,  0,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC2);	/* P2_0 - EXTBUS_A13 � External memory address line 13 */	
	scu_pinmux(0x6,  8,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC1);	/* P6_8 - EXTBUS_A14 � External memory address line 14 */

	scu_pinmux(0x6,  9,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3);  /* P6_9: EXTBUS_DYCS0  (function 0) > CS# errata */
	scu_pinmux(0x1,  6,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3);  /* P1_6: WE (function 0) errata */
	scu_pinmux(0x6,  4,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3);  /* P6_4: CAS  (function 0) > CAS# errata */
	scu_pinmux(0x6,  5,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3);  /* P6_5: RAS  (function 0) > RAS# errata */

	LPC_SCU_CLK(0) = 0 + (MD_PLN | MD_EZI | MD_ZI | MD_EHS); /* SFSCLK0: EXTBUS_CLK0  (function 0, from datasheet) > CLK ds */
	LPC_SCU_CLK(1) = 0 + (MD_PLN | MD_EZI | MD_ZI | MD_EHS); /* SFSCLK1: EXTBUS_CLK1  (function 0, from datasheet) */
	LPC_SCU_CLK(2) = 0 + (MD_PLN | MD_EZI | MD_ZI | MD_EHS); /* SFSCLK2: EXTBUS_CLK2  (function 0, from datasheet) */
	LPC_SCU_CLK(3) = 0 + (MD_PLN | MD_EZI | MD_ZI | MD_EHS); /* SFSCLK3: EXTBUS_CLK3  (function 0, from datasheet) */

	scu_pinmux(0x6, 11,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3);  /* P6_11: CKEOUT0  (function 0) > CKE errata */
	scu_pinmux(0x6, 12,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3);  /* P6_12: DQMOUT0  (function 0) > DQM0 errata */
	scu_pinmux(0x6, 10,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3);  /* P6_10: DQMOUT1  (function 0) > DQM1 errata */
	//scu_pinmux(0xD,  0,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC2);  /* PD_0: DQMOUT2  (function 2, from datasheet) > DQM2 errata */
	//scu_pinmux(0xE, 13,  (MD_PLN | MD_EZI | MD_ZI | MD_EHS), FUNC3);  /* PE_13: DQMOUT3  (function 3, from datasheet) > DQM3 errata */
}

/* SDRAM refresh time to 16 clock num */
#define EMC_SDRAM_REFRESH(freq,time)  \
		(((uint64_t)((uint64_t)time * freq)/16000000000ull)+1)

static void emc_configure(uint32_t u32BaseAddr, uint32_t u32Width, uint32_t u32Size, uint32_t u32DataBus, uint32_t u32ColAddrBits)
{
#define CAS_LATENCY 2
	// Set all clock delays together
	LPC_SCU->EMCDELAYCLK = (SDRAM_CLK0_DELAY
			|  (SDRAM_CLK0_DELAY << 4)
			|  (SDRAM_CLK0_DELAY << 8)
			|  (SDRAM_CLK0_DELAY << 12));

	/* Initialize EMC to interface with SDRAM */
	LPC_EMC->CONTROL 			= 0x00000001;   /* Enable the external memory controller */	
	LPC_EMC->CONFIG 			= 0;

	// 0<<12: high performance mapping, RBC
	LPC_EMC->DYNAMICCONFIG0 	= ((u32Width << 7) | (u32Size << 9) | (0UL << 12) | (u32DataBus << 14));

	LPC_EMC->DYNAMICRASCAS0 	= (2 << 0) | (CAS_LATENCY << 8);

	LPC_EMC->DYNAMICREADCONFIG	= EMC_COMMAND_DELAYED_STRATEGY;

	LPC_EMC->DYNAMICRP 			= 3-1;
	LPC_EMC->DYNAMICRAS 		= 4-1;
	LPC_EMC->DYNAMICSREX 		= 6-1; // same as tXSR
	LPC_EMC->DYNAMICAPR 		= 2-1;
	LPC_EMC->DYNAMICDAL 		= 4-1;
	LPC_EMC->DYNAMICWR 			= 2-1;
	LPC_EMC->DYNAMICRC 			= 6-1;
	LPC_EMC->DYNAMICRFC 		= 6-1;
	LPC_EMC->DYNAMICXSR 		= 6-1;
	LPC_EMC->DYNAMICRRD 		= 2-1;
	LPC_EMC->DYNAMICMRD 		= 2-1;

	LPC_EMC->DYNAMICCONTROL 	= EMC_CE_ENABLE | EMC_CS_ENABLE | EMC_INIT(EMC_NOP);
	emc_waitus(100);

	LPC_EMC->DYNAMICCONTROL 	= EMC_CE_ENABLE | EMC_CS_ENABLE | EMC_INIT(EMC_PRECHARGE_ALL);

	LPC_EMC->DYNAMICREFRESH 	= 2;
	emc_waitus(100);

	LPC_EMC->DYNAMICREFRESH 	= 120;

	LPC_EMC->DYNAMICCONTROL 	= EMC_CE_ENABLE | EMC_CS_ENABLE | EMC_INIT(EMC_MODE);

	if(u32DataBus == 0) 
	{
		/* burst size 8 */
		*((volatile uint32_t *)(u32BaseAddr | ((3 | (CAS_LATENCY << 4)) << (u32ColAddrBits + 2 + 1))));
	}
	else 
	{
		/* burst size 4 */
		// untested
		*((volatile uint32_t *)(u32BaseAddr | ((2UL | (CAS_LATENCY << 4)) << (u32ColAddrBits + 2))));
	}

	LPC_EMC->DYNAMICCONTROL 	= 0; // EMC_CE_ENABLE | EMC_CS_ENABLE;
	LPC_EMC->DYNAMICCONFIG0 	= ((u32Width << 7) | (u32Size << 9) | (1UL << 12) | (u32DataBus << 14)) | EMC_B_ENABLE;
	LPC_EMC->DYNAMICCONFIG1 	= ((u32Width << 7) | (u32Size << 9) | (1UL << 12) | (u32DataBus << 14)) | EMC_B_ENABLE;
	LPC_EMC->DYNAMICCONFIG2 	= ((u32Width << 7) | (u32Size << 9) | (1UL << 12) | (u32DataBus << 14)) | EMC_B_ENABLE;
	LPC_EMC->DYNAMICCONFIG3 	= ((u32Width << 7) | (u32Size << 9) | (1UL << 12) | (u32DataBus << 14)) | EMC_B_ENABLE;
}

void emc_init()
{
	if (__EMCDIV == 2) {
		LPC_CREG->CREG6 |= (1 << 16); // EMC clock divide by 2
		LPC_CCU1->CLK_M4_EMCDIV_CFG |= (1 << 5); // EMC clock divide by 2
	}

    emc_pinsetup();

    CGU_ConfigPWR(CGU_PERIPHERAL_EMC, ENABLE);
    volatile uint32_t emcclk = CGU_GetPCLKFrequency(CGU_PERIPHERAL_EMC);

    emc_configure(SDRAM_BASE_ADDR, SDRAM_WIDTH, SDRAM_SIZE_MBITS, EMC_SDRAM_DATA_BUS_16_BITS, SDRAM_COL_ADDR_BITS);
}

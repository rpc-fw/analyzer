/**********************************************************************
* $Id: lpc43xx_emc.h 8765 2011-12-08 00:51:21Z nxp21346 $		lpc43xx_emc.h		2011-12-07
*//**
* @file		lpc43xx_emc.h
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

/* Modified by Lauri Koponen 2014-09-17 */

/* SDRAM Address Base for DYCS0 */
#define SDRAM_BASE_ADDR 0x28000000

#define SDRAM_SIZE_BYTES		(1024UL * 1024UL * 16UL)

void emc_init();

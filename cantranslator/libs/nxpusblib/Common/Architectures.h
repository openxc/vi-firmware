/*
* Copyright(C) NXP Semiconductors, 2011
* All rights reserved.
*
* Copyright (C) Dean Camera, 2011.
*
* LUFA Library is licensed from Dean Camera by NXP for NXP customers 
* for use with NXP's LPC microcontrollers.
*
* Software that is described herein is for illustrative purposes only
* which provides customers with programming information regarding the
* LPC products.  This software is supplied "AS IS" without any warranties of
* any kind, and NXP Semiconductors and its licensor disclaim any and 
* all warranties, express or implied, including all implied warranties of 
* merchantability, fitness for a particular purpose and non-infringement of 
* intellectual property rights.  NXP Semiconductors assumes no responsibility
* or liability for the use of the software, conveys no license or rights under any
* patent, copyright, mask work right, or any other intellectual property rights in 
* or to any products. NXP Semiconductors reserves the right to make changes
* in the software without notification. NXP Semiconductors also makes no 
* representation or warranty that such application will be suitable for the
* specified use without further testing or modification.
* 
* Permission to use, copy, modify, and distribute this software and its 
* documentation is hereby granted, under NXP Semiconductors' and its 
* licensor's relevant copyrights in the software, without fee, provided that it 
* is used in conjunction with NXP Semiconductors microcontrollers.  This 
* copyright, permission, and disclaimer notice must appear in all copies of 
* this code.
*/



/** \file
 *  \brief Supported library architecture defines.
 *
 *  \copydetails Group_Architectures
 *
 *  \note Do not include this file directly, rather include the Common.h header file instead to gain this file's
 *        functionality.
 */

/** \ingroup Group_Common
 *  \defgroup Group_Architectures Hardware Architectures
 *  \brief Supported library architecture defines.
 *
 *  Architecture macros for selecting the desired target microcontroller architecture. One of these values should be
 *  defined as the value of \c ARCH in the user project makefile via the \c -D compiler switch to GCC, to select the
 *  target architecture.
 *
 *  The selected architecture should remain consistent with the makefile \c ARCH value, which is used to select the
 *  underlying driver source files for each architecture.
 *
 *  @{
 */

#ifndef __NXPUSBLIB_ARCHITECTURES_H__
#define __NXPUSBLIB_ARCHITECTURES_H__

	/* Preprocessor Checks: */
		#if !defined(__INCLUDE_FROM_COMMON_H)
			#error Do not include this file directly. Include nxpUSBlib/Common/Common.h instead to gain this functionality.
		#endif

	/* Public Interface - May be used in end-application: */
		/* Macros: */
			/** Selects the Atmel 8-bit AVR (AT90USB* and ATMEGA*U* chips) architecture. */
			#define ARCH_AVR8           0

			/** Selects the Atmel 32-bit UC3 AVR (AT32UC3* chips) architecture. */
			#define ARCH_UC3            1
			
			/** Selects the Atmel XMEGA AVR (ATXMEGA*U chips) architecture. */
			#define ARCH_XMEGA          2

			/** Selects the NXP ARM architecture. */
			#define ARCH_LPC            3

			#if !defined(__DOXYGEN__)
				#define ARCH_           ARCH_LPC

				#if !defined(ARCH)
					#define ARCH        ARCH_LPC
				#endif
			#endif

#endif

/** @} */


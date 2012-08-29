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
 *  \brief Architecture specific definitions relating to specific processor architectures.
 *
 *  \copydetails Group_ArchitectureSpecific
 *
 *  \note Do not include this file directly, rather include the Common.h header file instead to gain this file's
 *        functionality.
 */

/** \ingroup Group_Common
 *  \defgroup Group_ArchitectureSpecific Architecture Specific Definitions
 *  \brief Architecture specific definitions relating to specific processor architectures.
 *
 *  Architecture specific macros, functions and other definitions, which relate to specific architectures. This
 *  definitions may or may not be available in some form on other architectures, and thus should be protected by
 *  preprocessor checks in portable code to prevent compile errors.
 *
 *  @{
 */

#ifndef __NXPUSBLIB_ARCHSPEC_H__
#define __NXPUSBLIB_ARCHSPEC_H__

	/* Preprocessor Checks: */
		#if !defined(__INCLUDE_FROM_COMMON_H)
			#error Do not include this file directly. Include nxpUSBlib/Common/Common.h instead to gain this functionality.
		#endif

	/* Enable C linkage for C++ Compilers: */
		#if defined(__cplusplus)
			extern "C" {
		#endif

	/* Public Interface - May be used in end-application: */
		/* Macros: */
			#if (ARCH == ARCH_AVR8) || defined(__DOXYGEN__)
				/** Disables the AVR's JTAG bus in software, until a system reset. This will override the current JTAG
				 *  status as set by the JTAGEN fuse, disabling JTAG debugging and reverting the JTAG pins back to GPIO
				 *  mode.
				 *
				 *  \note This macro is not available for all architectures.
				 */
				#define JTAG_DISABLE()                  MACROS{                                      \
				                                                __asm__ __volatile__ (               \
				                                                "in __tmp_reg__,__SREG__" "\n\t"     \
				                                                "cli" "\n\t"                         \
				                                                "out %1, %0" "\n\t"                  \
				                                                "out __SREG__, __tmp_reg__" "\n\t"   \
				                                                "out %1, %0" "\n\t"                  \
				                                                :                                    \
				                                                : "r" (1 << JTD),                    \
				                                                  "M" (_SFR_IO_ADDR(MCUCR))          \
				                                                : "r0");                             \
				                                        }MACROE
			
				/** Defines a volatile \c NOP statement which cannot be optimized out by the compiler, and thus can always
				 *  be set as a breakpoint in the resulting code. Useful for debugging purposes, where the optimizer
				 *  removes/reorders code to the point where break points cannot reliably be set.
				 *
				 *  \note This macro is not available for all architectures.
				 */
				#define JTAG_DEBUG_POINT()              __asm__ __volatile__ ("nop" ::)

				/** Defines an explicit JTAG break point in the resulting binary via the assembly \c BREAK statement. When
				 *  a JTAG is used, this causes the program execution to halt when reached until manually resumed.
				 *
				 *  \note This macro is not available for all architectures.
				 */
				#define JTAG_DEBUG_BREAK()              __asm__ __volatile__ ("break" ::)

				/** Macro for testing condition "x" and breaking via \ref JTAG_DEBUG_BREAK() if the condition is false.
				 *
				 *  \note This macro is not available for all architectures.
				 *
				 *  \param[in] Condition  Condition that will be evaluated.
				*/
				#define JTAG_ASSERT(Condition)          MACROS{ if (!(Condition)) { JTAG_DEBUG_BREAK(); } }MACROE

				/** Macro for testing condition \c "x" and writing debug data to the stdout stream if \c false. The stdout stream
				 *  must be pre-initialized before this macro is run and linked to an output device, such as the microcontroller's
				 *  USART peripheral.
				 *
				 *  The output takes the form "{FILENAME}: Function {FUNCTION NAME}, Line {LINE NUMBER}: Assertion {Condition} failed."
				 *
				 *  \note This macro is not available for all architectures.
				 *
				 *  \param[in] Condition  Condition that will be evaluated,
				 */
				#define STDOUT_ASSERT(Condition)        MACROS{ if (!(x)) { printf_P(PSTR("%s: Function \"%s\", Line %d: "   \
				                                                "Assertion \"%s\" failed.\r\n"),     \
				                                                __FILE__, __func__, __LINE__, #Condition); } }MACROE

				#if !defined(pgm_read_ptr) || defined(__DOXYGEN__)
					/** Reads a pointer out of PROGMEM space on the AVR8 architecture. This is currently a wrapper for the
					 *  avr-libc \c pgm_read_ptr() macro with a \c void* cast, so that its value can be assigned directly
					 *  to a pointer variable or used in pointer arithmetic without further casting in C. In a future
					 *  avr-libc distribution this will be part of the standard API and will be implemented in a more formal
					 *  manner.
					 *
					 *  \note This macro is not available for all architectures.
					 *
					 *  \param[in] Address  Address of the pointer to read.
					 *
					 *  \return Pointer retrieved from PROGMEM space.
					 */
					#define pgm_read_ptr(Address)        (void*)pgm_read_word(Address)
				#endif
			#endif
			
	/* Disable C linkage for C++ Compilers: */
		#if defined(__cplusplus)
			}
		#endif

#endif

/** @} */


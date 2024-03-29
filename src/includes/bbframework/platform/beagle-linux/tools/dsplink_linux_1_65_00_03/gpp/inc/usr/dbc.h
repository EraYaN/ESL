/** ============================================================================
 *  @file   dbc.h
 *
 *  @path   $(DSPLINK)/gpp/inc/usr/
 *
 *  @desc   Design by Contract
 *
 *  @ver    1.65.00.03
 *  ============================================================================
 *  Copyright (C) 2002-2009, Texas Instruments Incorporated -
 *  http://www.ti.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  
 *  *  Neither the name of Texas Instruments Incorporated nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *  ============================================================================
 */


#if !defined (DBC_H)
#define DBC_H


/*  ----------------------------------- DSP/BIOS Link                   */
#include <dsplink.h>


#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


/*  ============================================================================
 *  @macro  DBC_PRINTF
 *
 *  @desc   This macro expands to the print function. It makes the DBC
 *          macros portable across OSes.
 *  ============================================================================
 */
#define  DBC_PRINTF     PRINT_Printf


#if defined (DDSP_DEBUG)

/** ============================================================================
 *  @macro  DBC_Assert
 *
 *  @desc   Assert on expression.
 *  ============================================================================
 */
#define DBC_Assert(exp)                                                        \
        if (!(exp)) {                                                          \
            DBC_PRINTF ("Assertion failed ("#exp"). File : "__FILE__           \
                        " Line : %d\n", __LINE__) ;                            \
        }

/** ============================================================================
 *  @macro  DBC_Require
 *
 *  @desc   Function Precondition.
 *  ============================================================================
 */
#define DBC_Require    DBC_Assert

/** ============================================================================
 *  @macro  DBC_Ensure
 *
 *  @desc   Function Postcondition.
 *  ============================================================================
 */
#define DBC_Ensure     DBC_Assert

#else /* defined (DDSP_DEBUG) */

/*  ============================================================================
 *  @macro  DBC_Assert/DBC_Require/DBC_Ensure
 *
 *  @desc   Asserts defined out.
 *  ============================================================================
 */
#define DBC_Assert(exp)
#define DBC_Require(exp)
#define DBC_Ensure(exp)

#endif /* defined (DDSP_DEBUG) */


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */


#endif  /* !defined (DBC_H) */
